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
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "OpenGLSceneRenderer.h"
#include "OpenGLLinePrimitive.h"
#include "OpenGLParticlePrimitive.h"
#include "OpenGLTextPrimitive.h"
#include "OpenGLImagePrimitive.h"
#include "OpenGLCylinderPrimitive.h"
#include "OpenGLMeshPrimitive.h"
#include "OpenGLMarkerPrimitive.h"
#include "OpenGLHelpers.h"

#include <QOffscreenSurface>
#include <QSurface>
#include <QWindow>
#include <QScreen>
#ifdef OVITO_DEBUG
	#include <QOpenGLDebugLogger>
#endif

namespace Ovito {

IMPLEMENT_OVITO_CLASS(OpenGLSceneRenderer);

/// The vendor of the OpenGL implementation in use.
QByteArray OpenGLSceneRenderer::_openGLVendor;

/// The renderer name of the OpenGL implementation in use.
QByteArray OpenGLSceneRenderer::_openGLRenderer;

/// The version string of the OpenGL implementation in use.
QByteArray OpenGLSceneRenderer::_openGLVersion;

/// The version of the OpenGL shading language supported by the system.
QByteArray OpenGLSceneRenderer::_openGLSLVersion;

/// The current surface format used by the OpenGL implementation.
QSurfaceFormat OpenGLSceneRenderer::_openglSurfaceFormat;

/// The list of extensions supported by the OpenGL implementation.
QSet<QByteArray> OpenGLSceneRenderer::_openglExtensions;

/// Indicates whether the OpenGL implementation supports geometry shaders.
bool OpenGLSceneRenderer::_openGLSupportsGeometryShaders = false;

/******************************************************************************
* Is called by OVITO to query the class for any information that should be 
* included in the application's system report.
******************************************************************************/
void OpenGLSceneRenderer::OOMetaClass::querySystemInformation(QTextStream& stream, DataSetContainer& container) const
{
	if(this == &OpenGLSceneRenderer::OOClass()) {
		OpenGLSceneRenderer::determineOpenGLInfo();

		stream << "======= OpenGL info =======" << "\n";
		const QSurfaceFormat& format = OpenGLSceneRenderer::openglSurfaceFormat();
		stream << "Version: " << format.majorVersion() << QStringLiteral(".") << format.minorVersion() << "\n";
		stream << "Profile: " << (format.profile() == QSurfaceFormat::CoreProfile ? "core" : (format.profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "none")) << "\n";
		stream << "Alpha: " << format.hasAlpha() << "\n";
		stream << "Vendor: " << OpenGLSceneRenderer::openGLVendor() << "\n";
		stream << "Renderer: " << OpenGLSceneRenderer::openGLRenderer() << "\n";
		stream << "Version string: " << OpenGLSceneRenderer::openGLVersion() << "\n";
		stream << "Swap behavior: " << (format.swapBehavior() == QSurfaceFormat::SingleBuffer ? QStringLiteral("single buffer") : (format.swapBehavior() == QSurfaceFormat::DoubleBuffer ? QStringLiteral("double buffer") : (format.swapBehavior() == QSurfaceFormat::TripleBuffer ? QStringLiteral("triple buffer") : QStringLiteral("other")))) << "\n";
		stream << "Depth buffer size: " << format.depthBufferSize() << "\n";
		stream << "Stencil buffer size: " << format.stencilBufferSize() << "\n";
		stream << "Shading language: " << OpenGLSceneRenderer::openGLSLVersion() << "\n";
		stream << "Deprecated functions: " << (format.testOption(QSurfaceFormat::DeprecatedFunctions) ? "yes" : "no") << "\n";
		stream << "Geometry shader support: " << (OpenGLSceneRenderer::openGLSupportsGeometryShaders() ? "yes" : "no") << "\n";
		stream << "Supported extensions:\n";
		QStringList extensionList;
		for(const QByteArray& extension : OpenGLSceneRenderer::openglExtensions())
			extensionList << extension;
		extensionList.sort();
		for(const QString& extension : extensionList)
			stream << extension << "\n";
	}
}

/******************************************************************************
* Determines the capabilities of the current OpenGL implementation.
******************************************************************************/
void OpenGLSceneRenderer::determineOpenGLInfo()
{
	if(!_openGLVendor.isEmpty())
		return;		// Already done.

	// Create a temporary GL context and an offscreen surface if necessary.
	QOpenGLContext tempContext;
	QOffscreenSurface offscreenSurface;
	std::unique_ptr<QWindow> window;
	if(QOpenGLContext::currentContext() == nullptr) {
		if(!tempContext.create())
			throw Exception(tr("Failed to create an OpenGL context. Please check your graphics driver installation to make sure your system supports OpenGL applications. "
								"Sometimes this may only be a temporary error after an automatic operating system update was installed in the background. In this case, simply rebooting your computer can help."));
		if(Application::instance()->headlessMode() == false) {
			// Create a hidden, temporary window to make the GL context current.
			window.reset(new QWindow());
			window->setSurfaceType(QSurface::OpenGLSurface);
			window->setFormat(tempContext.format());
			window->create();
			if(!tempContext.makeCurrent(window.get()))
				throw Exception(tr("Failed to make OpenGL context current. Cannot query OpenGL information."));
		}
		else {
			// Create temporary offscreen buffer to make GL context current.
			offscreenSurface.setFormat(tempContext.format());
			offscreenSurface.create();
			if(!offscreenSurface.isValid())
				throw Exception(tr("Failed to create temporary offscreen rendering surface. Cannot query OpenGL information."));
			if(!tempContext.makeCurrent(&offscreenSurface))
				throw Exception(tr("Failed to make OpenGL context current on offscreen rendering surface. Cannot query OpenGL information."));
		}
		OVITO_ASSERT(QOpenGLContext::currentContext() == &tempContext);
	}

	_openGLVendor = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_VENDOR));
	_openGLRenderer = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_RENDERER));
	_openGLVersion = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_VERSION));
	_openGLSLVersion = reinterpret_cast<const char*>(tempContext.functions()->glGetString(GL_SHADING_LANGUAGE_VERSION));
	_openglSurfaceFormat = QOpenGLContext::currentContext()->format();
	_openglExtensions = tempContext.extensions();
	_openGLSupportsGeometryShaders = QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);
}

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with
* the given other renderer.
******************************************************************************/
bool OpenGLSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	// Two OpenGL renderers are compatible and share resources if they use the same QOpenGLContextGroup.
	if(OpenGLSceneRenderer* otherGLRenderer = dynamic_object_cast<OpenGLSceneRenderer>(otherRenderer)) {
		if(!_glcontextGroup.isNull())
			return _glcontextGroup == otherGLRenderer->_glcontextGroup;
	}
	return false;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void OpenGLSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect)
{
	// Convert viewport rect from logical device coordinates to OpenGL framebuffer coordinates.
	QRect openGLViewportRect(viewportRect.x() * antialiasingLevel(), viewportRect.y() * antialiasingLevel(), viewportRect.width() * antialiasingLevel(), viewportRect.height() * antialiasingLevel());

	SceneRenderer::beginFrame(time, params, vp, openGLViewportRect);

	if(Application::instance()->headlessMode())
		throwException(tr("Cannot use OpenGL renderer in headless mode."));

	// Get the GL context being used for the current rendering pass.
	_glcontext = QOpenGLContext::currentContext();
	if(!_glcontext)
		throwException(tr("Cannot render scene: There is no active OpenGL context"));
	_glcontextGroup = _glcontext->shareGroup();
	_glsurface = _glcontext->surface();
	OVITO_ASSERT(_glsurface != nullptr);

	// Check OpenGL version.
	if(_glcontext->format().majorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MAJOR || (_glcontext->format().majorVersion() == OVITO_OPENGL_MINIMUM_VERSION_MAJOR && _glcontext->format().minorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MINOR)) {
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
    OVITO_REPORT_OPENGL_ERRORS(this);

	// Obtain surface format.
    OVITO_REPORT_OPENGL_ERRORS(this);
	_glformat = _glcontext->format();

	// Get the OpenGL version.
	_glversion = QT_VERSION_CHECK(_glformat.majorVersion(), _glformat.minorVersion(), 0);

#ifdef OVITO_DEBUG
//	_glversion = QT_VERSION_CHECK(4, 1, 0);
//	_glversion = QT_VERSION_CHECK(3, 2, 0);
//	_glversion = QT_VERSION_CHECK(3, 1, 0);
//	_glversion = QT_VERSION_CHECK(2, 1, 0);

	// Initialize debug logger.
	if(_glformat.testOption(QSurfaceFormat::DebugContext)) {
		QOpenGLDebugLogger* logger = findChild<QOpenGLDebugLogger*>();
		if(!logger) {
			logger = new QOpenGLDebugLogger(this);
			connect(logger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage& debugMessage) {
				qDebug() << debugMessage;
			});
		}
		logger->initialize();
		logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
		logger->enableMessages();
	}
#endif

	// Get optional function pointers.
	glMultiDrawArrays = reinterpret_cast<void (QOPENGLF_APIENTRY *)(GLenum, const GLint*, const GLsizei*, GLsizei)>(_glcontext->getProcAddress("glMultiDrawArrays"));
	glMultiDrawArraysIndirect = reinterpret_cast<void (QOPENGLF_APIENTRY *)(GLenum, const void*, GLsizei, GLsizei)>(_glcontext->getProcAddress("glMultiDrawArraysIndirect"));
#ifndef Q_OS_WASM
	OVITO_ASSERT(glMultiDrawArrays); // glMultiDrawArrays() should always be available in desktop OpenGL 2.0+.
#endif

	// Set up a vertex array object (VAO). An active VAO is required during rendering according to the OpenGL core profile.
	if(glformat().majorVersion() >= 3) {
		_vertexArrayObject = std::make_unique<QOpenGLVertexArrayObject>();
		OVITO_CHECK_OPENGL(this, _vertexArrayObject->create());
		OVITO_CHECK_OPENGL(this, _vertexArrayObject->bind());
	}
    OVITO_REPORT_OPENGL_ERRORS(this);

	// Make sure we have a valid frame set for the resource manager during this render pass.
	OVITO_ASSERT(_currentResourceFrame != 0);

	// Reset OpenGL state.
	initializeGLState();

	// Clear background.
	clearFrameBuffer();
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Puts the GL context into its default initial state before rendering
* a frame begins.
******************************************************************************/
void OpenGLSceneRenderer::initializeGLState()
{
	// Set up OpenGL state.
    OVITO_CHECK_OPENGL(this, this->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
	OVITO_CHECK_OPENGL(this, this->glDisable(GL_STENCIL_TEST));
	OVITO_CHECK_OPENGL(this, this->glDisable(GL_BLEND));
	OVITO_CHECK_OPENGL(this, this->glEnable(GL_DEPTH_TEST));
	OVITO_CHECK_OPENGL(this, this->glDepthFunc(GL_LESS));
	OVITO_CHECK_OPENGL(this, this->glDepthRangef(0, 1));
	OVITO_CHECK_OPENGL(this, this->glClearDepthf(1));
	OVITO_CHECK_OPENGL(this, this->glDepthMask(GL_TRUE));
	OVITO_CHECK_OPENGL(this, this->glDisable(GL_SCISSOR_TEST));
	setClearColor(ColorA(0, 0, 0, 0));

	// Set up OpenGL render viewport.
	OVITO_CHECK_OPENGL(this, this->glViewport(viewportRect().x(), viewportRect().y(), viewportRect().width(), viewportRect().height()));

    if(viewport() && viewport()->window()) {
		// When rendering an interactive viewport, use viewport background color to clear frame buffer.
		if(isInteractive() && !isPicking()) {
			if(!viewport()->renderPreviewMode())
				setClearColor(Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG));
			else if(renderSettings())
				setClearColor(renderSettings()->backgroundColor());
		}
    }
	else {
		if(renderSettings() && !isPicking())
			setClearColor(ColorA(renderSettings()->backgroundColor(), 0));
	}
    OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void OpenGLSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer, const QRect& viewportRect)
{
	if(QOpenGLContext::currentContext()) {
	    initializeOpenGLFunctions();
	    OVITO_REPORT_OPENGL_ERRORS(this);
	}
#ifdef OVITO_DEBUG
	// Stop debug logger.
	if(QOpenGLDebugLogger* logger = findChild<QOpenGLDebugLogger*>()) {
		logger->stopLogging();
	}
#endif
	_vertexArrayObject.reset();
	_glcontext = nullptr;

	// Convert viewport rect from logical device coordinates to OpenGL framebuffer coordinates.
	QRect openGLViewportRect(viewportRect.x() * antialiasingLevel(), viewportRect.y() * antialiasingLevel(), viewportRect.width() * antialiasingLevel(), viewportRect.height() * antialiasingLevel());

	SceneRenderer::endFrame(renderingSuccessful, frameBuffer, openGLViewportRect);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool OpenGLSceneRenderer::renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	OVITO_ASSERT(_glcontext == QOpenGLContext::currentContext());
    OVITO_REPORT_OPENGL_ERRORS(this);

	// Set up poor man's stereosopic rendering using red/green filtering.
	if(stereoTask == StereoscopicLeft) {
		OVITO_CHECK_OPENGL(this, this->glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE));
	}
	else if(stereoTask == StereoscopicRight) {
		OVITO_CHECK_OPENGL(this, this->glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE));
	}

	// Render the 3D scene objects.
	if(renderScene(operation.subOperation())) {
		OVITO_REPORT_OPENGL_ERRORS(this);

		// Render additional content that is only visible in the interactive viewports.
		renderInteractiveContent();
		OVITO_REPORT_OPENGL_ERRORS(this);

		// Render translucent objects in a second pass.
		rebindVAO();
		for(auto& deferredPrimitive : _translucentParticles) {
			setWorldTransform(std::get<0>(deferredPrimitive));
			static_cast<OpenGLParticlePrimitive&>(*std::get<1>(deferredPrimitive)).render(this);
		}
		for(auto& deferredPrimitive : _translucentCylinders) {
			setWorldTransform(std::get<0>(deferredPrimitive));
			static_cast<OpenGLCylinderPrimitive&>(*std::get<1>(deferredPrimitive)).render(this);
		}
		for(auto& deferredPrimitive : _translucentMeshes) {
			setWorldTransform(std::get<0>(deferredPrimitive));
			static_cast<OpenGLMeshPrimitive&>(*std::get<1>(deferredPrimitive)).render(this);
		}
		_translucentParticles.clear();
		_translucentCylinders.clear();
		_translucentMeshes.clear();
	}

	// Restore default OpenGL state.
	OVITO_CHECK_OPENGL(this, this->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

	return !operation.isCanceled();
}

