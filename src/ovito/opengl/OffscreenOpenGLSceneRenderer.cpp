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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/app/Application.h>
#include "OffscreenOpenGLSceneRenderer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(OffscreenOpenGLSceneRenderer);

/******************************************************************************
* Constructor.
******************************************************************************/
OffscreenOpenGLSceneRenderer::OffscreenOpenGLSceneRenderer(DataSet* dataset) : OpenGLSceneRenderer(dataset) 
{
	// Create the offscreen surface.
	// This must happen in the main thread.
	createOffscreenSurface();

	// Initialize OpenGL in main thread if it hasn't already been initialized.
	// This call is a workaround for an access vialotion that otherwise occurs on Windows
	// when creating the first OpenGL context from a worker thread when running in headless mode. 
	OpenGLSceneRenderer::determineOpenGLInfo();
}

/******************************************************************************
* Creates the QOffscreenSurface in the main thread.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::createOffscreenSurface() 
{ 
	OVITO_ASSERT(QThread::currentThread() == qApp->thread());
	
	if(!_offscreenSurface)
		_offscreenSurface = new QOffscreenSurface(nullptr, this);
	if(_offscreenContext)
		_offscreenSurface->setFormat(_offscreenContext->format());
	else if(QOpenGLContext::globalShareContext())
		_offscreenSurface->setFormat(QOpenGLContext::globalShareContext()->format());
	else
		_offscreenSurface->setFormat(QSurfaceFormat::defaultFormat());
	_offscreenSurface->create(); 
}

/******************************************************************************
* Prepares the renderer for rendering and sets the data set that is being rendered.
******************************************************************************/
bool OffscreenOpenGLSceneRenderer::startRender(DataSet* dataset, RenderSettings* settings, const QSize& frameBufferSize)
{
	if(Application::instance()->headlessMode())
		throwException(tr("Cannot use OpenGL renderer when running in headless mode. "
				"Please use a different rendering engine or run program on a machine where access to "
				"graphics hardware is possible."));

	if(!OpenGLSceneRenderer::startRender(dataset, settings, frameBufferSize))
		return false;

	// Create a OpenGL context for rendering to an offscreen buffer.
	_offscreenContext.reset(new QOpenGLContext());
	// The context should share its resources with the one of the viewport renderers.
	_offscreenContext->setShareContext(QOpenGLContext::globalShareContext());
	if(!_offscreenContext->create())
		throwException(tr("Failed to create OpenGL context for rendering."));

	// Check offscreen surface (creation must happen in the main thread).
	OVITO_ASSERT(_offscreenSurface);
	if(!_offscreenSurface->isValid())
		throwException(tr("Failed to create offscreen rendering surface."));
	OVITO_ASSERT(_offscreenSurface->format() == _offscreenContext->format());

	// Make the context current.
	if(!_offscreenContext->makeCurrent(_offscreenSurface))
		throwException(tr("Failed to make OpenGL context current."));

	// Check OpenGL version.
	if(_offscreenContext->format().majorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MAJOR || (_offscreenContext->format().majorVersion() == OVITO_OPENGL_MINIMUM_VERSION_MAJOR && _offscreenContext->format().minorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MINOR)) {
		throwException(tr(
				"The OpenGL implementation available on this system does not support OpenGL version %4.%5 or newer.\n\n"
				"Ovito requires modern graphics hardware to accelerate 3d rendering. You current system configuration is not compatible with Ovito.\n\n"
				"To avoid this error message, please install the newest graphics driver, or upgrade your graphics card.\n\n"
				"The currently installed OpenGL graphics driver reports the following information:\n\n"
				"OpenGL Vendor: %1\n"
				"OpenGL Renderer: %2\n"
				"OpenGL Version: %3\n\n"
				"Ovito requires OpenGL version %4.%5 or higher.")
				.arg(QString(OpenGLSceneRenderer::openGLVendor()))
				.arg(QString(OpenGLSceneRenderer::openGLRenderer()))
				.arg(QString(OpenGLSceneRenderer::openGLVersion()))
				.arg(OVITO_OPENGL_MINIMUM_VERSION_MAJOR)
				.arg(OVITO_OPENGL_MINIMUM_VERSION_MINOR)
				);
	}

	// Prepare a functions table allowing us to call OpenGL functions in a platform-independent way.
    initializeOpenGLFunctions();

	// Determine internal framebuffer size including supersampling.
	_outputSize = frameBufferSize;
	_framebufferSize = QSize(_outputSize.width() * antialiasingLevel(), _outputSize.height() * antialiasingLevel());

	// Create OpenGL framebuffer.
	QOpenGLFramebufferObjectFormat framebufferFormat;
	framebufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	_framebufferObject.reset(new QOpenGLFramebufferObject(_framebufferSize, framebufferFormat));
	if(!_framebufferObject->isValid())
		throwException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

	// Bind OpenGL buffer.
	if(!_framebufferObject->bind())
		throwException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	// Make GL context current.
	if(!_offscreenContext || !_offscreenContext->makeCurrent(_offscreenSurface))
		throwException(tr("Failed to make OpenGL context current."));

	OpenGLSceneRenderer::beginFrame(time, params, vp);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::initializeGLState()
{
	OpenGLSceneRenderer::initializeGLState();

	setRenderingViewport(QRect(QPoint(0,0), _framebufferSize));
	setDepthTestEnabled(true);
	if(renderSettings())
		setClearColor(ColorA(renderSettings()->backgroundColor(), 0));
	else
		setClearColor(ColorA(0, 0, 0, 0));
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool OffscreenOpenGLSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	OVITO_ASSERT(frameBuffer != nullptr);

	// Let the base class do the main rendering work.
	if(!OpenGLSceneRenderer::renderFrame(frameBuffer, stereoTask, std::move(operation)))
		return false;

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
	if(renderingSuccessful && frameBuffer) {

		// Flush the contents to the FBO before extracting image.
		glcontext()->swapBuffers(_offscreenSurface);

		// Fetch rendered image from OpenGL framebuffer.
		QImage renderedImage = _framebufferObject->toImage();
		// We need it in ARGB32 format for best results.
		renderedImage.reinterpretAsFormat(QImage::Format_ARGB32);
		// Rescale supersampled image to final size.
		QImage scaledImage = renderedImage.scaled(_outputSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

		// Transfer OpenGL image to the output frame buffer.
		if(!frameBuffer->image().isNull()) {
			QPainter painter(&frameBuffer->image());
			painter.drawImage(painter.window(), scaledImage);
		}
		else {
			frameBuffer->image() = scaledImage;
		}
		frameBuffer->update();
	}

	OpenGLSceneRenderer::endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::endRender()
{
	OpenGLSceneRenderer::endRender();

	// Release OpenGL resources.
	QOpenGLFramebufferObject::bindDefault();
	if(_offscreenContext && _offscreenContext.data() == QOpenGLContext::currentContext())
		_offscreenContext->doneCurrent();
	_framebufferObject.reset();
	_offscreenContext.reset();
	// Keep offscreen surface alive an re-use it in subsequent render passes until the renderer is deleted.
}

}	// End of namespace
