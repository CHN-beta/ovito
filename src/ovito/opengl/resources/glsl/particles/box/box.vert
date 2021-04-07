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

#include "../../global_uniforms.glsl"

// Inputs:
in vec3 position;
in float radius;
in vec4 color;
in mat4 shape_orientation;

// Outputs:
flat out vec4 color_fs;
flat out vec3 flat_normal_fs;

void main()
{
	// Const array of vertex positions for the unit cube triangle strip.
	const vec3 cube[14] = vec3[14](
        vec3( 1.0,  1.0,  1.0),
        vec3( 1.0, -1.0,  1.0),
        vec3( 1.0,  1.0, -1.0),
        vec3( 1.0, -1.0, -1.0),
        vec3(-1.0, -1.0, -1.0),
        vec3( 1.0, -1.0,  1.0),
        vec3(-1.0, -1.0,  1.0),
        vec3( 1.0,  1.0,  1.0),
        vec3(-1.0,  1.0,  1.0),
        vec3( 1.0,  1.0, -1.0),
        vec3(-1.0,  1.0, -1.0),
        vec3(-1.0, -1.0, -1.0),
        vec3(-1.0,  1.0,  1.0),
        vec3(-1.0, -1.0,  1.0)
	);

	// Const array of vertex normals for the unit cube triangle strip.
    // Note the difference between Vulkan and OpenGL.
	const vec3 normals[14] = vec3[14](
        vec3( 1.0,  0.0,  0.0),
        vec3( 1.0,  0.0,  0.0),
        vec3( 1.0,  0.0,  0.0),
        vec3( 1.0,  0.0,  0.0),
        vec3( 0.0,  0.0, -1.0),
        vec3( 0.0, -1.0,  0.0),
        vec3( 0.0, -1.0,  0.0),
        vec3( 0.0,  0.0,  1.0),
        vec3( 0.0,  0.0,  1.0),
        vec3( 0.0,  1.0,  0.0),
        vec3( 0.0,  1.0,  0.0),
        vec3( 0.0,  0.0, -1.0),
        vec3(-1.0,  0.0,  0.0),
        vec3(-1.0,  0.0,  0.0)
    );

    // The index of the box corner.
    int corner = gl_VertexID;

    // Compute rotated and scaled unit cube corner coordinates.
    vec4 scaled_corner = vec4(position, 1.0) + (shape_orientation * vec4(cube[corner], 0.0));

	// Apply model-view-projection matrix to particle position displaced by the cube vertex position.
    gl_Position = modelview_projection_matrix * scaled_corner;

    // Forward particle color to fragment shader.
    color_fs = color;

    // Transform local vertex normal.
    flat_normal_fs = normalize(vec3(normal_tm * shape_orientation * vec4(normals[corner], 0.0)));
}
