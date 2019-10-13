////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
layout(triangle_strip, max_vertices=24) out;

// Inputs from calling program:
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 modelviewprojection_matrix;
uniform mat3 normal_matrix;

// Inputs from vertex shader
in vec4 particle_color_gs[1];
in float particle_radius_gs[1];

// Outputs to fragment shader
flat out vec4 particle_color_fs;
flat out vec3 surface_normal_fs;

void main()
{
#if 0
	// This code leads, which generates a single triangle strip for the cube, seems to be
	// incompatible with the Intel graphics driver on Linux.

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], -particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[2];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[1];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[1];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[2];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[2];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[1];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(particle_radius_gs[0], particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[1];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[2];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], -particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[0];
	gl_Position = modelviewprojection_matrix *
		(gl_in[0].gl_Position + vec4(-particle_radius_gs[0], -particle_radius_gs[0], particle_radius_gs[0], 0));
	EmitVertex();

	EndPrimitive();

#else

	// Generate 6 triangle strips to be compatible with the Intel graphics driver on Linux.

	vec4 corner = modelviewprojection_matrix * (gl_in[0].gl_Position - vec4(particle_radius_gs[0], particle_radius_gs[0], particle_radius_gs[0], 0));
	vec4 dx = modelviewprojection_matrix * vec4(2 * particle_radius_gs[0], 0, 0, 0);
	vec4 dy = modelviewprojection_matrix * vec4(0, 2 * particle_radius_gs[0], 0, 0);
	vec4 dz = modelviewprojection_matrix * vec4(0, 0, 2 * particle_radius_gs[0], 0);

	// -X
	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[0];
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[0];
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[0];
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[0];
	gl_Position = corner + dz;
	EmitVertex();
	EndPrimitive();

	// +X
	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = corner + dx;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = corner + dx + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[0];
	gl_Position = corner + dx + dy + dz;
	EmitVertex();
	EndPrimitive();

	// -Y
	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[1];
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[1];
	gl_Position = corner + dx;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[1];
	gl_Position = corner + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[1];
	gl_Position = corner + dx + dz;
	EmitVertex();
	EndPrimitive();

	// +Y
	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[1];
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[1];
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[1];
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[1];
	gl_Position = corner + dy + dx + dz;
	EmitVertex();
	EndPrimitive();

	// -Z
	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[2];
	gl_Position = corner + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[2];
	gl_Position = corner + dx + dy;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[2];
	gl_Position = corner;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = -normal_matrix[2];
	gl_Position = corner + dx;
	EmitVertex();
	EndPrimitive();

	// +Z
	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[2];
	gl_Position = corner + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[2];
	gl_Position = corner + dx + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[2];
	gl_Position = corner + dy + dz;
	EmitVertex();

	particle_color_fs = particle_color_gs[0];
	surface_normal_fs = normal_matrix[2];
	gl_Position = corner + dx + dy + dz;
	EmitVertex();
	EndPrimitive();

#endif
}
