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

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 modelviewprojection_matrix;
uniform vec3 cubeVerts[14];
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
in vec2 roundness;

// Outputs to fragment shader:
flat out vec4 particle_color_fs;
flat out mat3 view_particle_matrix_fs;
flat out vec3 particle_view_pos_fs;
flat out vec2 particle_exponents_fs;

void main()
{
	// Convert particle orientation from quaternion representation to rotation matrix.
	mat3 rot;
	if(orientation != vec4(0)) {
		// Normalize quaternion.
		vec4 quat = normalize(orientation);
		rot = mat3(
			1.0 - 2.0*(quat.y*quat.y + quat.z*quat.z),
			2.0*(quat.x*quat.y + quat.w*quat.z),
			2.0*(quat.x*quat.z - quat.w*quat.y),
			2.0*(quat.x*quat.y - quat.w*quat.z),
			1.0 - 2.0*(quat.x*quat.x + quat.z*quat.z),
			2.0*(quat.y*quat.z + quat.w*quat.x),
			2.0*(quat.x*quat.z + quat.w*quat.y),
			2.0*(quat.y*quat.z - quat.w*quat.x),
			1.0 - 2.0*(quat.x*quat.x + quat.y*quat.y)
		);
	}
	else {
		rot = mat3(1.0);
	}

	// Compute transformation matrix from superquadrics space to view space.
	vec3 shape2 = shape;
	if(shape2.x == 0.0) shape2.x = particle_radius;
	if(shape2.y == 0.0) shape2.y = particle_radius;
	if(shape2.z == 0.0) shape2.z = particle_radius;
	mat3 particle_model_matrix = rot * mat3(shape2.x, 0, 0, 0, shape2.y, 0, 0, 0, shape2.z);
	view_particle_matrix_fs = inverse_mat3(mat3(modelview_matrix) * particle_model_matrix);

	// The x-component of the input vector is exponent 'e', the y-component is 'n'.
	particle_exponents_fs.x = 2.0 / (roundness.x > 0.0 ? roundness.x : 1.0);
	particle_exponents_fs.y = 2.0 / (roundness.y > 0.0 ? roundness.y : 1.0);

	if(!is_picking_mode) {
		// Forward color to fragment shader.
		particle_color_fs = (selection != 0) ? selection_color : vec4(color, 1.0 - transparency);
	}
	else {
		// Compute color from object ID.
		particle_color_fs = pickingModeColor(picking_base_id, gl_VertexID / 14); 
	}

	// Transform and project vertex.
	int cubeCorner = gl_VertexID % 14;
	vec3 delta = particle_model_matrix * cubeVerts[cubeCorner];
	gl_Position = modelviewprojection_matrix * vec4(position + delta, 1);

	particle_view_pos_fs = (modelview_matrix * vec4(position, 1)).xyz;
}
