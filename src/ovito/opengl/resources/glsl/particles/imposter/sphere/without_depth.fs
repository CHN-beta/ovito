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

// Input from calling program:
uniform bool is_picking_mode;

// Input from vertex shader:
flat in vec4 particle_color_fs;
in vec2 texcoords;

// Output fragment color:
out vec4 FragColor;

void main()
{
	// Test if fragment is within the unit circle.
	vec2 shifted_coords = texcoords - vec2(0.5, 0.5);
	float rsq = dot(shifted_coords, shifted_coords);
	if(rsq >= 0.25) discard;

	if(!is_picking_mode) {
		// Calculate surface normal in view coordinate system.
		vec3 surface_normal;
		surface_normal.x =  2.0 * shifted_coords.x;
		surface_normal.y = -2.0 * shifted_coords.y;
		surface_normal.z = sqrt(1.0 - 4.0 * rsq);

		FragColor = shadeSurfaceColorQuick(surface_normal, particle_color_fs.rgb, particle_color_fs.a);
	}
	else {
		FragColor = particle_color_fs;
	}
}
