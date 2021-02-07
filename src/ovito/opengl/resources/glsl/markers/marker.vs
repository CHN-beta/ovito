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

uniform mat4 modelview_projection_matrix;
uniform int picking_base_id;
uniform bool is_picking_mode;
uniform vec4 color;

in vec3 position;
flat out vec4 vertex_color_fs;

void main()
{
	if(!is_picking_mode) {
		// Forward color to fragment shader.
		vertex_color_fs = color;
	}
	else {
		// Compute color from object ID.
		vertex_color_fs = pickingModeColor(picking_base_id, gl_VertexID);
	}

	gl_Position = modelview_projection_matrix * vec4(position, 1.0);
}
