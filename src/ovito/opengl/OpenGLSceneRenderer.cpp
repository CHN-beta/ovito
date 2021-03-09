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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	#include <QOpenGLVersionFunctionsFactory>
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

/// Indicates whether the OpenGL implementation supports geometry shader programs.
bool OpenGLSceneRenderer::_openglSupportsGeomShaders = false;

/******************************************************************************
* Is called by OVITO to query the class for any information that should be 
* included in the application's system report.
******************************************************************************/
void OpenGLSceneRenderer::OOMetaClass::querySystemInformation(QTextStream& stream) const
{
	if(this == &OpenGLSceneRenderer::OOClass()) {
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
		stream << "Geometry shaders supported: " << (OpenGLSceneRenderer::geometryShadersSupported() ? "yes" : "no") << "\n";
		stream << "Using deprecated functions: " << (format.testOption(QSurfaceFormat::DeprecatedFunctions) ? "yes" : "no") << "\n";
		stream << "Using geometry shaders: " << (OpenGLSceneRenderer::geometryShadersEnabled() ? "yes" : "no") << "\n";
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
	_openglSupportsGeomShaders = QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);
	_openglSurfaceFormat = QOpenGLContext::currentContext()->format();
}

/******************************************************************************
* Determines whether OpenGL geometry shader programs should be used or not.
******************************************************************************/
bool OpenGLSceneRenderer::geometryShadersEnabled(bool forceDefaultSetting)
{
#ifdef Q_OS_WASM
	// Completely disable support for geometry shaders on WebAssembly platform for now. 
	return false;
#endif

#if 0
	if(!forceDefaultSetting) {
		// The user can override the use of geometry shaders.
		QVariant userSetting = QSettings().value("display/use_geometry_shaders");
		if(userSetting.isValid())
			return userSetting.toBool() && geometryShadersSupported();
	}
#endif

	if(Application::instance()->guiMode())
		return geometryShadersSupported();
	else if(QOpenGLContext::currentContext())
		return QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);
	else
		return false;
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
void OpenGLSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);

	if(Application::instance()->headlessMode())
		throwException(tr("Cannot use OpenGL renderer in headless mode."));

	// Get the GL context being used for the current rendering pass.
	_glcontext = QOpenGLContext::currentContext();
	if(!_glcontext)
		throwException(tr("Cannot render scene: There is no active OpenGL context"));
	_glcontextGroup = _glcontext->shareGroup();
	_glsurface = _glcontext->surface();
	OVITO_ASSERT(_glsurface != nullptr);

	// Prepare a functions table allowing us to call OpenGL functions in a platform-independent way.
    initializeOpenGLFunctions();
    OVITO_REPORT_OPENGL_ERRORS(this);

	// Obtain surface format.
    OVITO_REPORT_OPENGL_ERRORS(this);
	_glformat = _glcontext->format();

#ifndef Q_OS_WASM
	if(!glcontext()->isOpenGLES()) {
		// Obtain a functions object that allows to call OpenGL 3.0 functions in a platform-independent way.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		_glFunctions30 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_0>(_glcontext);
#else
		_glFunctions30 = _glcontext->versionFunctions<QOpenGLFunctions_3_0>();
		if(!_glFunctions30 || !_glFunctions30->initializeOpenGLFunctions())
			_glFunctions30 = nullptr;
#endif

		// Obtain a functions object that allows to call OpenGL 3.2 core functions in a platform-independent way.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		_glFunctions32 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(_glcontext);
#else
		_glFunctions32 = _glcontext->versionFunctions<QOpenGLFunctions_3_2_Core>();
		if(!_glFunctions32 || !_glFunctions32->initializeOpenGLFunctions())
			_glFunctions32 = nullptr;
#endif

		if(!_glFunctions30 && !_glFunctions32)
			throwException(tr("Could not resolve OpenGL functions. Invalid OpenGL context."));
	}