/******************************************************************************
* Makes the renderer's GL context current.
******************************************************************************/
void OpenGLSceneRenderer::makeContextCurrent()
{
#ifndef Q_OS_WASM
	OVITO_ASSERT(glcontext());
	if(!glcontext()->makeCurrent(_glsurface))
		throwException(tr("Failed to make OpenGL context current."));
#endif		
}

/******************************************************************************
* Translates an OpenGL error code to a human-readable message string.
******************************************************************************/
const char* OpenGLSceneRenderer::openglErrorString(GLenum errorCode)
{
	switch(errorCode) {
	case GL_NO_ERROR: return "GL_NO_ERROR - No error has been recorded.";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument.";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE - A numeric argument is out of range.";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION - The specified operation is not allowed in the current state.";
	case 0x0503 /*GL_STACK_OVERFLOW*/: return "GL_STACK_OVERFLOW - This command would cause a stack overflow.";
	case 0x0504 /*GL_STACK_UNDERFLOW*/: return "GL_STACK_UNDERFLOW - This command would cause a stack underflow.";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY - There is not enough memory left to execute the command.";
	case 0x8031 /*GL_TABLE_TOO_LARGE*/: return "GL_TABLE_TOO_LARGE - The specified table exceeds the implementation's maximum supported table size.";
	default: return "Unknown OpenGL error code.";
	}
}

