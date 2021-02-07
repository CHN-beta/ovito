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
uniform mat4 projection_matrix;
uniform mat4 inverse_projection_matrix;
uniform bool is_perspective;
uniform vec2 viewport_origin;		// Specifies the transformation from screen coordinates to viewport coordinates.
uniform vec2 inverse_viewport_size;	// Specifies the transformation from screen coordinates to viewport coordinates.
uniform bool is_picking_mode;

// Input from vertex shader:
flat in vec4 particle_color_fs;
flat in vec3 surface_normal_fs;
out vec4 FragColor;

void main()
{
	if(!is_picking_mode) {
		// Calculate viewing ray direction in view space.
		if(is_perspective) {
			// Calculate the pixel coordinate in viewport space.
			vec2 view_c = ((gl_FragCoord.xy - viewport_origin) * inverse_viewport_size) - 1.0;
			vec3 ray_dir = normalize(vec3(inverse_projection_matrix * vec4(view_c.x, view_c.y, 1.0, 1.0)));
			FragColor = shadeSurfaceColor(surface_normal_fs, ray_dir, particle_color_fs.rgb, particle_color_fs.a);
		}
		else {
			FragColor = shadeSurfaceColorQuick(surface_normal_fs, particle_color_fs.rgb, particle_color_fs.a);
		}
	}
	else {
		FragColor = particle_color_fs;
	}
}