#endif

	// Determine whether its okay to use geometry shaders.
	_useGeometryShaders = geometryShadersEnabled() && QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry);

	// Set up a vertex array object (VAO). An active VAO is required during rendering according to the OpenGL core profile.
	if(glformat().majorVersion() >= 3) {
		_vertexArrayObject.reset(new QOpenGLVertexArrayObject());
		OVITO_CHECK_OPENGL(this, _vertexArrayObject->create());
		OVITO_CHECK_OPENGL(this, _vertexArrayObject->bind());
	}
    OVITO_REPORT_OPENGL_ERRORS(this);

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

    if(viewport() && viewport()->window()) {
		
		// Set up OpenGL render viewport to cover the entire window by default.
    	QSize vpSize = viewport()->windowSize();
    	setRenderingViewport(0, 0, vpSize.width(), vpSize.height());

		// When rendering an interactive viewport, use viewport background color to clear frame buffer.
		if(isInteractive()) {
			if(!viewport()->renderPreviewMode())
				setClearColor(Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG));
			else if(renderSettings())
				setClearColor(renderSettings()->backgroundColor());
		}
    }
    OVITO_REPORT_OPENGL_ERRORS(this);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void OpenGLSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
	if(QOpenGLContext::currentContext()) {
	    initializeOpenGLFunctions();
	    OVITO_REPORT_OPENGL_ERRORS(this);
	}
	_vertexArrayObject.reset();
	_glcontext = nullptr;

	SceneRenderer::endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool OpenGLSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
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

		// Call subclass to render additional content that is only visible in the interactive viewports.
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
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLLinePrimitive>(this);
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
		ParticlePrimitive::ShadingMode shadingMode, ParticlePrimitive::RenderingQuality renderingQuality, ParticlePrimitive::ParticleShape shape)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLParticlePrimitive>(this, shadingMode, renderingQuality, shape);
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
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLTextPrimitive>(this);
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
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLImagePrimitive>(this);
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
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLCylinderPrimitive>(this, shape, shadingMode, renderingQuality);
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
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLMarkerPrimitive>(this, shape);
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
	OVITO_ASSERT(glcontext() != nullptr);
	OVITO_ASSERT(QOpenGLContext::currentContext() == glcontext());
	return std::make_shared<OpenGLMeshPrimitive>(this);
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
	QOpenGLContextGroup* contextGroup = glcontext()->shareGroup();
	OVITO_ASSERT(contextGroup == QOpenGLContextGroup::currentContextGroup());

	OVITO_ASSERT(QOpenGLShaderProgram::hasOpenGLShaderPrograms());
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Vertex));
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Fragment));

	// Each OpenGL shader is only created once per OpenGL context group.
	QScopedPointer<QOpenGLShaderProgram> program(contextGroup->findChild<QOpenGLShaderProgram*>(id));
	if(program)
		return program.take();

	// The program's source code hasn't been compiled so far. Do it now and cache the shader program.
	program.reset(new QOpenGLShaderProgram(contextGroup));
	program->setObjectName(id);

	// Load and compile vertex shader source.
	loadShader(program.data(), QOpenGLShader::Vertex, vertexShaderFile);

	// Load and compile fragment shader source.
	loadShader(program.data(), QOpenGLShader::Fragment, fragmentShaderFile);

	// Load and compile geometry shader source.
	if(!geometryShaderFile.isEmpty()) {
		OVITO_ASSERT(useGeometryShaders());
		loadShader(program.data(), QOpenGLShader::Geometry, geometryShaderFile);
	}

	// Compile the shader program.
	if(!program->link()) {
		Exception ex(QString("The OpenGL shader program %1 failed to link.").arg(id));
		ex.appendDetailMessage(program->log());
		throw ex;
	}

	OVITO_REPORT_OPENGL_ERRORS(this);

	return program.take();
}

