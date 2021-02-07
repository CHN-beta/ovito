////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include "OpenGLMarkerPrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLMarkerPrimitive::OpenGLMarkerPrimitive(OpenGLSceneRenderer* renderer, MarkerShape shape) :
	MarkerPrimitive(shape)
{
	QString prefix = renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles") : QStringLiteral(":/openglrenderer");

	// Initialize OpenGL shaders.
	if(shape == BoxShape) {
		_shader = renderer->loadShaderProgram("box_marker",
				prefix + "/glsl/markers/box_lines.vs",
				prefix + "/glsl/markers/marker.fs");
	}
	else {
		_shader = renderer->loadShaderProgram("dot_marker",
				prefix + "/glsl/markers/marker.vs",
				prefix + "/glsl/markers/marker.fs");
	}
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLMarkerPrimitive::render(OpenGLSceneRenderer* renderer)
{
	if(!positions() || positions()->size() == 0)
		return;

#ifndef Q_OS_WASM	
	// Load OpenGL shader program.
	if(!_shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader program."));

	int verticesPerMarker = 1;
	if(shape() == BoxShape)
		verticesPerMarker = 24;

	// Fill VBOs.
	_positionsBuffer.uploadData<Point3>(positions(), verticesPerMarker);

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, _shader);

	// Set up state.
	_shader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());
	if(!renderer->isPicking()) {
		_shader->setUniformValue("color", color().r(), color().g(), color().b(), color().a());
	}
	else {
		GLint pickingBaseID = renderer->registerSubObjectIDs(positions()->size());
		_shader->setUniformValue("picking_base_id", pickingBaseID);
	}

	if(shape() == DotShape) {
		OVITO_CHECK_OPENGL(renderer, renderer->glPointSize(3));
		_shader->setUniformValue("modelview_projection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
		renderer->glDrawArrays(GL_POINTS, 0, positions()->size());
	}
	else if(shape() == BoxShape) {
		_shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
		_shader->setUniformValue("viewprojection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->projParams().viewMatrix));
		_shader->setUniformValue("model_matrix", (QMatrix4x4)renderer->worldTransform());
		_shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
		GLint viewportCoords[4];
		renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
		_shader->setUniformValue("marker_size", 4.0f / viewportCoords[3]);

		static const QVector3D cubeVerts[] = {
				{-1, -1, -1}, { 1,-1,-1},
				{-1, -1,  1}, { 1,-1, 1},
				{-1, -1, -1}, {-1,-1, 1},
				{ 1, -1, -1}, { 1,-1, 1},
				{-1,  1, -1}, { 1, 1,-1},
				{-1,  1,  1}, { 1, 1, 1},
				{-1,  1, -1}, {-1, 1, 1},
				{ 1,  1, -1}, { 1, 1, 1},
				{-1, -1, -1}, {-1, 1,-1},
				{ 1, -1, -1}, { 1, 1,-1},
				{ 1, -1,  1}, { 1, 1, 1},
				{-1, -1,  1}, {-1, 1, 1}
		};
		_shader->setUniformValueArray("cubeVerts", cubeVerts, 24);

		renderer->glDrawArrays(GL_LINES, 0, _positionsBuffer.elementCount() * _positionsBuffer.verticesPerElement());
	}

	_positionsBuffer.detachPositions(renderer, _shader);

	// Reset state.
	_shader->release();
#endif
}

}	// End of namespace
