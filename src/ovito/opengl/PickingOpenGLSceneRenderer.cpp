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
#include "PickingOpenGLSceneRenderer.h"
#include "OpenGLDepthTextureBlitter.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(PickingOpenGLSceneRenderer);

/******************************************************************************
* Constructor.
******************************************************************************/
PickingOpenGLSceneRenderer::PickingOpenGLSceneRenderer(ObjectCreationParams params) : OffscreenInteractiveOpenGLSceneRenderer(params) 
{
	setPicking(true);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool PickingOpenGLSceneRenderer::renderFrame(const QRect& viewportRect, MainThreadOperation& operation)
{
	// Clear previous object records.
	reset();

	// Let the base class do the main rendering work.
	if(!OffscreenInteractiveOpenGLSceneRenderer::renderFrame(viewportRect, operation))
		return false;

	if(framebufferObject()) {
		// Fetch rendered image from the OpenGL framebuffer.
		QSize size = framebufferObject()->size();

#ifndef Q_OS_WASM
		// Acquire OpenGL depth buffer data.
		// The depth information is used to compute the XYZ coordinate of the point under the mouse cursor.
		_depthBufferBits = glformat().depthBufferSize();
		if(_depthBufferBits == 16) {
			_depthBuffer = std::make_unique<quint8[]>(size.width() * size.height() * sizeof(GLushort));
			glReadPixels(0, 0, size.width(), size.height(), GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, _depthBuffer.get());
		}
		else if(_depthBufferBits == 24) {
			_depthBuffer = std::make_unique<quint8[]>(size.width() * size.height() * sizeof(GLuint));
			while(glGetError() != GL_NO_ERROR);
			glReadPixels(0, 0, size.width(), size.height(), 0x84F9 /*GL_DEPTH_STENCIL*/, 0x84FA /*GL_UNSIGNED_INT_24_8*/, _depthBuffer.get());
			if(glGetError() != GL_NO_ERROR) {
				glReadPixels(0, 0, size.width(), size.height(), GL_DEPTH_COMPONENT, GL_FLOAT, _depthBuffer.get());
				_depthBufferBits = 0;
			}
		}
		else if(_depthBufferBits == 32) {
			_depthBuffer = std::make_unique<quint8[]>(size.width() * size.height() * sizeof(GLuint));
			glReadPixels(0, 0, size.width(), size.height(), GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, _depthBuffer.get());
		}
		else {
			_depthBuffer = std::make_unique<quint8[]>(size.width() * size.height() * sizeof(GLfloat));
			glReadPixels(0, 0, size.width(), size.height(), GL_DEPTH_COMPONENT, GL_FLOAT, _depthBuffer.get());
			_depthBufferBits = 0;
		}
#endif
	}
	else {
		// Create a temporary OpenGL framebuffer.
		QOpenGLFramebufferObjectFormat framebufferFormat;
		QSize size = viewport()->window()->viewportWindowDeviceSize();
		QOpenGLFramebufferObject framebufferObject(size, framebufferFormat);

		// Clear OpenGL error state and verify validity of framebuffer.
		while(this->glGetError() != GL_NO_ERROR);
		if(!framebufferObject.isValid())
			throwRendererException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

		// Bind OpenGL framebuffer.
		if(!framebufferObject.bind())
			throwRendererException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

		// Reset OpenGL context state.
		this->glDisable(GL_CULL_FACE);
		this->glDisable(GL_STENCIL_TEST);
		this->glDisable(GL_BLEND);
		this->glDisable(GL_DEPTH_TEST);

		// Transfer depth buffer to the color buffer so that the pixel data can be read.
		// WebGL1 doesn't allow direct reading the data of a depth texture.
		OpenGLDepthTextureBlitter blitter;
		blitter.create();
		blitter.bind();
		blitter.blit(depthTextureId());
		blitter.release();

		// Read depth buffer contents from the color attachment of the framebuffer.
		// Depth values are encoded as RGB values in each pixel.
		_depthBufferBits = 24;
		_depthBuffer = std::make_unique<quint8[]>(size.width() * size.height() * sizeof(GLuint));
		OVITO_CHECK_OPENGL(this, this->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, _depthBuffer.get()));
	}

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void PickingOpenGLSceneRenderer::endFrame(bool renderingSuccessful, const QRect& viewportRect)
{
	endPickObject();

	OffscreenInteractiveOpenGLSceneRenderer::endFrame(renderingSuccessful, viewportRect);
}

/******************************************************************************
* Resets the internal state of the picking renderer and clears the stored object records.
******************************************************************************/
void PickingOpenGLSceneRenderer::reset()
{
	_objects.clear();
	endPickObject();
#if 1
	_nextAvailablePickingID = 1;
#else
	// This can be enabled during debugging to avoid alpha!=1 pixels in the picking render buffer.
	_nextAvailablePickingID = 0xEF000000;
#endif
	discardFramebufferImage();
}

/******************************************************************************
* When picking mode is active, this registers an object being rendered.
******************************************************************************/
quint32 PickingOpenGLSceneRenderer::beginPickObject(const PipelineSceneNode* objNode, ObjectPickInfo* pickInfo)
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
quint32 PickingOpenGLSceneRenderer::registerSubObjectIDs(quint32 subObjectCount, const ConstDataBufferPtr& indices)
{
	OVITO_ASSERT_MSG(_currentObject.objectNode, "PickingOpenGLSceneRenderer::registerSubObjectIDs()", "You forgot to register the current object via beginPickObject().");

	quint32 baseObjectID = _nextAvailablePickingID;
	if(indices)
		_currentObject.indexedRanges.push_back(std::make_pair(indices, _nextAvailablePickingID - _currentObject.baseObjectID));
	_nextAvailablePickingID += subObjectCount;
	return baseObjectID;
}

/******************************************************************************
* Call this when rendering of a pickable object is finished.
******************************************************************************/
void PickingOpenGLSceneRenderer::endPickObject()
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
std::tuple<const PickingOpenGLSceneRenderer::ObjectRecord*, quint32> PickingOpenGLSceneRenderer::objectAtLocation(const QPoint& pos) const
{
	if(!framebufferImage().isNull()) {
		if(pos.x() >= 0 && pos.x() < framebufferImage().width() && pos.y() >= 0 && pos.y() < framebufferImage().height()) {
			QPoint mirroredPos(pos.x(), framebufferImage().height() - 1 - pos.y());
			QRgb pixel = framebufferImage().pixel(mirroredPos);
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
	return std::tuple<const PickingOpenGLSceneRenderer::ObjectRecord*, quint32>(nullptr, 0);
}

/******************************************************************************
* Given an object ID, looks up the corresponding record.
******************************************************************************/
const PickingOpenGLSceneRenderer::ObjectRecord* PickingOpenGLSceneRenderer::lookupObjectRecord(quint32 objectID) const
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
* Returns the Z-value at the given window position.
******************************************************************************/
FloatType PickingOpenGLSceneRenderer::depthAtPixel(const QPoint& pos) const
{
	if(!framebufferImage().isNull() && _depthBuffer) {
		int w = framebufferImage().width();
		int h = framebufferImage().height();
		if(pos.x() >= 0 && pos.x() < w && pos.y() >= 0 && pos.y() < h) {
			QPoint mirroredPos(pos.x(), framebufferImage().height() - 1 - pos.y());
			if(framebufferImage().pixel(mirroredPos) != 0) {
				if(_depthBufferBits == 16) {
					GLushort bval = reinterpret_cast<const GLushort*>(_depthBuffer.get())[(mirroredPos.y()) * w + pos.x()];
					return (FloatType)bval / FloatType(65535.0);
				}
				else if(_depthBufferBits == 24) {
					GLuint bval = reinterpret_cast<const GLuint*>(_depthBuffer.get())[(mirroredPos.y()) * w + pos.x()];
					return (FloatType)((bval >> 8) & 0x00FFFFFF) / FloatType(16777215.0);
				}
				else if(_depthBufferBits == 32) {
					GLuint bval = reinterpret_cast<const GLuint*>(_depthBuffer.get())[(mirroredPos.y()) * w + pos.x()];
					return (FloatType)bval / FloatType(4294967295.0);
				}
				else if(_depthBufferBits == 0) {
					return reinterpret_cast<const GLfloat*>(_depthBuffer.get())[(mirroredPos.y()) * w + pos.x()];
				}
			}
		}
	}
	return 0;
}

/******************************************************************************
* Returns the world space position corresponding to the given screen position.
******************************************************************************/
Point3 PickingOpenGLSceneRenderer::worldPositionFromLocation(const QPoint& pos) const
{
	FloatType zvalue = depthAtPixel(pos);
	if(zvalue != 0) {
		OVITO_ASSERT(!framebufferImage().isNull());
		Point3 ndc(
				(FloatType)pos.x() / framebufferImage().width() * FloatType(2) - FloatType(1),
				1.0 - (FloatType)pos.y() / framebufferImage().height() * FloatType(2),
				zvalue * FloatType(2) - FloatType(1));
		Point3 worldPos = projParams().inverseViewMatrix * (projParams().inverseProjectionMatrix * ndc);
		return worldPos;
	}
	return Point3::Origin();
}

}	// End of namespace
