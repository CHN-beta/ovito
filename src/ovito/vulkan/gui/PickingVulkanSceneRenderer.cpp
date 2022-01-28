////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/core/Core.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "PickingVulkanSceneRenderer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(PickingVulkanSceneRenderer);

/******************************************************************************
* Constructor.
******************************************************************************/
PickingVulkanSceneRenderer::PickingVulkanSceneRenderer(ObjectCreationParams params, std::shared_ptr<VulkanContext> vulkanDevice, ViewportWindowInterface* window) 
	: OffscreenVulkanSceneRenderer(params, std::move(vulkanDevice), true), _window(window) 
{
	setPicking(true);
	setInteractive(true);
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void PickingVulkanSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect, FrameBuffer* frameBuffer)
{
	// Caller should never provide an external frame buffer.
	OVITO_ASSERT(!frameBuffer);

	// Use our internal frame buffer instead.
	frameBuffer = &_frameBuffer;

	OffscreenVulkanSceneRenderer::beginFrame(time, params, vp, viewportRect, frameBuffer);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool PickingVulkanSceneRenderer::renderFrame(const QRect& viewportRect, MainThreadOperation& operation)
{
	// Caller should never provide an external frame buffer.
	OVITO_ASSERT(frameBuffer() == &_frameBuffer);

	// Clear previous object records.
	resetPickingBuffer();

	// Let the base class do the main rendering work.
	if(!OffscreenVulkanSceneRenderer::renderFrame(viewportRect, operation))
		return false;

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void PickingVulkanSceneRenderer::endFrame(bool renderingSuccessful, const QRect& viewportRect)
{
	// Caller should never provide an external frame buffer.
	OVITO_ASSERT(frameBuffer() == &_frameBuffer);

	// Make sure old framebuffer content has been discarded, because we don't want OffscreenVulkanSceneRenderer::endFrame() to blend images. 
	OVITO_ASSERT(_frameBuffer.image().isNull());

	// Reset state.
	endPickObject();

	// Let the base implementation fetch the Vulkan framebuffer contents.
	OffscreenVulkanSceneRenderer::endFrame(renderingSuccessful, viewportRect);
}

/******************************************************************************
* Resets the internal state of the picking renderer and clears the stored object records.
******************************************************************************/
void PickingVulkanSceneRenderer::resetPickingBuffer()
{
	_objects.clear();
	endPickObject();
#if 1
	_nextAvailablePickingID = 1;
#else
	// This can be enabled during debugging to avoid alpha!=1 pixels in the picking render buffer.
	_nextAvailablePickingID = 0xEF000000;
#endif
	_frameBuffer.image() = QImage();
}

/******************************************************************************
* When picking mode is active, this registers an object being rendered.
******************************************************************************/
quint32 PickingVulkanSceneRenderer::beginPickObject(const PipelineSceneNode* objNode, ObjectPickInfo* pickInfo)
{
	OVITO_ASSERT(objNode != nullptr);
	OVITO_ASSERT(isPicking());

	_currentObject.objectNode = const_cast<PipelineSceneNode*>(objNode);
	_currentObject.pickInfo = pickInfo;
	_currentObject.baseObjectID = _nextAvailablePickingID;
	return _currentObject.baseObjectID;
}

/******************************************************************************
* Registers a range of sub-IDs belonging to the current object being rendered.
******************************************************************************/
quint32 PickingVulkanSceneRenderer::registerSubObjectIDs(quint32 subObjectCount, const ConstDataBufferPtr& indices)
{
	OVITO_ASSERT_MSG(_currentObject.objectNode, "PickingVulkanSceneRenderer::registerSubObjectIDs()", "You forgot to register the current object via beginPickObject().");

	quint32 baseObjectID = _nextAvailablePickingID;
	if(indices)
		_currentObject.indexedRanges.push_back(std::make_pair(indices, _nextAvailablePickingID - _currentObject.baseObjectID));
	_nextAvailablePickingID += subObjectCount;
	return baseObjectID;
}

/******************************************************************************
* Call this when rendering of a pickable object is finished.
******************************************************************************/
void PickingVulkanSceneRenderer::endPickObject()
{
	if(_currentObject.objectNode) {
		_objects.push_back(std::move(_currentObject));
	}
	_currentObject.baseObjectID = 0;
	_currentObject.objectNode = nullptr;
	_currentObject.pickInfo = nullptr;
	_currentObject.indexedRanges.clear();
}

/******************************************************************************
* Returns the object record and the sub-object ID for the object at the given pixel coordinates.
******************************************************************************/
std::tuple<const PickingVulkanSceneRenderer::ObjectRecord*, quint32> PickingVulkanSceneRenderer::objectAtLocation(const QPoint& pos) const
{
	if(!_frameBuffer.image().isNull()) {
		if(pos.x() >= 0 && pos.x() < _frameBuffer.image().width() && pos.y() >= 0 && pos.y() < _frameBuffer.image().height()) {
			QRgb pixel = _frameBuffer.image().pixel(pos);
			quint32 red = qRed(pixel);
			quint32 green = qGreen(pixel);
			quint32 blue = qBlue(pixel);
			quint32 alpha = qAlpha(pixel);
			quint32 objectID = red + (green << 8) + (blue << 16) + (alpha << 24);
			if(const ObjectRecord* objRecord = lookupObjectRecord(objectID)) {
				quint32 subObjectID = objectID - objRecord->baseObjectID;
				for(const auto& range : objRecord->indexedRanges) {
					if(subObjectID >= range.second && subObjectID < range.second + range.first->size()) {
						subObjectID = range.second + ConstDataBufferAccess<int>(range.first).get(subObjectID - range.second);
						break;
					}
				}
				return std::make_tuple(objRecord, subObjectID);
			}
		}
	}
	return std::tuple<const PickingVulkanSceneRenderer::ObjectRecord*, quint32>(nullptr, 0);
}

/******************************************************************************
* Given an object ID, looks up the corresponding record.
******************************************************************************/
const PickingVulkanSceneRenderer::ObjectRecord* PickingVulkanSceneRenderer::lookupObjectRecord(quint32 objectID) const
{
	if(objectID == 0 || _objects.empty())
		return nullptr;

	for(auto iter = _objects.begin(); iter != _objects.end(); iter++) {
		if(iter->baseObjectID > objectID) {
			OVITO_ASSERT(iter != _objects.begin());
			OVITO_ASSERT(objectID >= (iter-1)->baseObjectID);
			return &*std::prev(iter);
		}
	}

	OVITO_ASSERT(objectID >= _objects.back().baseObjectID);
	return &_objects.back();
}

/******************************************************************************
* Returns the world space position corresponding to the given screen position.
******************************************************************************/
Point3 PickingVulkanSceneRenderer::worldPositionFromLocation(const QPoint& pos) const
{
	FloatType zvalue = depthAtPixel(pos);
	if(zvalue != 0) {
		Point3 ndc(
				(FloatType)pos.x() / _frameBuffer.image().width() * FloatType(2) - FloatType(1),
				(FloatType)pos.y() / _frameBuffer.image().height() * FloatType(2) - FloatType(1),
				zvalue);
		Point3 worldPos = projParams().inverseViewMatrix * (projParams().inverseProjectionMatrix * clipCorrection().inverse() * ndc);
		return worldPos;
	}
	return Point3::Origin();
}

}	// End of namespace
