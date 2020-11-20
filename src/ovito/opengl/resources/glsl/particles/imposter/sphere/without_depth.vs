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
uniform mat4 modelviewprojection_matrix;
uniform float radius_scalingfactor;
uniform bool is_picking_mode;
uniform int picking_base_id;

// The particle data:
in vec3 position;
in vec4 color;
in float particle_radius;

// Output to geometry shader.
out vec4 particle_color_gs;
out float particle_radius_gs;

void main()
{
	particle_radius_gs = particle_radius * radius_scalingfactor;
	gl_Position = modelviewprojection_matrix * vec4(position, 1.0);

	if(!is_picking_mode) {
		particle_color_gs = color;
	}
	else {
		// Compute color from object ID.
		particle_color_gs = pickingModeColor(picking_base_id, gl_VertexID);
	}
}
