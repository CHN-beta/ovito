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
	// Surface creation can only be performed in the main thread.
	OVITO_ASSERT(QThread::currentThread() == qApp->thread());
	OVITO_ASSERT(!_offscreenContext);
	
	if(!_offscreenSurface)
		_offscreenSurface = new QOffscreenSurface(nullptr, this);
	if(QOpenGLContext::globalShareContext())
		_offscreenSurface->setFormat(QOpenGLContext::globalShareContext()->format());
	else
		_offscreenSurface->setFormat(QSurfaceFormat::defaultFormat());
	_offscreenSurface->create(); 
}

/******************************************************************************
* Prepares the renderer for rendering and sets the dataset that is being rendered.
******************************************************************************/
bool OffscreenOpenGLSceneRenderer::startRender(DataSet* dataset, RenderSettings* settings, const QSize& frameBufferSize)
{
	if(Application::instance()->headlessMode())
		throwRendererException(tr("Cannot use OpenGL renderer when running in headless mode. "
				"Please use a different rendering engine or run program on a machine where access to "
				"graphics hardware is possible."));

	if(!OpenGLSceneRenderer::startRender(dataset, settings, frameBufferSize))
		return false;

	// Create a OpenGL context for rendering to an offscreen buffer.
	_offscreenContext = std::make_unique<QOpenGLContext>();
	// The context should share its resources with interactive viewport renderers (only when operating in the same thread).
	if(QOpenGLContext::globalShareContext() && QThread::currentThread() == QOpenGLContext::globalShareContext()->thread())
		_offscreenContext->setShareContext(QOpenGLContext::globalShareContext());
	if(!_offscreenContext->create())
		throwRendererException(tr("Failed to create OpenGL context for rendering."));

	// Check offscreen surface (creation must have happened in the main thread).
	OVITO_ASSERT(_offscreenSurface);
	if(!_offscreenSurface->isValid())
		throwRendererException(tr("Failed to create offscreen rendering surface."));
	OVITO_ASSERT(_offscreenSurface->format() == _offscreenContext->format());

	// Make the context current.
	if(!_offscreenContext->makeCurrent(_offscreenSurface))
		throwRendererException(tr("Failed to make OpenGL context current."));

	// Determine internal framebuffer size including supersampling.
	_framebufferSize = QSize(frameBufferSize.width() * antialiasingLevel(), frameBufferSize.height() * antialiasingLevel());

	// Create OpenGL framebuffer.
	QOpenGLFramebufferObjectFormat framebufferFormat;
	framebufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	_framebufferObject = std::make_unique<QOpenGLFramebufferObject>(_framebufferSize, framebufferFormat);
	if(!_framebufferObject->isValid())
		throwRendererException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

	// Bind OpenGL buffer.
	if(!_framebufferObject->bind())
		throwRendererException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

	// Tell the base class about the FBO we are rendering into.
	setPrimaryFramebuffer(_framebufferObject->handle());

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect)
{
	// Make GL context current.
	if(!_offscreenContext || !_offscreenContext->makeCurrent(_offscreenSurface))
		throwRendererException(tr("Failed to make OpenGL context current."));

	// Tell the resource manager that we are beginning a new frame.
	OVITO_ASSERT(currentResourceFrame() == 0);
	setCurrentResourceFrame(OpenGLResourceManager::instance()->acquireResourceFrame());

	// Always render into the upper left corner of the OpenGL framebuffer.
	// That's because the OpenGL framebuffer may be smaller than the target OVITO framebuffer.
	QRect shiftedViewportRect = viewportRect;
	shiftedViewportRect.moveTo(0,0);

	OpenGLSceneRenderer::beginFrame(time, params, vp, shiftedViewportRect);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool OffscreenOpenGLSceneRenderer::renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, SynchronousOperation operation)
{
	OVITO_ASSERT(frameBuffer != nullptr);

	// Always render into the upper left corner of the OpenGL framebuffer.
	// That's because the OpenGL framebuffer may be smaller than the target OVITO framebuffer.
	QRect shiftedViewportRect = viewportRect;
	shiftedViewportRect.moveTo(0,0);

	// Let the base class do the main rendering work.
	if(!OpenGLSceneRenderer::renderFrame(frameBuffer, shiftedViewportRect, std::move(operation)))
		return false;

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer, const QRect& viewportRect)
{
	if(renderingSuccessful && frameBuffer) {

		// Flush the contents to the FBO before extracting image.
		glcontext()->swapBuffers(_offscreenSurface);

		// Fetch rendered image from OpenGL framebuffer.
		QImage renderedImage = _framebufferObject->toImage();
		// Rescale supersampled image.
		QSize originalSize(renderedImage.width() / antialiasingLevel(), renderedImage.height() / antialiasingLevel());
		QImage scaledImage = renderedImage.scaled(originalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

		// Transfer OpenGL image to the output frame buffer.
		if(!frameBuffer->image().isNull()) {
			QPainter painter(&frameBuffer->image());
			painter.drawImage(viewportRect, scaledImage, QRect(0, scaledImage.height() - viewportRect.height(), viewportRect.width(), viewportRect.height()));
		}
		else {
			frameBuffer->image() = scaledImage;
		}
		frameBuffer->update(viewportRect);
	}

	// Tell the resource manager that we are done rendering the frame.
	if(_previousResourceFrame) {
		OpenGLResourceManager::instance()->releaseResourceFrame(_previousResourceFrame);
	}
	// Keep the resource from the last frame alive to speed up rendering of successive frames.
	_previousResourceFrame = currentResourceFrame();
	setCurrentResourceFrame(0);

	// Always render into the upper left corner of the OpenGL framebuffer.
	// That's because the OpenGL framebuffer may be smaller than the target OVITO framebuffer.
	QRect shiftedViewportRect = viewportRect;
	shiftedViewportRect.moveTo(0,0);

	OpenGLSceneRenderer::endFrame(renderingSuccessful, frameBuffer, shiftedViewportRect);
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void OffscreenOpenGLSceneRenderer::endRender()
{
	OpenGLSceneRenderer::endRender();

	// Tell the resource manager that we are done rendering the frame.
	if(_previousResourceFrame) {
		OpenGLResourceManager::instance()->releaseResourceFrame(_previousResourceFrame);
		_previousResourceFrame = 0;
	}

	// Release OpenGL resources.
	QOpenGLFramebufferObject::bindDefault();
	if(_offscreenContext && _offscreenContext.get() == QOpenGLContext::currentContext())
		_offscreenContext->doneCurrent();
	_framebufferObject.reset();
	_offscreenContext.reset();
	setPrimaryFramebuffer(0);
	// Keep offscreen surface alive and re-use it in subsequent render passes until the renderer is deleted.
}

}	// End of namespace
