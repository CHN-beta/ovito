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

	OpenGLShaderHelper shader(renderer);
	switch(shape()) {
	
	case BoxShape:

		if(renderer->isPicking())
			shader.load("marker_box", "marker/marker_box.vert", "marker/marker_box.frag");
		else
			shader.load("marker_box_picking", "marker/marker_box_picking.vert", "marker/marker_box_picking.frag");
		shader.setVerticesPerInstance(24); // 12 edges of a wireframe cube, 2 vertices per edge.
		break;

	default:
		return;
	}
	shader.setInstanceCount(positions()->size());

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
	QOpenGLBuffer positionsBuffer = shader.uploadDataBuffer(positions(), OpenGLShaderHelper::PerInstance);
	shader.bindBuffer(positionsBuffer, "position", GL_FLOAT, 3, sizeof(Point_3<float>), 0, OpenGLShaderHelper::PerInstance);

	// Issue instance drawing command.
	shader.drawArrays(GL_LINES);
}

}	// End of namespace
