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
#include "OpenGLMarkerPrimitive.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

namespace Ovito {

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLMarkerPrimitive::render(OpenGLSceneRenderer* renderer)
{
	// Step out early if there is nothing to render.
	if(!positions() || positions()->size() == 0)
		return;

    // The effective number of primitives being rendered:
    uint32_t verticesPerPrimitive = 0;

	OpenGLShaderHelper shader(renderer);
	switch(shape()) {
	
	case BoxShape:

		if(renderer->isPicking())
			shader.load("marker_box", "marker/marker_box.vert", "marker/marker_box.frag");
		else
			shader.load("marker_box_picking", "marker/marker_box_picking.vert", "marker/marker_box_picking.frag");
		verticesPerPrimitive = 24; // 12 edges of a wireframe cube, 2 vertices per edge.
		break;

	default:
		return;
	}

    // Are we rendering semi-transparent markers?
    bool useBlending = !renderer->isPicking() && color().a() < 1.0;
	if(useBlending) shader.enableBlending();

	if(renderer->isPicking()) {
		// Pass picking base ID to shader.
		shader.setPickingBaseId(renderer->registerSubObjectIDs(positions()->size()));
	}
	else {
		// Pass uniform marker color to fragment shader as a uniform value.
		shader.setUniformValue("color", color());
	}

	// Marker sclaing factor:
	shader.setUniformValue("marker_size", 4.0 / renderer->renderingViewport().height());

	// Upload marker positions.
	QOpenGLBuffer positionsBuffer = shader.uploadDataBuffer(positions(), renderer->currentResourceFrame());
	shader.bindBuffer(positionsBuffer, "position", GL_FLOAT, 3, sizeof(Point_3<float>), 0, OpenGLShaderHelper::PerInstance);

	// Issue instance drawing command.
	OVITO_CHECK_OPENGL(renderer, renderer->glDrawArraysInstanced(GL_LINES, 0, verticesPerPrimitive, positions()->size()));


#if 0
	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
	if(renderer->isPicking())
		shader.load("line_thin_picking", "lines/line_picking.vert", "lines/line.frag");
	else if(colors())
		shader.load("line_thin", "lines/line.vert", "lines/line.frag");
	else
		shader.load("line_thin_uniform_color", "lines/line_uniform_color.vert", "lines/line_uniform_color.frag");

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
#endif
}

}	// End of namespace
