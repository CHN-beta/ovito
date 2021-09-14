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
#include "OpenGLLinePrimitive.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

namespace Ovito {

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLLinePrimitive::render(OpenGLSceneRenderer* renderer)
{
	// Step out early if there is nothing to render.
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
	OVITO_REPORT_OPENGL_ERRORS(renderer);

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
	if(renderer->isPicking())
		shader.load("line_thin_picking", "lines/line_picking.vert", "lines/line.frag");
	else if(colors())
		shader.load("line_thin", "lines/line.vert", "lines/line.frag");
	else
		shader.load("line_thin_uniform_color", "lines/line_uniform_color.vert", "lines/line_uniform_color.frag");

	shader.setVerticesPerInstance(positions()->size());
	shader.setInstanceCount(1);

	// Upload vertex positions.
	QOpenGLBuffer positionsBuffer = shader.uploadDataBuffer(positions(), OpenGLShaderHelper::PerVertex);
	shader.bindBuffer(positionsBuffer, "position", GL_FLOAT, 3, sizeof(Point_3<float>), 0, OpenGLShaderHelper::PerVertex);

	if(!renderer->isPicking()) {
		if(colors()) {
			OVITO_ASSERT(colors()->size() == positions()->size());
			// Upload vertex colors.
			QOpenGLBuffer colorsBuffer = shader.uploadDataBuffer(colors(), OpenGLShaderHelper::PerVertex);
			shader.bindBuffer(colorsBuffer, "color", GL_FLOAT, 4, sizeof(ColorAT<float>), 0, OpenGLShaderHelper::PerVertex);
		}
		else {
            // Pass uniform line color to fragment shader as a uniform value.
            shader.setUniformValue("color", uniformColor());
		}
	}
	else {
		// Pass picking base ID to shader.
		shader.setPickingBaseId(renderer->registerSubObjectIDs(positions()->size() / 2));
	}

	// Issue line drawing command.
	shader.drawArrays(GL_LINES);
}

/******************************************************************************
* Renders the lines using triangle strips.
******************************************************************************/
void OpenGLLinePrimitive::renderThickLines(OpenGLSceneRenderer* renderer)
{
	// Effective line width.
	FloatType effectiveLineWidth = (lineWidth() <= 0) ? renderer->devicePixelRatio() : lineWidth();

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
	if(renderer->isPicking())
		shader.load("line_thick_picking", "lines/thick_line_picking.vert", "lines/line.frag");
	else if(colors())
		shader.load("line_thick", "lines/thick_line.vert", "lines/line.frag");
	else
		shader.load("line_thick_uniform_color", "lines/thick_line_uniform_color.vert", "lines/line_uniform_color.frag");

	shader.setVerticesPerInstance(4);
	shader.setInstanceCount(positions()->size() / 2);

    // Put start/end vertex positions into one combined Vulkan buffer.
	QOpenGLBuffer positionsBuffer = shader.uploadDataBuffer(positions(), OpenGLShaderHelper::PerInstance);
	shader.bindBuffer(positionsBuffer, "position_from", GL_FLOAT, 3, 2 * sizeof(Point_3<float>), 0, OpenGLShaderHelper::PerInstance);
	shader.bindBuffer(positionsBuffer, "position_to", GL_FLOAT, 3, 2 * sizeof(Point_3<float>), sizeof(Point_3<float>), OpenGLShaderHelper::PerInstance);

	if(!renderer->isPicking()) {
		if(colors()) {
			OVITO_ASSERT(colors()->size() == positions()->size());
			// Upload vertex colors.
			QOpenGLBuffer colorsBuffer = shader.uploadDataBuffer(colors(), OpenGLShaderHelper::PerInstance);
			shader.bindBuffer(colorsBuffer, "color_from", GL_FLOAT, 4, 2 * sizeof(ColorAT<float>), 0, OpenGLShaderHelper::PerInstance);
			shader.bindBuffer(colorsBuffer, "color_to", GL_FLOAT, 4, 2 * sizeof(ColorAT<float>), sizeof(ColorAT<float>), OpenGLShaderHelper::PerInstance);
		}
		else {
            // Pass uniform line color to fragment shader as a uniform value.
            shader.setUniformValue("color", uniformColor());
		}
	}
	else {
		// Pass picking base ID to shader.
		shader.setPickingBaseId(renderer->registerSubObjectIDs(positions()->size() / 2));
	}
	
	// Compute line width in viewport space.
	shader.setUniformValue("line_thickness", effectiveLineWidth / renderer->viewportRect().height());

	// Issue instanced drawing command.
	shader.drawArrays(GL_TRIANGLE_STRIP);
}

}	// End of namespace
