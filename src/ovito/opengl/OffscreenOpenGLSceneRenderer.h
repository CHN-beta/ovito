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
#include "OpenGLSceneRenderer.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

namespace Ovito {

/**
 * \brief OpenGL renderer that renders into an offscreen framebuffer instead of the interactive viewports.
 */
class OVITO_OPENGLRENDERER_EXPORT OffscreenOpenGLSceneRenderer : public OpenGLSceneRenderer
{
	Q_OBJECT
	OVITO_CLASS(OffscreenOpenGLSceneRenderer)

public:

	/// Constructor.
	Q_INVOKABLE OffscreenOpenGLSceneRenderer(DataSet* dataset);

	/// Prepares the renderer for rendering and sets the data set that is being rendered.
	virtual bool startRender(DataSet* dataset, RenderSettings* settings, const QSize& frameBufferSize) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer) override;

	/// Is called after rendering has finished.
	virtual void endRender() override;

	/// Returns the final size of the rendered image in pixels.
	virtual QSize outputSize() const override { return _outputSize; }

protected:

	/// Puts the GL context into its default initial state before rendering of a frame begins.
	virtual void initializeGLState() override;

private:

	/// Creates the QOffscreenSurface in the main thread.
	void createOffscreenSurface();

private:

	/// The offscreen surface used to render into an image buffer using OpenGL.
	QOffscreenSurface* _offscreenSurface = nullptr;

	/// The temporary OpenGL rendering context.
	QScopedPointer<QOpenGLContext> _offscreenContext;

	/// The OpenGL framebuffer.
	QScopedPointer<QOpenGLFramebufferObject> _framebufferObject;

	/// The resolution of the offscreen framebuffer.
	QSize _framebufferSize;

	/// The resolution of the output image.
	QSize _outputSize;
};

}	// End of namespace