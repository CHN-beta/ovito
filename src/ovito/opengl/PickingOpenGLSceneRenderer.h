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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/opengl/OffscreenInteractiveOpenGLSceneRenderer.h>

namespace Ovito {

/**
 * \brief A viewport renderer used for object picking.
 */
class OVITO_OPENGLRENDERER_EXPORT PickingOpenGLSceneRenderer : public OffscreenInteractiveOpenGLSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(PickingOpenGLSceneRenderer)

public:

	struct ObjectRecord {
		quint32 baseObjectID;
		OORef<PipelineSceneNode> objectNode;
		OORef<ObjectPickInfo> pickInfo;
		std::vector<std::pair<ConstDataBufferPtr, quint32>> indexedRanges;
	};

public:

	/// Constructor.
	explicit PickingOpenGLSceneRenderer(DataSet* dataset);

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, StereoRenderingTask stereoTask, SynchronousOperation operation) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer, const QRect& viewportRect) override;

	/// When picking mode is active, this registers an object being rendered.
	virtual quint32 beginPickObject(const PipelineSceneNode* objNode, ObjectPickInfo* pickInfo = nullptr) override;

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount, const ConstDataBufferPtr& indices) override;

	/// Call this when rendering of a pickable object is finished.
	virtual void endPickObject() override;

	/// Returns the object record and the sub-object ID for the object at the given pixel coordinates.
	std::tuple<const ObjectRecord*, quint32> objectAtLocation(const QPoint& pos) const;

	/// Given an object ID, looks up the corresponding record.
	const ObjectRecord* lookupObjectRecord(quint32 objectID) const;

	/// Returns the world space position corresponding to the given screen position.
	Point3 worldPositionFromLocation(const QPoint& pos) const;

	/// Returns true if the picking buffer needs to be regenerated; returns false if the picking buffer still contains valid data.
	bool isRefreshRequired() const { return framebufferImage().isNull(); }

	/// Resets the picking buffer and clears the stored object records.
	void reset();

	/// Returns the Z-value at the given window position.
	FloatType depthAtPixel(const QPoint& pos) const;

private:

	/// The next available object ID.
	ObjectRecord _currentObject;

	/// The next available object ID.
	quint32 _nextAvailablePickingID;

	/// The list of registered objects.
	std::vector<ObjectRecord> _objects;

	/// The depth buffer data.
	std::unique_ptr<quint8[]> _depthBuffer;

	/// The number of depth buffer bits per pixel.
	int _depthBufferBits;
};

}	// End of namespace
