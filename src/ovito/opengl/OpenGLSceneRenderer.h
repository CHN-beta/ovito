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
#include "OpenGLResourceManager.h"

#include <QOpenGLExtraFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject> 
#include <QOpenGLFramebufferObjectFormat>

namespace Ovito {

class OpenGLShaderHelper; // defined in OpenGLShaderHelper.h

/**
 * \brief An OpenGL-based scene renderer. This serves as base class for both the interactive renderer used
 *        by the viewports and the standard output renderer.
 */
class OVITO_OPENGLRENDERER_EXPORT OpenGLSceneRenderer : public SceneRenderer, public QOpenGLExtraFunctions
{
public:

	/// Defines a metaclass specialization for this renderer class.
	class OVITO_OPENGLRENDERER_EXPORT OOMetaClass : public SceneRenderer::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using SceneRenderer::OOMetaClass::OOMetaClass;

		/// Is called by OVITO to query the class for any information that should be included in the application's system report.
		virtual void querySystemInformation(QTextStream& stream, UserInterface& userInterface) const override;
	};

	OVITO_CLASS_META(OpenGLSceneRenderer, OOMetaClass)

public:

	/// Constructor.
	OpenGLSceneRenderer(ObjectCreationParams params);

	/// This may be called on a renderer before startRender() to control its supersampling level.
	virtual void setAntialiasingHint(int antialiasingLevel) override { _antialiasingLevel = antialiasingLevel; }

	/// Returns the device pixel ratio of the output device we are rendering to.
	virtual qreal devicePixelRatio() const override { return antialiasingLevel() * SceneRenderer::devicePixelRatio(); }

	/// Renders the current animation frame.
	virtual bool renderFrame(const QRect& viewportRect, MainThreadOperation& operation) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect, FrameBuffer* frameBuffer) override;

	/// Renders the overlays/underlays of the viewport into the framebuffer.
	virtual bool renderOverlays(bool underlays, const QRect& logicalViewportRect, const QRect& physicalViewportRect, MainThreadOperation& operation) override;
	
	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderingSuccessful, const QRect& viewportRect) override;

	/// Renders the line geometry stored in the given buffer.
	virtual void renderLines(const LinePrimitive& primitive) override;

	/// Renders the particles stored in the given primitive buffer.
	virtual void renderParticles(const ParticlePrimitive& primitive) override;

	/// Renders the marker geometry stored in the given buffer.
	virtual void renderMarkers(const MarkerPrimitive& primitive) override;

	/// Renders the text stored in the given primitive buffer.
	virtual void renderText(const TextPrimitive& primitive) override;

	/// Renders the image stored in the given primitive buffer.
	virtual void renderImage(const ImagePrimitive& primitive) override;

	/// Renders the cylinder or arrow elements stored in the given buffer.
	virtual void renderCylinders(const CylinderPrimitive& primitive) override;

	/// Renders the triangle mesh stored in the given buffer.
	virtual void renderMesh(const MeshPrimitive& primitive) override;

	/// Returns the OpenGL context this renderer uses.
	QOpenGLContext* glcontext() const { return _glcontext; }

	/// Returns the surface format of the current OpenGL context.
	const QSurfaceFormat& glformat() const { return _glformat; }

	/// Translates an OpenGL error code to a human-readable message string.
	static const char* openglErrorString(GLenum errorCode);

	/// Loads and compiles an OpenGL shader program.
	QOpenGLShaderProgram* loadShaderProgram(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile = QString());

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	/// This is an internal method used by the PickingOpenGLSceneRenderer class to implement the picking mechanism.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount, const ConstDataBufferPtr& indices = {}) { return 1; }

	/// Binds the default vertex array object again in case another VAO was bound in between.
	/// This method should be called before calling an OpenGL rendering function.
	void rebindVAO() {
		makeContextCurrent();
		if(_vertexArrayObject) _vertexArrayObject->bind();
	}

	/// Sets the frame buffer background color.
	void setClearColor(const ColorA& color);

	/// Clears the frame buffer contents.
	void clearFrameBuffer(bool clearDepthBuffer = true, bool clearStencilBuffer = true);

	/// Temporarily enables/disables the depth test while rendering.
	virtual void setDepthTestEnabled(bool enabled) override;

	/// Activates the special highlight rendering mode.
	virtual void setHighlightMode(int pass) override;

	/// Reports OpenGL error status codes.
	void checkOpenGLErrorStatus(const char* command, const char* sourceFile, int sourceLine);

	/// Returns the monotonically increasing identifier of the current frame being rendered.
	OpenGLResourceManager::ResourceFrameHandle currentResourceFrame() const { return _currentResourceFrame; }

	/// Sets monotonically increasing identifier of the current frame being rendered.
	void setCurrentResourceFrame(OpenGLResourceManager::ResourceFrameHandle frame) { _currentResourceFrame = frame; }

	/// Returns the OpenGL context version encoded as an integer.
	quint32 glversion() const { return _glversion; }

	/// Indicates whether OpenGL geometry shaders are supported.
	bool useGeometryShaders() const { return QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry, glcontext()); }

	/// Sets the primary framebuffer to be used by the renderer.
	void setPrimaryFramebuffer(GLuint primaryFramebuffer) { _primaryFramebuffer = primaryFramebuffer; }

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

	/// Returns the list of extensions supported by the OpenGL implementation.
	static const QSet<QByteArray>& openglExtensions() { return _openglExtensions; }

	/// Returns whether the OpenGL implementation supports geometry shaders.
	static bool openGLSupportsGeometryShaders() { return _openGLSupportsGeometryShaders; }

	/// Determines the capabilities of the current OpenGL implementation.
	static void determineOpenGLInfo();

