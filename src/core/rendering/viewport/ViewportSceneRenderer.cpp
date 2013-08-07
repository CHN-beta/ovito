///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include <core/scene/SceneNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/GroupNode.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/display/DisplayObject.h>
#include <core/dataset/DataSet.h>
#include <core/viewport/input/ViewportInputManager.h>
#include "ViewportSceneRenderer.h"

namespace Ovito {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, ViewportSceneRenderer, SceneRenderer);

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void ViewportSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);

	_glcontext = QOpenGLContext::currentContext();
	if(!_glcontext)
		throw Exception(tr("Cannot render scene: There is no active OpenGL context"));

	OVITO_CHECK_OPENGL();

	// Obtain a functions object that allows to call OpenGL 2.0 functions in a platform-independent way.
	_glFunctions = _glcontext->functions();

	// Obtain a functions object that allows to call OpenGL 2.1 functions in a platform-independent way.
	_glFunctions21 = _glcontext->versionFunctions<QOpenGLFunctions_2_1>();
	if(!_glFunctions21 || !_glFunctions21->initializeOpenGLFunctions())
		_glFunctions21 = nullptr;

	// Obtain a functions object that allows to call OpenGL 3.0 functions in a platform-independent way.
	_glFunctions30 = _glcontext->versionFunctions<QOpenGLFunctions_3_0>();
	if(!_glFunctions30 || !_glFunctions30->initializeOpenGLFunctions())
		_glFunctions30 = nullptr;

	// Obtain a functions object that allows to call OpenGL 3.2 core functions in a platform-independent way.
	_glFunctions32 = _glcontext->versionFunctions<QOpenGLFunctions_3_2_Core>();
	if(!_glFunctions32 || !_glFunctions32->initializeOpenGLFunctions())
		_glFunctions32 = nullptr;

	if(!_glFunctions21 && !_glFunctions30 && !_glFunctions32)
		throw Exception(tr("Could not resolve OpenGL functions. Invalid OpenGL context."));

	// Obtain surface format.
	_glformat = _glcontext->format();

	// Set up a vertex array object. This is only required when using OpenGL 3.2 Core Profile.
	if(glformat().profile() == QSurfaceFormat::CoreProfile) {
		_vertexArrayObject.reset(new QOpenGLVertexArrayObject());
		_vertexArrayObject->create();
		_vertexArrayObject->bind();
	}

	// Set viewport background color.
	OVITO_CHECK_OPENGL();
	Color backgroundColor = Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG);
	OVITO_CHECK_OPENGL(glClearColor(backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), 1));
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void ViewportSceneRenderer::endFrame()
{
	_vertexArrayObject.reset();
	_glcontext = nullptr;

	SceneRenderer::endFrame();
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool ViewportSceneRenderer::renderFrame(FrameBuffer* frameBuffer, QProgressDialog* progress)
{
	OVITO_ASSERT(_glcontext == QOpenGLContext::currentContext());

	// Clear background.
	OVITO_CHECK_OPENGL();
	OVITO_CHECK_OPENGL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	OVITO_CHECK_OPENGL(glEnable(GL_DEPTH_TEST));

	renderScene();

	// Render visual 3D representation of the modifiers.
	renderModifiers(false);

	// Render visual 2D representation of the modifiers.
	renderModifiers(true);

	// Render input mode overlays.
	for(const auto& handler : ViewportInputManager::instance().stack()) {
		handler->renderOverlay(viewport(), this, handler == ViewportInputManager::instance().currentHandler());
	}

	return true;
}

/******************************************************************************
* Changes the current local to world transformation matrix.
******************************************************************************/
void ViewportSceneRenderer::setWorldTransform(const AffineTransformation& tm)
{
	_modelViewTM = projParams().viewMatrix * tm;
}

/******************************************************************************
* Translates an OpenGL error code to a human-readable message string.
******************************************************************************/
const char* ViewportSceneRenderer::openglErrorString(GLenum errorCode)
{
	switch(errorCode) {
	case GL_NO_ERROR: return "GL_NO_ERROR - No error has been recorded.";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM - An unacceptable value is specified for an enumerated argument.";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE - A numeric argument is out of range.";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION - The specified operation is not allowed in the current state.";
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW - This command would cause a stack overflow.";
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW - This command would cause a stack underflow.";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY - There is not enough memory left to execute the command.";
	case GL_TABLE_TOO_LARGE: return "GL_TABLE_TOO_LARGE - The specified table exceeds the implementation's maximum supported table size.";
	default: return "Unknown OpenGL error code.";
	}
}

/******************************************************************************
* Renders a single node.
******************************************************************************/
void ViewportSceneRenderer::renderNode(SceneNode* node)
{
    OVITO_CHECK_OBJECT_POINTER(node);

    // Setup transformation matrix.
	TimeInterval interval;
	const AffineTransformation& nodeTM = node->getWorldTransform(time(), interval);
	setWorldTransform(nodeTM);

	if(node->isObjectNode()) {
		ObjectNode* objNode = static_object_cast<ObjectNode>(node);

		// Do not render node if it is the view node of the viewport or
		// if it is the target of the view node.
		if(viewport() && viewport()->viewNode()) {
			if(viewport()->viewNode() == objNode || viewport()->viewNode()->targetNode() == objNode)
				return;
		}

		// Evaluate geometry pipeline of object node and render the results.
		objNode->render(time(), this);
	}

	// Continue with rendering the child nodes.
	SceneRenderer::renderNode(node);
}

/******************************************************************************
* Renders the visual representation of the modifiers.
******************************************************************************/
void ViewportSceneRenderer::renderModifiers(bool renderOverlay)
{
	// Visit all pipeline objects in the scene.
	dataset()->sceneRoot()->visitChildren([this, renderOverlay](SceneNode* node) {
		if(node->isObjectNode()) {
			ObjectNode* objNode = static_object_cast<ObjectNode>(node);
			PipelineObject* pipelineObj = dynamic_object_cast<PipelineObject>(objNode->sceneObject());
			if(pipelineObj)
				renderModifiers(pipelineObj, objNode, renderOverlay);
		}
	});
}


/******************************************************************************
* Renders the visual representation of the modifiers.
******************************************************************************/
void ViewportSceneRenderer::renderModifiers(PipelineObject* pipelineObj, ObjectNode* objNode, bool renderOverlay)
{
	OVITO_CHECK_OBJECT_POINTER(pipelineObj);

	// Render the visual representation of the modifier that is currently being edited.
	for(ModifierApplication* modApp : pipelineObj->modifierApplications()) {
		Modifier* mod = modApp->modifier();

		TimeInterval interval;
		// Setup transformation.
		setWorldTransform(objNode->getWorldTransform(time(), interval));

		// Render selected modifier.
		mod->render(time(), objNode, modApp, this, renderOverlay);
	}

	// Continue with nested pipeline objects.
	for(int i = 0; i < pipelineObj->inputObjectCount(); i++) {
		PipelineObject* input = dynamic_object_cast<PipelineObject>(pipelineObj->inputObject(i));
		if(input)
			renderModifiers(input, objNode, renderOverlay);
	}
}

/******************************************************************************
* Determines the bounding box of the visual representation of the modifiers.
******************************************************************************/
void ViewportSceneRenderer::boundingBoxModifiers(PipelineObject* pipelineObj, ObjectNode* objNode, Box3& boundingBox)
{
	OVITO_CHECK_OBJECT_POINTER(pipelineObj);
	TimeInterval interval;

	// Render the visual representation of the modifier that is currently being edited.
	for(ModifierApplication* modApp : pipelineObj->modifierApplications()) {
		Modifier* mod = modApp->modifier();

		// Compute bounding box and transform it to world space.
		boundingBox.addBox(mod->boundingBox(time(), objNode, modApp).transformed(objNode->getWorldTransform(time(), interval)));
	}

	// Continue with nested pipeline objects.
	for(int i = 0; i < pipelineObj->inputObjectCount(); i++) {
		PipelineObject* input = dynamic_object_cast<PipelineObject>(pipelineObj->inputObject(i));
		if(input)
			boundingBoxModifiers(input, objNode, boundingBox);
	}
}

/******************************************************************************
* Computes the bounding box of the entire scene to be rendered.
******************************************************************************/
Box3 ViewportSceneRenderer::sceneBoundingBox(TimePoint time)
{
	Box3 bb = SceneRenderer::sceneBoundingBox(time);
	if(isInteractive()) {
		// Visit all pipeline objects in the scene.
		dataset()->sceneRoot()->visitChildren([this, &bb](SceneNode* node) {
			if(node->isObjectNode()) {
				ObjectNode* objNode = static_object_cast<ObjectNode>(node);
				PipelineObject* pipelineObj = dynamic_object_cast<PipelineObject>(objNode->sceneObject());
				if(pipelineObj)
					boundingBoxModifiers(pipelineObj, objNode, bb);
			}
		});
	}
	return bb;
}

/******************************************************************************
* Loads an OpenGL shader program.
******************************************************************************/
QOpenGLShaderProgram* ViewportSceneRenderer::loadShaderProgram(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile)
{
	QOpenGLContextGroup* contextGroup = glcontext()->shareGroup();
	OVITO_ASSERT(contextGroup == QOpenGLContextGroup::currentContextGroup());

	OVITO_ASSERT(QOpenGLShaderProgram::hasOpenGLShaderPrograms());
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Vertex));
	OVITO_ASSERT(QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Fragment));

	// The OpenGL shaders are only created once per OpenGL context group.
	QScopedPointer<QOpenGLShaderProgram> program(contextGroup->findChild<QOpenGLShaderProgram*>(id));
	if(program)
		return program.take();

	program.reset(new QOpenGLShaderProgram(contextGroup));
	program->setObjectName(id);

	// Load and compile vertex shader source.
	loadShader(program.data(), QOpenGLShader::Vertex, vertexShaderFile);

	// Load and compile fragment shader source.
	loadShader(program.data(), QOpenGLShader::Fragment, fragmentShaderFile);

	// Load and compile geometry shader source.
	if(!geometryShaderFile.isEmpty()) {
		loadShader(program.data(), QOpenGLShader::Geometry, geometryShaderFile);
	}

	if(!program->link()) {
		qDebug() << "OpenGL shader log:";
		qDebug() << program->log();
		throw Exception(QString("The OpenGL shader program %1 failed to link. See log for details.").arg(id));
	}

	OVITO_ASSERT(contextGroup->findChild<QOpenGLShaderProgram*>(id) == program.data());
	return program.take();
}

/******************************************************************************
* Loads and compiles a GLSL shader and adds it to the given program object.
******************************************************************************/
void ViewportSceneRenderer::loadShader(QOpenGLShaderProgram* program, QOpenGLShader::ShaderType shaderType, const QString& filename)
{
	// Load shader source.
	QFile shaderSourceFile(filename);
	if(!shaderSourceFile.open(QFile::ReadOnly))
		throw Exception(QString("Unable to open shader source file %1.").arg(filename));
    QByteArray shaderSource = shaderSourceFile.readAll();

    // Insert GLSL version string at the top.
	// Pick GLSL language version based on current OpenGL version.
	if(glformat().majorVersion() >= 3 && glformat().minorVersion() >= 2)
		shaderSource.prepend("#version 150\n");
	else if(glformat().majorVersion() >= 3)
		shaderSource.prepend("#version 130\n");
	else 
		shaderSource.prepend("#version 120\n");

	// Load and compile vertex shader source.
    if(!program->addShaderFromSourceCode(shaderType, shaderSource)) {
		qDebug() << "OpenGL shader log:";
		qDebug() << program->log();
		throw Exception(QString("The shader source file %1 failed to compile. See log for details.").arg(filename));
	}
}


};
