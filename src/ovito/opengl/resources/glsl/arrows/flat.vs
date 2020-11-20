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

uniform int picking_base_id;
uniform bool is_picking_mode;

in vec3 position;
in vec4 color;
in vec3 cylinder_axis;
in float cylinder_radius;

out vec4 color_gs;
out vec3 cylinder_axis_gs;
out float cylinder_radius_gs;

void main()
{
	if(!is_picking_mode) {
		// Forward color to geometry shader.
		color_gs = color;
	}
	else {
		// Compute color from object ID.
		color_gs = pickingModeColor(picking_base_id, gl_VertexID);
	}		

	cylinder_axis_gs = cylinder_axis;
	cylinder_radius_gs = cylinder_radius;
	gl_Position = vec4(position, 1);
}