/******************************************************************************
* Requests a new line geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<LinePrimitive> OpenGLSceneRenderer::createLinePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLLinePrimitive>();
}

/******************************************************************************
* Renders the line primitives stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderLines(const std::shared_ptr<LinePrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLLinePrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	static_cast<OpenGLLinePrimitive&>(*primitive).render(this);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Requests a new particle geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<ParticlePrimitive> OpenGLSceneRenderer::createParticlePrimitive(
		ParticlePrimitive::ParticleShape shape, ParticlePrimitive::ShadingMode shadingMode, ParticlePrimitive::RenderingQuality renderingQuality)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLParticlePrimitive>(shape, shadingMode, renderingQuality);
}

/******************************************************************************
* Renders the particles stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderParticles(const std::shared_ptr<ParticlePrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLParticlePrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	// Render particles immediately if they are all fully opaque. Otherwise defer rendering to a later time.
	if(isPicking() || !primitive->transparencies())
		static_cast<OpenGLParticlePrimitive&>(*primitive).render(this);
	else
		_translucentParticles.emplace_back(worldTransform(), primitive);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Requests a new text geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<TextPrimitive> OpenGLSceneRenderer::createTextPrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLTextPrimitive>();
}

/******************************************************************************
* Renders the text stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderText(const std::shared_ptr<TextPrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLTextPrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	static_cast<OpenGLTextPrimitive&>(*primitive).render(this);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Requests a new image geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<ImagePrimitive> OpenGLSceneRenderer::createImagePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLImagePrimitive>();
}

/******************************************************************************
* Renders the 2d image stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderImage(const std::shared_ptr<ImagePrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLImagePrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	static_cast<OpenGLImagePrimitive&>(*primitive).render(this);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Requests a new cylinder geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<CylinderPrimitive> OpenGLSceneRenderer::createCylinderPrimitive(
		CylinderPrimitive::Shape shape,
		CylinderPrimitive::ShadingMode shadingMode,
		CylinderPrimitive::RenderingQuality renderingQuality)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLCylinderPrimitive>(shape, shadingMode, renderingQuality);
}

/******************************************************************************
* Renders the cylinders stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderCylinders(const std::shared_ptr<CylinderPrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLCylinderPrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	// Render primitives immediately if they are all fully opaque. Otherwise defer rendering to a later time.
	if(isPicking() || !primitive->transparencies())
		static_cast<OpenGLCylinderPrimitive&>(*primitive).render(this);
	else
		_translucentCylinders.emplace_back(worldTransform(), primitive);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Requests a new marker geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<MarkerPrimitive> OpenGLSceneRenderer::createMarkerPrimitive(MarkerPrimitive::MarkerShape shape)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLMarkerPrimitive>(shape);
}

/******************************************************************************
* Renders the markers stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderMarkers(const std::shared_ptr<MarkerPrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLMarkerPrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	static_cast<OpenGLMarkerPrimitive&>(*primitive).render(this);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Requests a new triangle mesh buffer from the renderer.
******************************************************************************/
std::shared_ptr<MeshPrimitive> OpenGLSceneRenderer::createMeshPrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<OpenGLMeshPrimitive>();
}

