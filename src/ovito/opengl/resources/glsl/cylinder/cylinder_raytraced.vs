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
uniform float modelview_uniform_scale;
uniform bool is_picking_mode;
uniform int picking_base_id;

in vec3 position;
in vec4 color;

// The cylinder data:
in vec3 cylinder_base;				// The base position of the cylinder in model coordinates.
in vec3 cylinder_head;				// The head position of the cylinder in model coordinates.
in float cylinder_radius;			// The radius of the cylinder in model coordinates.

// Outputs to geometry shader
out vec4 cylinder_color_gs;			// The base color of the cylinder.
out float cylinder_radius_gs;		// The radius of the cylinder
out vec3 cylinder_view_base_gs;		// Transformed cylinder position in view coordinates
out vec4 cylinder_view_axis_gs;		// Transformed cylinder axis in view coordinates

void main()
{
	if(!is_picking_mode) {
		// Forward color to geometry shader.
		cylinder_color_gs = color;
	}
	else {
		// Compute color from object ID.
		cylinder_color_gs = pickingModeColor(picking_base_id, gl_VertexID);
	}

	// Pass radius to geometry shader.
	cylinder_radius_gs = cylinder_radius * modelview_uniform_scale;

	// Transform cylinder to eye coordinates.
	gl_Position = modelview_matrix * vec4(position, 1.0);
	cylinder_view_base_gs = gl_Position.xyz;
	cylinder_view_axis_gs = modelview_matrix * vec4(cylinder_head - cylinder_base, 0.0);
}