/******************************************************************************
* Loads and compiles a GLSL shader and adds it to the given program object.
******************************************************************************/
void OpenGLSceneRenderer::loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename)
{
	QByteArray shaderSource;

	// Insert GLSL version string at the top.
	// Pick GLSL language version based on current OpenGL version.
	if(!glcontext()->isOpenGLES()) {

		// Inject GLSL version directive into shader source. 
		// Note: Use GLSL 1.50 when running on a OpenGL 3.2+ platform.
		if(shaderType == QOpenGLShader::Geometry || (glformat().majorVersion() >= 3 && glformat().minorVersion() >= 2) || glformat().majorVersion() > 3)
			shaderSource.append("#version 150\n");
		else
			shaderSource.append("#version 130\n");

		// Inject utilities code file, which is shared by all shaders.
		QFile utilitiesSourceFile(QStringLiteral(":/openglrenderer/glsl/utilities.glsl"));
		if(!utilitiesSourceFile.open(QFile::ReadOnly))
			throw Exception(QString("Unable to open shader source file %1.").arg(utilitiesSourceFile.fileName()));
		shaderSource.append(utilitiesSourceFile.readAll());
	}
	else {
		// Using OpenGL ES context.

		// Inject GLSL version directive into shader source. 
		if(glformat().majorVersion() >= 3)
			shaderSource.append("#version 300 es\n");

	}

	// Load actual shader source code.
	QFile shaderSourceFile(filename);
	if(!shaderSourceFile.open(QFile::ReadOnly))
		throw Exception(QString("Unable to open shader source file %1.").arg(filename));
	shaderSource.append(shaderSourceFile.readAll());

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
* Renders a 2d polyline in the viewport.
******************************************************************************/
void OpenGLSceneRenderer::render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed)
{
	if(isBoundingBoxPass())
		return;

	makeContextCurrent();

	// Load OpenGL shader.
	QOpenGLShaderProgram* shader = loadShaderProgram("line", ":/openglrenderer/glsl/lines/line.vs", ":/openglrenderer/glsl/lines/line.fs");
	if(!shader->bind())
		throwException(tr("Failed to bind OpenGL shader."));

	bool wasDepthTestEnabled = this->glIsEnabled(GL_DEPTH_TEST);
	OVITO_CHECK_OPENGL(this, this->glDisable(GL_DEPTH_TEST));

	GLint vc[4];
	this->glGetIntegerv(GL_VIEWPORT, vc);
	QMatrix4x4 tm;
	tm.ortho(vc[0], vc[0] + vc[2], vc[1] + vc[3], vc[1], -1, 1);
	OVITO_CHECK_OPENGL(this, shader->setUniformValue("modelview_projection_matrix", tm));

	OpenGLBuffer<Point_2<float>> vertexBuffer;
	OpenGLBuffer<ColorAT<float>> colorBuffer;
	vertexBuffer.create(QOpenGLBuffer::StaticDraw, count);
	vertexBuffer.fill(points);
	vertexBuffer.bind(this, shader, "position", GL_FLOAT, 0, 2);
	colorBuffer.create(QOpenGLBuffer::StaticDraw, count);
	colorBuffer.fillConstant(color);
	OVITO_CHECK_OPENGL(this, colorBuffer.bindColors(this, shader, 4));

	OVITO_CHECK_OPENGL(this, glDrawArrays(closed ? GL_LINE_LOOP : GL_LINE_STRIP, 0, count));

	vertexBuffer.detach(this, shader, "position");
	colorBuffer.detachColors(this, shader);
	shader->release();
	if(wasDepthTestEnabled) 
		OVITO_CHECK_OPENGL(this, this->glEnable(GL_DEPTH_TEST));
}

/******************************************************************************
* Sets the frame buffer background color.
******************************************************************************/
void OpenGLSceneRenderer::setClearColor(const ColorA& color)
{
	OVITO_CHECK_OPENGL(this, this->glClearColor(color.r(), color.g(), color.b(), color.a()));
}

/******************************************************************************
* Sets the rendering region in the frame buffer.
******************************************************************************/
void OpenGLSceneRenderer::setRenderingViewport(int x, int y, int width, int height)
{
	OVITO_CHECK_OPENGL(this, this->glViewport(x, y, width, height));
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