/******************************************************************************
* Renders the triangle mesh stored in the given buffer.
******************************************************************************/
void OpenGLSceneRenderer::renderMesh(const std::shared_ptr<MeshPrimitive>& primitive)
{
	OVITO_REPORT_OPENGL_ERRORS(this);
	OVITO_ASSERT(dynamic_cast<OpenGLMeshPrimitive*>(primitive.get()) != nullptr);
	rebindVAO();
	// Render mesh immediately if it is fully opaque. Otherwise defer rendering to a later time.
	if(isPicking() || primitive->isFullyOpaque())
		static_cast<OpenGLMeshPrimitive&>(*primitive).render(this);
	else
		_translucentMeshes.emplace_back(worldTransform(), primitive);
	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Loads an OpenGL shader program.
******************************************************************************/
QOpenGLShaderProgram* OpenGLSceneRenderer::loadShaderProgram(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile)
{
	QOpenGLContextGroup* contextGroup = QOpenGLContextGroup::currentContextGroup();
	OVITO_ASSERT(contextGroup);

	OVITO_ASSERT(QThread::currentThread() == contextGroup->thread());
	OVITO_ASSERT(QOpenGLShaderProgram::hasOpenGLShaderPrograms());
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Vertex));
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Fragment));

	// Each OpenGL shader is only created once per OpenGL context group.
	std::unique_ptr<QOpenGLShaderProgram> program(contextGroup->findChild<QOpenGLShaderProgram*>(id));
	if(program)
		return program.release();

	// The program's source code hasn't been compiled so far. Do it now and cache the shader program.
	program = std::make_unique<QOpenGLShaderProgram>();
	program->setObjectName(id);

	// Load and compile vertex shader source.
	loadShader(program.get(), QOpenGLShader::Vertex, vertexShaderFile);

	// Load and compile fragment shader source.
	loadShader(program.get(), QOpenGLShader::Fragment, fragmentShaderFile);

	// Load and compile geometry shader source.
	if(!geometryShaderFile.isEmpty()) {
		loadShader(program.get(), QOpenGLShader::Geometry, geometryShaderFile);
	}

	// Make the shader program a child object of the GL context group.
	program->setParent(contextGroup);
	OVITO_ASSERT(contextGroup->findChild<QOpenGLShaderProgram*>(id));

	// Compile the shader program.
	if(!program->link()) {
		Exception ex(QString("The OpenGL shader program %1 failed to link.").arg(id));
		ex.appendDetailMessage(program->log());
		throw ex;
	}

	OVITO_REPORT_OPENGL_ERRORS(this);

	return program.release();
}

