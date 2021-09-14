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
PickingOpenGLSceneRenderer::PickingOpenGLSceneRenderer(DataSet* dataset) : OpenGLSceneRenderer(dataset) 
{
	setPicking(true);
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void PickingOpenGLSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect)
{
	// Get the viewport's window.
	ViewportWindowInterface* vpWindow = vp->window();
	if(!vpWindow)
		throwException(tr("Viewport window has not been created."));
	if(!vpWindow->isVisible())
		throwException(tr("Viewport window is not visible."));

	// Before making our GL context current, remember the old context that
	// is currently active so we can restore it after we are done.
	_oldContext = QOpenGLContext::currentContext();
	_oldSurface = _oldContext ? _oldContext->surface() : nullptr;

	// Get OpenGL context associated with the viewport window.
	vpWindow->makeOpenGLContextCurrent();
	QOpenGLContext* context = QOpenGLContext::currentContext();
	if(!context || !context->isValid())
		throwException(tr("OpenGL context for viewport window has not been created."));

	// Prepare a functions table allowing us to call OpenGL functions in a platform-independent way.
    initializeOpenGLFunctions();
	
	// Size of the viewport window in physical pixels.
	QSize size = vpWindow->viewportWindowDeviceSize();

	if(!context->isOpenGLES() || !context->hasExtension("WEBGL_depth_texture")) {
		// Create OpenGL framebuffer.
		QOpenGLFramebufferObjectFormat framebufferFormat;
		framebufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
		_framebufferObject = std::make_unique<QOpenGLFramebufferObject>(size.width(), size.height(), framebufferFormat);

		// Clear OpenGL error state and verify validity of framebuffer.
		while(context->functions()->glGetError() != GL_NO_ERROR);
		if(!_framebufferObject->isValid())
			throwException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

		// Bind OpenGL framebuffer.
		if(!_framebufferObject->bind())
			throwException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));
	}
	else {
		// When running in a web browser environment which supports the WEBGL_depth_texture extension,
		// create a custom framebuffer with attached color and and depth textures. 

		// Create a texture for storing the color buffer.
		glGenTextures(2, _framebufferTexturesGLES);
		glBindTexture(GL_TEXTURE_2D, _framebufferTexturesGLES[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Create a texture for storing the depth buffer.
		glBindTexture(GL_TEXTURE_2D, _framebufferTexturesGLES[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, size.width(), size.height(), 0, GL_DEPTH_STENCIL, 0x84FA /*=GL_UNSIGNED_INT_24_8_WEBGL*/, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Cleanup
		glBindTexture(GL_TEXTURE_2D, 0);

		// Create a framebuffer and associate the textures with it.
		glGenFramebuffers(1, &_framebufferObjectGLES);
		glBindFramebuffer(GL_FRAMEBUFFER, _framebufferObjectGLES);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _framebufferTexturesGLES[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, _framebufferTexturesGLES[1], 0);

		// Check framebuffer status.
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			throwException(tr("Failed to create OpenGL framebuffer for picking offscreen rendering."));
	}

	OpenGLSceneRenderer::beginFrame(time, params, vp, viewportRect);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void PickingOpenGLSceneRenderer::initializeGLState()
{
	OpenGLSceneRenderer::initializeGLState();
	OVITO_ASSERT(viewport() && viewport()->window());
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool PickingOpenGLSceneRenderer::renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	// Clear previous object records.
	reset();

	// Let the base class do the main rendering work.
	if(!OpenGLSceneRenderer::renderFrame(frameBuffer, viewportRect, stereoTask, std::move(operation)))
		return false;

	// Clear OpenGL error state, so we start fresh for the glReadPixels() call below.
	while(this->glGetError() != GL_NO_ERROR);

	if(_framebufferObject) {
		// Fetch rendered image from the OpenGL framebuffer.
		QSize size = _framebufferObject->size();

#ifndef Q_OS_WASM
		// First, read color buffer.
		_image = QImage(size, QImage::Format_ARGB32);
		// Try GL_BGRA pixel format first. If not supported, use GL_RGBA instead and convert back to GL_BGRA.
		this->glReadPixels(0, 0, size.width(), size.height(), 0x80E1 /*GL_BGRA*/, GL_UNSIGNED_BYTE, _image.bits());
		if(this->glGetError() != GL_NO_ERROR) {
			OVITO_CHECK_OPENGL(this, this->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, _image.bits()));
			_image = std::move(_image).rgbSwapped();
		}

		// Now acquire OpenGL depth buffer data.
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
#else
		_image = _framebufferObject->toImage(false);
#endif
	}
	else {
		// Read color buffer contents, which is used for object picking.
		glFlush();
		QSize size = viewport()->window()->viewportWindowDeviceSize();
		QImage image(size, QImage::Format_ARGB32);
		OVITO_CHECK_OPENGL(this, this->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits()));
		_image = std::move(image).rgbSwapped();

		// Detach textures from framebuffer.
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

		// Delete texture-backed framebuffer object.
		glDeleteFramebuffers(1, &_framebufferObjectGLES);
		_framebufferObjectGLES = 0;

		// Create and bind OpenGL framebuffer.
		QOpenGLFramebufferObjectFormat framebufferFormat;
		_framebufferObject = std::make_unique<QOpenGLFramebufferObject>(size.width(), size.height(), framebufferFormat);

		// Clear OpenGL error state and verify validity of framebuffer.
		while(this->glGetError() != GL_NO_ERROR);
		if(!_framebufferObject->isValid())
			throwException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

		// Bind OpenGL framebuffer.
		if(!_framebufferObject->bind())
			throwException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

		// Reset OpenGL context state.
		this->glDisable(GL_CULL_FACE);
		this->glDisable(GL_STENCIL_TEST);
		this->glDisable(GL_BLEND);
		this->glDisable(GL_DEPTH_TEST);

		// Transfer depth buffer to the color buffer so that the pixel data can be read.
		// WebGL1 doesn't allow to read the data of a depth texture directly.
		OpenGLDepthTextureBlitter blitter;
		blitter.create();
		blitter.bind();
		blitter.blit(_framebufferTexturesGLES[1]);
		blitter.release();

		// Read depth buffer contents from the color attachment of the framebuffer.
		// Depth values are encoded as RGB values in each pixel.
		_depthBufferBits = 24;
		_depthBuffer = std::make_unique<quint8[]>(size.width() * size.height() * sizeof(GLuint));
		OVITO_CHECK_OPENGL(this, this->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, _depthBuffer.get()));

		_framebufferObject.reset();
	}

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void PickingOpenGLSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer, const QRect& viewportRect)
{
	endPickObject();
	if(_framebufferObject) {
		_framebufferObject.reset();
	}
	else {
		// Go back to using the default framebuffer.
		QOpenGLFramebufferObject::bindDefault();
		
		// Delete framebuffer object.
		glDeleteFramebuffers(1, &_framebufferObjectGLES);
		_framebufferObjectGLES = 0;

		// Delete color and depth textures used for offscreen rendering.
		glGenTextures(2, _framebufferTexturesGLES);
		_framebufferTexturesGLES[0] = _framebufferTexturesGLES[1] = 0;
	}
	OpenGLSceneRenderer::endFrame(renderingSuccessful, frameBuffer, viewportRect);

	// Reactivate old GL context.
	if(_oldSurface && _oldContext) {
		_oldContext->makeCurrent(_oldSurface);
	}
	else {
		QOpenGLContext* context = QOpenGLContext::currentContext();
		if(context) context->doneCurrent();
	}
	_oldContext = nullptr;
	_oldSurface = nullptr;
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
	_image = QImage();
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
	if(!_image.isNull()) {
		if(pos.x() >= 0 && pos.x() < _image.width() && pos.y() >= 0 && pos.y() < _image.height()) {
			QPoint mirroredPos(pos.x(), _image.height() - 1 - pos.y());
			QRgb pixel = _image.pixel(mirroredPos);
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
	if(!_image.isNull() && _depthBuffer) {
		int w = _image.width();
		int h = _image.height();
		if(pos.x() >= 0 && pos.x() < w && pos.y() >= 0 && pos.y() < h) {
			QPoint mirroredPos(pos.x(), _image.height() - 1 - pos.y());
			if(_image.pixel(mirroredPos) != 0) {
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
		Point3 ndc(
				(FloatType)pos.x() / _image.width() * FloatType(2) - FloatType(1),
				1.0 - (FloatType)pos.y() / _image.height() * FloatType(2),
				zvalue * FloatType(2) - FloatType(1));
		Point3 worldPos = projParams().inverseViewMatrix * (projParams().inverseProjectionMatrix * ndc);
		return worldPos;
	}
	return Point3::Origin();
}

}	// End of namespace
