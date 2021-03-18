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
#include "OpenGLHelpers.h"

#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

namespace Ovito {

/**
 * \brief An OpenGL-based scene renderer. This serves as base class for both the interactive renderer used
 *        by the viewports and the standard output renderer.
 */
class OVITO_OPENGLRENDERER_EXPORT OpenGLSceneRenderer : public SceneRenderer, protected QOpenGLFunctions
{
public:

	/// Defines a metaclass specialization for this renderer class.
	class OOMetaClass : public SceneRenderer::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using SceneRenderer::OOMetaClass::OOMetaClass;

		/// Is called by OVITO to query the class for any information that should be included in the application's system report.
		virtual void querySystemInformation(QTextStream& stream, DataSetContainer& container) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(OpenGLSceneRenderer, OOMetaClass)

public:

	/// Default constructor.
	explicit OpenGLSceneRenderer(DataSet* dataset) : SceneRenderer(dataset) {}

	/// This may be called on a renderer before startRender() to control its supersampling level.
	virtual void setAntialiasingHint(int antialiasingLevel) override { _antialiasingLevel = antialiasingLevel; }

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer) override;

	/// Requests a new line geometry buffer from the renderer.
	virtual std::shared_ptr<LinePrimitive> createLinePrimitive() override;

	/// Renders the line geometry stored in the given buffer.
	virtual void renderLines(const std::shared_ptr<LinePrimitive>& primitive) override;

	/// Requests a new particle geometry buffer from the renderer.
	virtual std::shared_ptr<ParticlePrimitive> createParticlePrimitive(
			ParticlePrimitive::ShadingMode shadingMode,
			ParticlePrimitive::RenderingQuality renderingQuality, 
			ParticlePrimitive::ParticleShape shape) override;

	/// Renders the particles stored in the given primitive buffer.
	virtual void renderParticles(const std::shared_ptr<ParticlePrimitive>& primitive) override;

	/// Requests a new marker geometry buffer from the renderer.
	virtual std::shared_ptr<MarkerPrimitive> createMarkerPrimitive(MarkerPrimitive::MarkerShape shape) override;

	/// Renders the marker geometry stored in the given buffer.
	virtual void renderMarkers(const std::shared_ptr<MarkerPrimitive>& primitive) override;

	/// Requests a new text geometry buffer from the renderer.
	virtual std::shared_ptr<TextPrimitive> createTextPrimitive() override;

	/// Renders the text stored in the given primitive buffer.
	virtual void renderText(const std::shared_ptr<TextPrimitive>& primitive) override;

	/// Requests a new image geometry buffer from the renderer.
	virtual std::shared_ptr<ImagePrimitive> createImagePrimitive() override;

	/// Renders the image stored in the given primitive buffer.
	virtual void renderImage(const std::shared_ptr<ImagePrimitive>& primitive) override;

	/// Requests a new cylinder geometry buffer from the renderer.
	virtual std::shared_ptr<CylinderPrimitive> createCylinderPrimitive(CylinderPrimitive::Shape shape,
			CylinderPrimitive::ShadingMode shadingMode,
			CylinderPrimitive::RenderingQuality renderingQuality) override;

	/// Renders the cylinder or arrow elements stored in the given buffer.
	virtual void renderCylinders(const std::shared_ptr<CylinderPrimitive>& primitive) override;

	/// Requests a new triangle mesh buffer from the renderer.
	virtual std::shared_ptr<MeshPrimitive> createMeshPrimitive() override;

	/// Renders the triangle mesh stored in the given buffer.
	virtual void renderMesh(const std::shared_ptr<MeshPrimitive>& primitive) override;

	/// Determines if this renderer can share geometry data and other resources with the given other renderer.
	virtual bool sharesResourcesWith(SceneRenderer* otherRenderer) const override;

	/// Renders a 2d polyline or polygon into an interactive viewport.
	virtual void render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed) override;

	/// Returns the OpenGL context this renderer uses.
	QOpenGLContext* glcontext() const { return _glcontext; }

	/// Returns the surface format of the current OpenGL context.
	const QSurfaceFormat& glformat() const { return _glformat; }

	/// Indicates whether it is okay to use GLSL geometry shaders.
	bool useGeometryShaders() const { return _useGeometryShaders; }

	/// Translates an OpenGL error code to a human-readable message string.
	static const char* openglErrorString(GLenum errorCode);

	/// Loads and compiles an OpenGL shader program.
	QOpenGLShaderProgram* loadShaderProgram(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile = QString());

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	/// This is an internal method used by the PickingOpenGLSceneRenderer class to implement the picking mechanism.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount) { return 1; }

	/// Binds the default vertex array object again in case another VAO was bound in between.
	/// This method should be called before calling an OpenGL rendering function.
	void rebindVAO() {
		makeContextCurrent();
		if(_vertexArrayObject) _vertexArrayObject->bind();
	}

	/// Sets the frame buffer background color.
	void setClearColor(const ColorA& color);

	/// Sets the rendering region in the frame buffer.
	void setRenderingViewport(int x, int y, int width, int height);

	/// Clears the frame buffer contents.
	void clearFrameBuffer(bool clearDepthBuffer = true, bool clearStencilBuffer = true);

	/// Temporarily enables/disables the depth test while rendering.
	virtual void setDepthTestEnabled(bool enabled) override;

	/// Activates the special highlight rendering mode.
	virtual void setHighlightMode(int pass) override;

	/// Reports OpenGL error status codes.
	void checkOpenGLErrorStatus(const char* command, const char* sourceFile, int sourceLine);

	/// Determines whether OpenGL geometry shader programs should be used or not.
	static bool geometryShadersEnabled(bool forceDefaultSetting = false);

	/// Determines whether OpenGL geometry shader programs are supported by the hardware.
	static bool geometryShadersSupported() { return _openglSupportsGeomShaders; }

	/// Returns the vendor name of the OpenGL implementation in use.
	static const QByteArray& openGLVendor() { return _openGLVendor; }

	/// Returns the renderer name of the OpenGL implementation in use.
	static const QByteArray& openGLRenderer() { return _openGLRenderer; }

	/// Returns the version string of the OpenGL implementation in use.
	static const QByteArray& openGLVersion() { return _openGLVersion; }

	/// Returns the version of the OpenGL shading language supported by the system.
	static const QByteArray& openGLSLVersion() { return _openGLSLVersion; }

	/// Returns the current surface format used by the OpenGL implementation.
	static const QSurfaceFormat& openglSurfaceFormat() { return _openglSurfaceFormat; }

	/// Determines the capabilities of the current OpenGL implementation.
	static void determineOpenGLInfo();

