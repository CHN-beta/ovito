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

layout(points) in;
layout(triangle_strip, max_vertices=24) out;

// Inputs from calling program:
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelviewprojection_matrix;

// Inputs from vertex shader
in vec4 particle_color_gs[1];
in mat3 particle_view_matrix_gs[1];
in vec2 particle_exponents_gs[1];

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out mat3 view_particle_matrix_fs;
flat out vec3 particle_view_pos_fs;
flat out vec2 particle_exponents_fs;

void main()
{
	vec3 particle_view_pos = (modelview_matrix * gl_in[0].gl_Position).xyz;

#if 0
	// This code leads, which generates a single triangle strip for the cube, seems to be
	// incompatible with the Intel graphics driver on Linux.

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, particle_shape_gs[0].y, -particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, -particle_shape_gs[0].y, -particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, -particle_shape_gs[0].y, -particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(particle_shape_gs[0].x, particle_shape_gs[0].y, -particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, particle_shape_gs[0].y, -particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, -particle_shape_gs[0].y, -particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	particle_quadric_fs = quadric;
	particle_view_pos_fs = particle_view_pos;
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(rot * vec3(-particle_shape_gs[0].x, -particle_shape_gs[0].y, particle_shape_gs[0].z), 0));
	EmitVertex();

#else

	// Generate 6 triangle strips to be compatible with the Intel graphics driver on Linux.

	vec4 dx = projection_matrix * vec4(particle_view_matrix_gs[0] * vec3(1, 0, 0), 0);
	vec4 dy = projection_matrix * vec4(particle_view_matrix_gs[0] * vec3(0, 1, 0), 0);
	vec4 dz = projection_matrix * vec4(particle_view_matrix_gs[0] * vec3(0, 0, 2), 0);
	vec4 corner = (modelviewprojection_matrix * gl_in[0].gl_Position) - (dx + dy + dz);
	dx *= 2.0;
	dy *= 2.0;
	dz *= 2.0;
	mat3 view_particle_matrix = inverse(particle_view_matrix_gs[0]);

	// -X
	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dz;
	EmitVertex();
	EndPrimitive();

	// +X
	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dy + dz;
	EmitVertex();
	EndPrimitive();

	// -Y
	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dz;
	EmitVertex();
	EndPrimitive();

	// +Y
	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy + dx + dz;
	EmitVertex();
	EndPrimitive();

	// -Z
	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx;
	EmitVertex();
	EndPrimitive();

	// +Z
	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	view_particle_matrix_fs = view_particle_matrix;
	particle_view_pos_fs = particle_view_pos;
	particle_exponents_fs = particle_exponents_gs[0];
	gl_Position = corner + dx + dy + dz;
	EmitVertex();
	EndPrimitive();

#endif
}
