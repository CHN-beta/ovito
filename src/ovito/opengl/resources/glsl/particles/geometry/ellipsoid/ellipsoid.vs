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
uniform bool is_picking_mode;
uniform int picking_base_id;
uniform vec4 selection_color;

// The particle data:
in vec3 position;
in vec3 color;
in float transparency;
in int selection;
in vec3 shape;
in vec4 orientation;
in float particle_radius;

// Output to geometry shader.
out vec4 particle_color_gs;
out vec3 particle_shape_gs;
out vec4 particle_orientation_gs;

void main()
{
	if(!is_picking_mode) {
		// Forward color to geometry shader.
		particle_color_gs = (selection != 0) ? selection_color : vec4(color, 1.0 - transparency);
	}
	else {
		// Compute color from object ID.
		particle_color_gs = pickingModeColor(picking_base_id, gl_VertexID);
	}
		
	// Forward information to geometry shader.
	if(shape != vec3(0))
		particle_shape_gs = shape;
	else
		particle_shape_gs = vec3(particle_radius, particle_radius, particle_radius);
	particle_orientation_gs = orientation;

	// Pass original particle position to geometry shader.
	gl_Position = vec4(position, 1);
}
