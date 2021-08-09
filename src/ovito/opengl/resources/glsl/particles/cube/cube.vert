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
uniform vec3 unit_cube_triangle_strip[14];
uniform vec3 unit_cube_strip_normals[14];

// Outputs:
flat out vec4 color_fs;
flat out vec3 flat_normal_fs;

void main()
{
    // The index of the cube corner.
    int corner = gl_VertexID;

	// Apply model-view-projection matrix to particle position displaced by the cube vertex position.
    gl_Position = modelview_projection_matrix * vec4(position + unit_cube_triangle_strip[corner] * radius, 1.0);

    // Forward particle color to fragment shader.
    color_fs = color;

    // Transform local vertex normal.
    flat_normal_fs = vec3(normal_tm * vec4(unit_cube_strip_normals[corner], 0.0));
}