protected:

	/// Loads and compiles a GLSL shader and adds it to the given program object.
	void loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename, bool isWBOITPass);

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

private:

	/// Render all semi-transparent geometry in a second rendering pass.
	void renderTransparentGeometry();

	/// Renders a set of particles.
	void renderParticlesImplementation(const ParticlePrimitive& primitive);

	/// Renders a triangle mesh.
	void renderMeshImplementation(const MeshPrimitive& primitive);

	/// Renders just the edges of a triangle mesh as a wireframe model.
	void renderMeshWireframeImplementation(const MeshPrimitive& primitive);

	/// Generates the wireframe line elements for the visible edges of a mesh.
	ConstDataBufferPtr generateMeshWireframeLines(const MeshPrimitive& primitive);

	/// Prepares the OpenGL buffer with the per-instance transformation matrices for 
	/// rendering a set of meshes.
	QOpenGLBuffer getMeshInstanceTMBuffer(const MeshPrimitive& primitive, OpenGLShaderHelper& shader);

	/// Renders a set of markers.
	void renderMarkersImplementation(const MarkerPrimitive& primitive);

	/// Renders a set of lines.
	void renderLinesImplementation(const LinePrimitive& primitive);

	/// Renders a set of lines using GL_LINES mode.
	void renderThinLinesImplementation(const LinePrimitive& primitive);

	/// Renders a set of lines using triangle strips.
	void renderThickLinesImplementation(const LinePrimitive& primitive);

	/// Renders a set of cylinders or arrow glyphs.
	void renderCylindersImplementation(const CylinderPrimitive& primitive);

	/// Renders a 2d pixel image into the output framebuffer.
	void renderImageImplementation(const ImagePrimitive& primitive);

	/// Returns whether the renderer is using a two-pass OIT method.
	bool orderIndependentTransparency() const { return _orderIndependentTransparency; }

private:

	/// The OpenGL context this renderer uses.
	QOpenGLContext* _glcontext = nullptr;

	/// The GL context group this renderer uses.
	QPointer<QOpenGLContextGroup> _glcontextGroup;

	/// The surface used by the GL context.
	QSurface* _glsurface = nullptr;

	/// Pointer to the glMultiDrawArrays() function. Requires OpenGL 2.0.
	void (QOPENGLF_APIENTRY *glMultiDrawArrays)(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) = nullptr;

	/// Pointer to the optional glMultiDrawArraysIndirect() function. Requires OpenGL 4.3.
	void (QOPENGLF_APIENTRY *glMultiDrawArraysIndirect)(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) = nullptr;

	/// The OpenGL vertex array object that is required by OpenGL 3.2 core profile.
	std::unique_ptr<QOpenGLVertexArrayObject> _vertexArrayObject;

	/// The OpenGL surface format.
	QSurfaceFormat _glformat;

	/// The OpenGL version of the context encoded as an integer.
	quint32 _glversion;

	/// Controls the number of sub-pixels to render.
	int _antialiasingLevel = 1;

	/// Controls whether the renderer is using a two-pass OIT method.
	bool _orderIndependentTransparency = false;

	/// Indicates that we are currently rendering the semi-transparent geometry of the scene.
	bool _isTransparencyPass = false;

	/// The primary framebuffer used by the renderer. The FBO's lifetime is managed by the subclass.
	/// It may be null when rendering to the system framebuffer provided by QOpenGLWidget.
	GLuint _primaryFramebuffer = 0;

	/// The additional framebuffer used for the OIT transparency pass.
	std::unique_ptr<QOpenGLFramebufferObject> _oitFramebuffer;

	/// The monotonically increasing identifier of the current frame being rendered.
	OpenGLResourceManager::ResourceFrameHandle _currentResourceFrame = 0;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, ParticlePrimitive>> _translucentParticles;

	/// List of semi-transparent czlinder primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, CylinderPrimitive>> _translucentCylinders;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, MeshPrimitive>> _translucentMeshes;

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

	/// The list of extensions supported by the OpenGL implementation.
	static QSet<QByteArray> _openglExtensions;

	/// Indicates whether the OpenGL implementation supports geometry shaders.
	static bool _openGLSupportsGeometryShaders;

	friend class OpenGLShaderHelper;
};

}	// End of namespace
