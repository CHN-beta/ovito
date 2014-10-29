///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

layout(points) in;
layout(triangle_strip, max_vertices=14) out;

// Inputs from calling program:
uniform mat4 projection_matrix;
uniform vec3 cubeVerts[14] = vec3[14](
vec3( 1,  1,  1),
vec3( 1, -1,  1),
vec3( 1,  1, -1),
vec3( 1, -1, -1),
vec3(-1, -1, -1),
vec3( 1, -1,  1),
vec3(-1, -1,  1),
vec3( 1,  1,  1),
vec3(-1,  1,  1),
vec3( 1,  1, -1),
vec3(-1,  1, -1),
vec3(-1, -1, -1),
vec3(-1,  1,  1),
vec3(-1, -1,  1)
);

// Inputs from vertex shader
in vec4 particle_color_gs[1];
in float particle_radius_gs[1];

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out float particle_radius_squared_fs;
flat out vec3 particle_view_pos_fs;

void main()
{
	for(int vertex = 0; vertex < 14; vertex++) {
		particle_view_pos_fs = vec3(gl_in[0].gl_Position); 
		particle_color_fs = particle_color_gs[0];
		particle_radius_squared_fs = particle_radius_gs[0] * particle_radius_gs[0];
		gl_Position = projection_matrix * vec4(particle_view_pos_fs + cubeVerts[vertex] * particle_radius_gs[0], 1);
		EmitVertex();
	}
}
