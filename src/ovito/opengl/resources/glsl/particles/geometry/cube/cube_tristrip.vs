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

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 modelviewprojection_matrix;
uniform mat3 normal_matrix;
uniform vec3 cubeVerts[14];
uniform vec3 normals[14];
uniform bool is_picking_mode;
uniform int picking_base_id;
uniform vec4 selection_color;

// The particle data:
in vec3 position;
in vec3 color;
in float transparency;
in float particle_radius;
in int selection;

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out vec3 surface_normal_fs;

void main()
{
	// Transform and project vertex.
	int cubeCorner = gl_VertexID % 14;
	gl_Position = modelviewprojection_matrix * vec4(position + cubeVerts[cubeCorner] * particle_radius, 1);

	if(!is_picking_mode) {
		// Forward color to fragment shader.
		particle_color_fs = (selection != 0) ? selection_color : vec4(color, 1.0 - transparency);

		// Determine face normal.
		surface_normal_fs = normal_matrix * normals[cubeCorner];
	}
	else {
		// Compute color from object ID.
		particle_color_fs = pickingModeColor(picking_base_id, gl_VertexID / 14); 
	}
}
