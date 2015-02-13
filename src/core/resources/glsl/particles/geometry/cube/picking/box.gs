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
uniform mat4 modelview_matrix;
uniform mat4 modelviewprojection_matrix;

// Inputs from vertex shader
in vec4 particle_color_gs[1];
in vec3 particle_shape_gs[1];

// Outputs to fragment shader
flat out vec4 particle_color_fs;

void main()
{
	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, particle_shape_gs[0].y, -particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, -particle_shape_gs[0].y, -particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, -particle_shape_gs[0].y, -particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_shape_gs[0].x, particle_shape_gs[0].y, -particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, particle_shape_gs[0].y, -particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, -particle_shape_gs[0].y, -particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z, 0));
	EmitVertex();
#endif
}
