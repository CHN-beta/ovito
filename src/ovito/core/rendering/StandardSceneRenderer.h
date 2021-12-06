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
 * \brief This is the default scene renderer used for high-quality image output.
 */
class OVITO_CORE_EXPORT StandardSceneRenderer : public SceneRenderer
{
	OVITO_CLASS(StandardSceneRenderer)
	Q_CLASSINFO("DisplayName", "OpenGL");
	Q_CLASSINFO("Description", "Hardware-accelerated rendering engine, also used by OVITO's interactive viewports. "
							   "The OpenGL renderer is fast and has the smallest memory footprint.");

public:

	/// Constructor.
	Q_INVOKABLE StandardSceneRenderer(DataSet* dataset);

	/// Prepares the renderer for rendering an image or animation and sets the dataset being rendered.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings, const QSize& frameBufferSize) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect) override;

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, SynchronousOperation operation) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer, const QRect& viewportRect) override;

	/// Is called after rendering has finished.
	virtual void endRender() override;

private:

	/// Controls the number of sub-pixels to render.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, antialiasingLevel, setAntialiasingLevel);

	/// The active renderer implementation (OpenGL or Vulkan). 
	OORef<SceneRenderer> _internalRenderer;
};

}	// End of namespace
