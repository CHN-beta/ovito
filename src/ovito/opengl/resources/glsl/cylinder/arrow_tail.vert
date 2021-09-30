////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
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

#include "../global_uniforms.glsl"

// Inputs:
in vec3 base;
in vec3 head;
in float radius;
in vec4 color1;
in vec4 color2;
uniform vec3 unit_cube_triangle_strip[14];

// Outputs:
flat out vec4 color_fs;
flat out vec3 cylinder_view_base;		// Transformed cylinder position in view coordinates
flat out vec3 cylinder_view_axis;		// Transformed cylinder axis in view coordinates
flat out float cylinder_radius_sq_fs;	// The squared radius of the cylinder
flat out float cylinder_length;			// The length of the cylinder
noperspective out vec3 ray_origin;
noperspective out vec3 ray_dir;

void main()
{
    // The index of the box corner.
    int corner = gl_VertexID;

    float arrowHeadRadius = radius * 2.5;
    float arrowHeadLength = (arrowHeadRadius * 1.8);

    // Set up an axis tripod that is aligned with the cylinder.
    mat3 orientation_tm;
    orientation_tm[2] = head - base;
    float len = length(orientation_tm[2]);
    if(len != 0.0 && arrowHeadLength < len) {
        orientation_tm[2] *= 1.0 - arrowHeadLength / len;
        if(orientation_tm[2].y != 0.0 || orientation_tm[2].x != 0.0)
            orientation_tm[0] = normalize(vec3(orientation_tm[2].y, -orientation_tm[2].x, 0.0)) * radius;
        else
            orientation_tm[0] = normalize(vec3(-orientation_tm[2].z, 0.0, orientation_tm[2].x)) * radius;
        orientation_tm[1] = normalize(cross(orientation_tm[2], orientation_tm[0])) * radius;
    }
    else {
        orientation_tm = mat3(0.0);
    }

	// Apply model-view-projection matrix to box vertex position.
    gl_Position = modelview_projection_matrix * vec4(base + (orientation_tm * unit_cube_triangle_strip[corner]), 1.0);

    // Forward cylinder color to fragment shader.
    color_fs = color1;

    // Apply additional scaling to cylinder radius due to model-view transformation. 
    float viewspace_radius = radius * length(modelview_matrix[0]);

	// Pass square of cylinder radius to fragment shader.
	cylinder_radius_sq_fs = viewspace_radius * viewspace_radius;

	// Transform cylinder to eye coordinates.
	cylinder_view_base = (modelview_matrix * vec4(base, 1.0)).xyz;
	cylinder_view_axis = (modelview_matrix * vec4(orientation_tm[2], 0.0)).xyz;

	// Pass cylinder length to fragment shader.
	cylinder_length = length(cylinder_view_axis);

    // Calculate ray passing through the vertex (in view space).
    calculate_view_ray(vec2(gl_Position.x / gl_Position.w, gl_Position.y / gl_Position.w), ray_origin, ray_dir);
}
