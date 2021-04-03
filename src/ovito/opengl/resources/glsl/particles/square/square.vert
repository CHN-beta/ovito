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

// Outputs:
flat out vec4 color_fs;

void main()
{
	// Const array of vertex positions for the quad triangle strip.
	const vec2 quad[4] = vec2[4](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
	);

    // The index of the particle being rendered.
    int particle_index = gl_InstanceID;

    // The index of the quad corner.
    int corner = gl_VertexID;

    // Transform particle center to view space.
	vec3 eye_position = (modelview_matrix * vec4(position, 1.0)).xyz;

    // Apply additional scaling due to model-view transformation to particle radius. 
    float viewspace_radius = radius * length(modelview_matrix[0]);

	// Project corner vertex.
    gl_Position = projection_matrix * (vec4(eye_position, 1.0) + vec4(quad[corner] * viewspace_radius, 0.0, 0.0));

    // Forward particle color to fragment shader.
    color_fs = color;
}