protected:

	/// \brief Loads and compiles a GLSL shader and adds it to the given program object.
	void loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename);

	/// Makes the renderer's GL context current.
	void makeContextCurrent();

	/// Returns the supersampling level.
	int antialiasingLevel() const { return _antialiasingLevel; }

	/// Puts the GL context into its default initial state before rendering of a frame begins.
	virtual void initializeGLState();

	/// This is called during rendering whenever the rendering process has been temporarily
	/// interrupted by an event loop and before rendering is resumed. It gives the renderer
	/// the opportunity to restore the active OpenGL context.
	virtual void resumeRendering() override {
		if(!isBoundingBoxPass())
			rebindVAO();
	}

#ifndef Q_OS_WASM

	/// The OpenGL glPointParameterf() function.
	void glPointSize(GLfloat size) {
		if(_glFunctions32) _glFunctions32->glPointSize(size);
		else if(_glFunctions30) _glFunctions30->glPointSize(size);
	}

	/// The OpenGL glPointParameterf() function.
	void glPointParameterf(GLenum pname, GLfloat param) {
		if(_glFunctions32) _glFunctions32->glPointParameterf(pname, param);
		else if(_glFunctions30) _glFunctions30->glPointParameterf(pname, param);
	}

	/// The OpenGL glPointParameterfv() function.
	void glPointParameterfv(GLenum pname, const GLfloat* params) {
		if(_glFunctions32) _glFunctions32->glPointParameterfv(pname, params);
		else if(_glFunctions30) _glFunctions30->glPointParameterfv(pname, params);
	}

	/// The OpenGL glMultiDrawArrays() function.
	void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) {
		if(_glFunctions32) _glFunctions32->glMultiDrawArrays(mode, first, count, drawcount);
		else if(_glFunctions30) _glFunctions30->glMultiDrawArrays(mode, first, count, drawcount);
	}

	void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
		if(_glFunctions30) _glFunctions30->glTexEnvf(target, pname, param);
	}

#endif

private:

	/// The OpenGL context this renderer uses.
	QOpenGLContext* _glcontext = nullptr;

	/// The GL context group this renderer uses.
	QPointer<QOpenGLContextGroup> _glcontextGroup;

	/// The surface used by the GL context.
	QSurface* _glsurface = nullptr;

#ifndef Q_OS_WASM

	/// The OpenGL 3.0 functions object.
	QOpenGLFunctions_3_0* _glFunctions30 = nullptr;

	/// The OpenGL 3.2 core profile functions object.
	QOpenGLFunctions_3_2_Core* _glFunctions32 = nullptr;

#endif	

	/// The OpenGL vertex array object that is required by OpenGL 3.2 core profile.
	QScopedPointer<QOpenGLVertexArrayObject> _vertexArrayObject;

	/// The OpenGL surface format.
	QSurfaceFormat _glformat;

	/// Indicates whether it is okay to use GLSL geometry shaders.
	bool _useGeometryShaders = false;

	/// Controls the number of sub-pixels to render.
	int _antialiasingLevel = 1;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<ParticlePrimitive>>> _translucentParticles;

	/// List of semi-transparent czlinder primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<CylinderPrimitive>>> _translucentCylinders;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<MeshPrimitive>>> _translucentMeshes;

	/// The vendor of the OpenGL implementation in use.
	static QByteArray _openGLVendor;

	/// The renderer name of the OpenGL implementation in use.
	static QByteArray _openGLRenderer;

	/// The version string of the OpenGL implementation in use.
	static QByteArray _openGLVersion;

	/// The version of the OpenGL shading language supported by the system.
	static QByteArray _openGLSLVersion;

	/// The current surface format used by the OpenGL implementation.
	static QSurfaceFormat _openglSurfaceFormat;

	/// Indicates whether the current OpenGL implementation supports geometry shader programs.
	static bool _openglSupportsGeomShaders;

	friend class OpenGLMeshPrimitive;
	friend class OpenGLCylinderPrimitive;
	friend class OpenGLImagePrimitive;
	friend class OpenGLLinePrimitive;
	friend class OpenGLTextPrimitive;
	friend class OpenGLParticlePrimitive;
	friend class OpenGLMarkerPrimitive;
	template<typename T> friend class OpenGLBuffer;
};

}	// End of namespace
