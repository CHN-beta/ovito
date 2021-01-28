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
	_indicesBuffer(QOpenGLBuffer::IndexBuffer)
{
	QString prefix = renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles") : QStringLiteral(":/openglrenderer");

	// Initialize OpenGL shaders.
	_thinLineShader = renderer->loadShaderProgram("line_thin", prefix + "/glsl/lines/line.vs", prefix + "/glsl/lines/line.fs");
	_thickLineShader = renderer->loadShaderProgram("line_thick", prefix + "/glsl/lines/thick_line.vs", prefix + "/glsl/lines/line.fs");
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLLinePrimitive::render(OpenGLSceneRenderer* renderer)
{
	if(!positions() || positions()->size() == 0)
		return;

	if(lineWidth() == 1 || (lineWidth() <= 0 && renderer->devicePixelRatio() <= 1))
		renderThinLines(renderer);
	else
		renderThickLines(renderer);
}

/******************************************************************************
* Renders the lines using GL_LINES mode.
******************************************************************************/
void OpenGLLinePrimitive::renderThinLines(OpenGLSceneRenderer* renderer)
{
	// Activate the OpenGL shader program.
	if(!_thinLineShader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	// Set shader uniforms.
	_thinLineShader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());
	_thinLineShader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));

	// Fill VBOs.
	_positionsBuffer.uploadData<Point3>(positions());
	_colorsBuffer.uploadData<ColorA>(colors());

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, _thinLineShader);
	if(!renderer->isPicking()) {
		if(_colorsBuffer.isCreated())
			_colorsBuffer.bindColors(renderer, _thinLineShader, 4);
		else
			_colorsBuffer.setUniformColor(renderer, _thinLineShader, uniformColor());
	}
	else {
		GLint pickingBaseID = renderer->registerSubObjectIDs(positions()->size() / 2);
		_thinLineShader->setUniformValue("picking_base_id", pickingBaseID);
	}

	OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_LINES, 0, _positionsBuffer.elementCount()));

	// Detach VBOs.
	_positionsBuffer.detachPositions(renderer, _thinLineShader);
	if(!renderer->isPicking() && _colorsBuffer.isCreated())
		_colorsBuffer.detachColors(renderer, _thinLineShader);

	// Reset state.
	_thinLineShader->release();
}

/******************************************************************************
* Renders the lines using polygons.
******************************************************************************/
void OpenGLLinePrimitive::renderThickLines(OpenGLSceneRenderer* renderer)
{
	// Effective line width.
	FloatType effectiveLineWidth = (lineWidth() <= 0) ? renderer->devicePixelRatio() : lineWidth();

	// Fill IBO.
	int vertexCount = positions()->size();
	if(_indicesBuffer.elementCount() < vertexCount * 6 / 2) {
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

	// Fill vector VBO.
	if(!_vectorsBuffer.isCreated()) {
		_vectorsBuffer.create(QOpenGLBuffer::StaticDraw, vertexCount, 2);
		Vector_3<float>* vectors = _vectorsBuffer.map();
		Vector_3<float>* vectors_end = vectors + _vectorsBuffer.elementCount() * _vectorsBuffer.verticesPerElement();
		ConstDataBufferAccess<Point3> positionsArray(positions());
		const Point3* coordinates = positionsArray.cbegin();
		for(; vectors != vectors_end; vectors += 4, coordinates += 2) {
			vectors[3] = vectors[0] = (Vector_3<float>)(coordinates[1] - coordinates[0]);
			vectors[1] = vectors[2] = -vectors[0];
		}
		_vectorsBuffer.unmap();
	}

	// Activate the OpenGL shader program.
	if(!_thickLineShader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

	// Set shader uniforms.
	_thickLineShader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
	_thickLineShader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	_thickLineShader->setUniformValue("line_width", (GLfloat)(effectiveLineWidth / (renderer->projParams().projectionMatrix(1,1) * viewportCoords[3])));
	_thickLineShader->setUniformValue("is_perspective", renderer->projParams().isPerspective);
	_thickLineShader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());

	// Fill VBOs.
	_positionsBuffer.uploadData<Point3>(positions(), 2);
	_colorsBuffer.uploadData<ColorA>(colors(), 2);

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, _thickLineShader);
	_vectorsBuffer.bind(renderer, _thickLineShader, "vector", GL_FLOAT, 0, 3);
	if(!renderer->isPicking()) {
		if(_colorsBuffer.isCreated())
			_colorsBuffer.bindColors(renderer, _thinLineShader, 4);
		else
			_colorsBuffer.setUniformColor(renderer, _thinLineShader, uniformColor());
	}
	else {
		GLint pickingBaseID = renderer->registerSubObjectIDs(positions()->size() / 2);
		_thickLineShader->setUniformValue("picking_base_id", pickingBaseID);
	}

	// Bind IBO.
	_indicesBuffer.oglBuffer().bind();

	renderer->glDrawElements(GL_TRIANGLES, _indicesBuffer.elementCount(), GL_UNSIGNED_INT, nullptr);

	// Detach VBOs and IBO.
	_indicesBuffer.oglBuffer().release();
	_positionsBuffer.detachPositions(renderer, _thickLineShader);
	if(!renderer->isPicking())
		_colorsBuffer.detachColors(renderer, _thickLineShader);

	// Reset state.
	_vectorsBuffer.detach(renderer, _thickLineShader, "vector");
	_thickLineShader->release();
}

}	// End of namespace