/******************************************************************************
* Loads and compiles a GLSL shader and adds it to the given program object.
******************************************************************************/
void OpenGLSceneRenderer::loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename)
{
	QByteArray shaderSource;
	bool isGLES = QOpenGLContext::currentContext()->isOpenGLES();

	// Insert GLSL version string at the top.
	// Pick GLSL language version based on current OpenGL version.
	if(!isGLES) {
		// Inject GLSL version directive into shader source. 
		// Note: Use GLSL 1.50 when running on a OpenGL 3.2+ platform.
		if(shaderType == QOpenGLShader::Geometry || _glversion >= QT_VERSION_CHECK(3, 2, 0))
			shaderSource.append("#version 150\n");
		else if(_glversion >= QT_VERSION_CHECK(3, 1, 0))
			shaderSource.append("#version 140\n");
		else if(_glversion >= QT_VERSION_CHECK(3, 0, 0))
			shaderSource.append("#version 130\n");
		else
			shaderSource.append("#version 120\n");
	}
	else {
		// Using OpenGL ES context.
		// Inject GLSL version directive into shader source. 
		if(glformat().majorVersion() >= 3) {
			shaderSource.append("#version 300 es\n");
		}
		else {
			shaderSource.append("precision highp float;\n");

			if(shaderType == QOpenGLShader::Fragment) {
				// OpenGL ES 2.0 has no built-in support for gl_FragDepth. 
				// Need to request EXT_frag_depth extension in such a case.
				shaderSource.append("#extension GL_EXT_frag_depth : enable\n");
				// Computation of local normal vectors in fragment shaders requires GLSL 
				// derivative functions dFdx, dFdy.
				shaderSource.append("#extension GL_OES_standard_derivatives : enable\n");
			}

			// Provide replacements of some missing GLSL functions in OpenGL ES Shading Language. 
			shaderSource.append("mat3 transpose(in mat3 tm) {\n");
			shaderSource.append("    vec3 i0 = tm[0];\n");
			shaderSource.append("    vec3 i1 = tm[1];\n");
			shaderSource.append("    vec3 i2 = tm[2];\n");
			shaderSource.append("    mat3 out_tm = mat3(\n");
			shaderSource.append("         vec3(i0.x, i1.x, i2.x),\n");
			shaderSource.append("         vec3(i0.y, i1.y, i2.y),\n");
			shaderSource.append("         vec3(i0.z, i1.z, i2.z));\n");
			shaderSource.append("    return out_tm;\n");
			shaderSource.append("}\n");
		}
	}

	if(_glversion < QT_VERSION_CHECK(3, 0, 0)) {
		// This is needed to emulate the special shader variables 'gl_VertexID' and 'gl_InstanceID' in GLSL 1.20:
		if(shaderType == QOpenGLShader::Vertex) {
			// Note: Data type 'float' is used for the vertex attribute, because some OpenGL implementation have poor support for integer
			// vertex attributes.
			shaderSource.append("attribute float vertexID;\n");
			shaderSource.append("uniform int vertices_per_instance;\n");
		}
	}
	else if(_glversion < QT_VERSION_CHECK(3, 3, 0)) {
		// This is needed to compute the special shader variable 'gl_VertexID' when instanced arrays are not supported:
		if(shaderType == QOpenGLShader::Vertex) {
			shaderSource.append("uniform int vertices_per_instance;\n");
		}
	}

	// Helper function that appends a source code line to the buffer after preprocessing it.
	auto preprocessShaderLine = [&](QByteArray& line) {
		if(_glversion < QT_VERSION_CHECK(3, 0, 0)) {
			// Automatically back-port shader source code to make it compatible with OpenGL 2.1 (GLSL 1.20):
			if(shaderType == QOpenGLShader::Vertex) {
				if(line.startsWith("in ")) line = QByteArrayLiteral("attribute") + line.mid(2);
				else if(line.startsWith("out ")) line = QByteArrayLiteral("varying") + line.mid(3);
				else if(line.startsWith("flat out vec3 flat_normal_fs")) {
					if(!isGLES) return;
					line = QByteArrayLiteral("varying vec3 tex_coords;\n");
				}
				else if(line.startsWith("flat out ")) line = QByteArrayLiteral("varying") + line.mid(8);
				else if(line.startsWith("noperspective out ")) return;
				else if(line.contains("calculate_view_ray(vec2(")) return;
				else if(line.contains("flat_normal_fs")) {
					if(!isGLES) line = "gl_TexCoord[1] = inverse_projection_matrix * gl_Position;\n";
					else line = "tex_coords = (inverse_projection_matrix * gl_Position).xyz;\n";
				}
				else {
					line.replace("gl_VertexID", "int(mod(vertexID+0.5, float(vertices_per_instance)))");
					line.replace("gl_InstanceID", "(int(vertexID) / vertices_per_instance)");
					if(!isGLES) {
						line.replace("float(objectID & 0xFF)", "floor(mod(objectID, 256.0))");
						line.replace("float((objectID >> 8) & 0xFF)", "floor(mod(objectID / 256.0, 256.0))");
						line.replace("float((objectID >> 16) & 0xFF)", "floor(mod(objectID / 65536.0, 256.0))");
						line.replace("float((objectID >> 24) & 0xFF)", "floor(mod(objectID / 16777216.0, 256.0))");
					}
					else {
						line.replace("float(objectID & 0xFF)", "floor(mod(float(objectID), 256.0))");
						line.replace("float((objectID >> 8) & 0xFF)", "floor(mod(float(objectID) / 256.0, 256.0))");
						line.replace("float((objectID >> 16) & 0xFF)", "floor(mod(float(objectID) / 65536.0, 256.0))");
						line.replace("float((objectID >> 24) & 0xFF)", "floor(mod(float(objectID) / 16777216.0, 256.0))");
					}
					line.replace("inverse(mat3(", "inverse_mat3(mat3(");
				}
			}
			else if(shaderType == QOpenGLShader::Fragment) {
				if(line.startsWith("in ")) line = QByteArrayLiteral("varying") + line.mid(2);
				else if(line.startsWith("flat in vec3 flat_normal_fs")) {
					if(!isGLES) return;
					line = QByteArrayLiteral("varying vec3 tex_coords;\n");
				}
				else if(line.startsWith("flat in ")) line = QByteArrayLiteral("varying") + line.mid(7);
				else if(line.startsWith("noperspective in ")) return;
				else if(line.startsWith("out ")) return;
				else if(line.contains("vec3 ray_dir_norm =")) {
					line = QByteArrayLiteral(
						"vec2 viewport_position = ((gl_FragCoord.xy - viewport_origin) * inverse_viewport_size) - 1.0;\n"
						"vec4 _near = inverse_projection_matrix * vec4(viewport_position, -1.0, 1.0);\n"
						"vec4 _far = _near + inverse_projection_matrix[2];\n"
						"vec3 ray_origin = _near.xyz / _near.w;\n"
						"vec3 ray_dir_norm = normalize(_far.xyz / _far.w - ray_origin);\n");
				}
				else {
					line.replace("fragColor", "gl_FragColor");
					line.replace("texture(", "texture2D(");
					line.replace("shadeSurfaceColor(flat_normal_fs", 
						!isGLES ? "shadeSurfaceColor(normalize(cross(dFdx(gl_TexCoord[1].xyz), dFdy(gl_TexCoord[1].xyz)))"
						: "shadeSurfaceColor(normalize(cross(dFdx(tex_coords), dFdy(tex_coords)))");
					line.replace("inverse(view_to_sphere_fs)", "inverse_mat3(view_to_sphere_fs)");
					if(isGLES) {
						if(line.contains("gl_FragDepth")) {
							line.replace("gl_FragDepth", "gl_FragDepthEXT");
							line.prepend(QByteArrayLiteral("#if defined(GL_EXT_frag_depth)\n"));
							line.append(QByteArrayLiteral("#endif\n"));
						}
					}
				}
			}
		}
		else {
			// Automatically back-port shader source code to make it compatible with OpenGL 3.0 - 3.2:
			if(shaderType == QOpenGLShader::Vertex) {
				if(_glversion < QT_VERSION_CHECK(3, 3, 0)) {
					line.replace("gl_VertexID", "(gl_VertexID % vertices_per_instance)");
					line.replace("gl_InstanceID", "(gl_VertexID / vertices_per_instance)");
				}
				if(_glversion < QT_VERSION_CHECK(3, 1, 0))
					line.replace("inverse(mat3(", "inverse_mat3(mat3(");
			}
			else if(shaderType == QOpenGLShader::Fragment) { 
				if(_glversion < QT_VERSION_CHECK(3, 1, 0))
					line.replace("inverse(view_to_sphere_fs)", "inverse_mat3(view_to_sphere_fs)");
			}
		}

		shaderSource.append(line);	
	};

	// Load actual shader source code.
	QFile shaderSourceFile(filename);
	if(!shaderSourceFile.open(QFile::ReadOnly))
		throw Exception(QString("Unable to open shader source file %1.").arg(filename));

	// Parse each line of the shader file and process #include directives.
	while(!shaderSourceFile.atEnd()) {
		QByteArray line = shaderSourceFile.readLine();
		if(line.startsWith("#include")) {
			// Resolve relative file paths.
			QFileInfo includeFile(QFileInfo(shaderSourceFile).dir(), QString::fromUtf8(line.mid(8).replace('\"', "").trimmed()));

			// Load the secondary shader file and insert it into the source of the primary shader.
			QFile secondarySourceFile(includeFile.filePath());
			if(!secondarySourceFile.open(QFile::ReadOnly))
				throw Exception(QString("Unable to open shader source file %1 referenced by include directive in shader file %2.").arg(includeFile.filePath()).arg(filename));
			while(!secondarySourceFile.atEnd()) {
				line = secondarySourceFile.readLine();
				preprocessShaderLine(line);
			}
			shaderSource.append('\n');
		}
		else {
			preprocessShaderLine(line);
		}
	}

	// Load and compile vertex shader source.
	if(!program->addShaderFromSourceCode(shaderType, shaderSource)) {
		Exception ex(QString("The shader source file %1 failed to compile.").arg(filename));
		ex.appendDetailMessage(program->log());
		ex.appendDetailMessage(QStringLiteral("Problematic shader source:"));
		ex.appendDetailMessage(shaderSource);
		throw ex;
	}

	OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* Sets the frame buffer background color.
******************************************************************************/
void OpenGLSceneRenderer::setClearColor(const ColorA& color)
{
	OVITO_CHECK_OPENGL(this, this->glClearColor(color.r(), color.g(), color.b(), color.a()));
}

/******************************************************************************
* Clears the frame buffer contents.
******************************************************************************/
void OpenGLSceneRenderer::clearFrameBuffer(bool clearDepthBuffer, bool clearStencilBuffer)
{
	OVITO_CHECK_OPENGL(this, this->glClear(GL_COLOR_BUFFER_BIT |
			(clearDepthBuffer ? GL_DEPTH_BUFFER_BIT : 0) |
			(clearStencilBuffer ? GL_STENCIL_BUFFER_BIT : 0)));
}

/******************************************************************************
* Temporarily enables/disables the depth test while rendering.
******************************************************************************/
void OpenGLSceneRenderer::setDepthTestEnabled(bool enabled)
{
	if(enabled) {
		OVITO_CHECK_OPENGL(this, this->glEnable(GL_DEPTH_TEST));
	}
	else {
		OVITO_CHECK_OPENGL(this, this->glDisable(GL_DEPTH_TEST));
	}
}

/******************************************************************************
* Activates the special highlight rendering mode.
******************************************************************************/
void OpenGLSceneRenderer::setHighlightMode(int pass)
{
	if(pass == 1) {
		this->glEnable(GL_DEPTH_TEST);
		this->glClearStencil(0);
		this->glClear(GL_STENCIL_BUFFER_BIT);
		this->glEnable(GL_STENCIL_TEST);
		this->glStencilFunc(GL_ALWAYS, 0x1, 0x1);
		this->glStencilMask(0x1);
		this->glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		this->glDepthFunc(GL_LEQUAL);
	}
	else if(pass == 2) {
		this->glDisable(GL_DEPTH_TEST);
		this->glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
		this->glStencilMask(0x1);
		this->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else {
		this->glDepthFunc(GL_LESS);
		this->glEnable(GL_DEPTH_TEST);
		this->glDisable(GL_STENCIL_TEST);
	}
}

/******************************************************************************
* Reports OpenGL error status codes.
******************************************************************************/
void OpenGLSceneRenderer::checkOpenGLErrorStatus(const char* command, const char* sourceFile, int sourceLine)
{
	GLenum error;
	while((error = this->glGetError()) != GL_NO_ERROR) {
		qDebug() << "WARNING: OpenGL call" << command << "failed "
				"in line" << sourceLine << "of file" << sourceFile
				<< "with error" << OpenGLSceneRenderer::openglErrorString(error);
	}
}

}	// End of namespace
