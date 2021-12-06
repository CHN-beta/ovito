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
#include <ovito/core/rendering/SceneRenderer.h>

namespace Ovito {

/**
 * \brief A scene renderer that makes use of the Qt3D framework.
 */
class OVITO_QT3DRENDERER_EXPORT Qt3DSceneRenderer : public SceneRenderer
{
	OVITO_CLASS(Qt3DSceneRenderer)

public:

	/// Constructor.
	explicit Qt3DSceneRenderer(DataSet* dataset);

	/// Destructor.
	virtual ~Qt3DSceneRenderer();

	/// This may be called on a renderer before startRender() to control its supersampling level.
	virtual void setAntialiasingHint(int antialiasingLevel) override { _antialiasingLevel = antialiasingLevel; }

	/// Returns the device pixel ratio of the output device we are rendering to.
	virtual qreal devicePixelRatio() const override { return antialiasingLevel() * SceneRenderer::devicePixelRatio(); }

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, SynchronousOperation operation) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect) override;

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	/// This is an internal method used by the PickingQt3DSceneRenderer class to implement the picking mechanism.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount, const ConstDataBufferPtr& indices = {}) { return 1; }

	/// Temporarily enables/disables the depth test while rendering.
	virtual void setDepthTestEnabled(bool enabled) override;

	/// Activates the special highlight rendering mode.
	virtual void setHighlightMode(int pass) override;

	/// Returns the size in pixels of the Vulkan frame buffer we are rendering into.
	const QSize& frameBufferSize() const { return _frameBufferSize; }

	/// Sets the size in pixels of the Vulkan frame buffer we are rendering into.
	void setFrameBufferSize(const QSize& size) { _frameBufferSize = size; }

protected:

	/// Returns the supersampling level.
	int antialiasingLevel() const { return _antialiasingLevel; }

private:

	/// Controls the number of sub-pixels to render.
	int _antialiasingLevel = 1;

	/// Indicates whether depth testing is currently enabled for drawing commands.
	bool _depthTestEnabled = true;

	/// The size of the frame buffer we are rendering into.
	QSize _frameBufferSize;
};

}	// End of namespace
