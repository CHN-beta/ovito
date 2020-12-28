////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include "OpenGLLinePrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLLinePrimitive::OpenGLLinePrimitive(OpenGLSceneRenderer* renderer) :
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_indicesBuffer(QOpenGLBuffer::IndexBuffer)
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);
	QString prefix = renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles") : QStringLiteral(":/openglrenderer");

	// Initialize OpenGL shaders.
	_shader = renderer->loadShaderProgram("line", prefix + "/glsl/lines/line.vs", prefix + "/glsl/lines/line.fs");
	_thickLineShader = renderer->loadShaderProgram("thick_line", prefix + "/glsl/lines/thick_line.vs", prefix + "/glsl/lines/line.fs");

	// Standard line width.
	_lineWidth = renderer->devicePixelRatio();
}

/******************************************************************************
* Allocates a vertex buffer with the given number of vertices.
******************************************************************************/
void OpenGLLinePrimitive::setVertexCount(int vertexCount, FloatType lineWidth)
{
	OVITO_ASSERT(vertexCount >= 0);
	OVITO_ASSERT((vertexCount & 1) == 0);
	OVITO_ASSERT(vertexCount < std::numeric_limits<int>::max() / sizeof(ColorAT<float>));
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	OVITO_ASSERT(lineWidth >= 0);

	if(lineWidth != 0)
		_lineWidth = lineWidth;

	if(_lineWidth == 1) {
		_positionsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount);
		_colorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount);
	}
	else {
		_positionsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		_colorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		_vectorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		_indicesBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount * 6 / 2);
		GLuint* indices = _indicesBuffer.map();
		for(int i = 0; i < vertexCount; i += 2, indices += 6) {
			indices[0] = i * 2;
			indices[1] = i * 2 + 1;
			indices[2] = i * 2 + 2;
			indices[3] = i * 2;
			indices[4] = i * 2 + 2;
			indices[5] = i * 2 + 3;
		}
		_indicesBuffer.unmap();
	}
}

/******************************************************************************
* Sets the coordinates of the vertices.
******************************************************************************/
void OpenGLLinePrimitive::setVertexPositions(const Point3* coordinates)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_positionsBuffer.fill(coordinates);

	if(_lineWidth != 1) {
		Vector_3<float>* vectors = _vectorsBuffer.map();
		Vector_3<float>* vectors_end = vectors + _vectorsBuffer.elementCount() * _vectorsBuffer.verticesPerElement();
		for(; vectors != vectors_end; vectors += 4, coordinates += 2) {
			vectors[3] = vectors[0] = (Vector_3<float>)(coordinates[1] - coordinates[0]);
			vectors[1] = vectors[2] = -vectors[0];
		}
		_vectorsBuffer.unmap();
	}
}

/******************************************************************************
* Sets the colors of the vertices.
******************************************************************************/
void OpenGLLinePrimitive::setVertexColors(const ColorA* colors)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorsBuffer.fill(colors);
}

/******************************************************************************
* Sets the color of all vertices to the given value.
******************************************************************************/
void OpenGLLinePrimitive::setLineColor(const ColorA color)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorsBuffer.fillConstant(color);
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLLinePrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = qobject_cast<OpenGLSceneRenderer*>(renderer);
	if(!vpRenderer) return false;
	return _positionsBuffer.isCreated() && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLLinePrimitive::render(SceneRenderer* renderer)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(vertexCount() <= 0 || !vpRenderer)
		return;

	vpRenderer->rebindVAO();

	if(_lineWidth == 1)
		renderLines(vpRenderer);
	else
		renderThickLines(vpRenderer);
}

/******************************************************************************
* Renders the lines using GL_LINES mode.
******************************************************************************/
void OpenGLLinePrimitive::renderLines(OpenGLSceneRenderer* renderer)
{
	// Get the OpenGL shader program.
	QOpenGLShaderProgram* shader = _shader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	// Set shader uniforms.
	shader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());
	shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, shader);
	if(!renderer->isPicking()) {
		_colorsBuffer.bindColors(renderer, shader, 4);
	}
	else {
		GLint pickingBaseID = renderer->registerSubObjectIDs(lineCount());
		shader->setUniformValue("picking_base_id", pickingBaseID);
	}

	OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_LINES, 0, _positionsBuffer.elementCount() * _positionsBuffer.verticesPerElement()));

	// Detach VBOs.
	_positionsBuffer.detachPositions(renderer, shader);
	if(!renderer->isPicking())
		_colorsBuffer.detachColors(renderer, shader);

	// Reset state.
	shader->release();
}

/******************************************************************************
* Renders the lines using polygons.
******************************************************************************/
void OpenGLLinePrimitive::renderThickLines(OpenGLSceneRenderer* renderer)
{
	// Get the OpenGL shader program.
	QOpenGLShaderProgram* shader = _thickLineShader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	// Set shader uniforms.
	shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
	shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	shader->setUniformValue("line_width", (GLfloat)(_lineWidth / (renderer->projParams().projectionMatrix(1,1) * viewportCoords[3])));
	shader->setUniformValue("is_perspective", renderer->projParams().isPerspective);
	shader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, shader);
	_vectorsBuffer.bind(renderer, shader, "vector", GL_FLOAT, 0, 3);
	if(!renderer->isPicking()) {
		_colorsBuffer.bindColors(renderer, shader, 4);
	}
	else {
		GLint pickingBaseID = renderer->registerSubObjectIDs(lineCount());
		shader->setUniformValue("picking_base_id", pickingBaseID);
	}

	// Bind IBO.
	_indicesBuffer.oglBuffer().bind();

	renderer->glDrawElements(GL_TRIANGLES, _indicesBuffer.elementCount(), GL_UNSIGNED_INT, nullptr);

	// Detach VBOs and IBO.
	_indicesBuffer.oglBuffer().release();
	_positionsBuffer.detachPositions(renderer, shader);
	if(!renderer->isPicking())
		_colorsBuffer.detachColors(renderer, shader);

	// Reset state.
	_vectorsBuffer.detach(renderer, shader, "vector");
	shader->release();
}

}	// End of namespace
