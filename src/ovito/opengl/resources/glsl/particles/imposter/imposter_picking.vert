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
#include "../../picking.glsl"

// Inputs:
in vec4 position;
in float radius;

// Outputs:
flat out vec4 color_fs;
out vec2 uv_fs;
flat out vec2 radius_and_eyez_fs;

void main()
{
	// Const array of vertex positions for the quad triangle strip.
	const vec2 quad[4] = vec2[4](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
	);

    // The index of the quad corner.
    int corner = gl_VertexID;

    // Transform particle center to view space.
	vec3 eye_position = (modelview_matrix * position).xyz;

    // Apply additional scaling due to model-view transformation to particle radius. 
    radius_and_eyez_fs.x = radius * length(modelview_matrix[0]);

	// Project corner vertex.
    gl_Position = projection_matrix * (vec4(eye_position, 1.0) + vec4(quad[corner] * radius_and_eyez_fs.x, 0.0, 0.0));

    // Compute color from object ID.
    color_fs = pickingModeColor(gl_InstanceID);

    // Pass UV quad coordinates to fragment shader.
    uv_fs = quad[corner];

	// Pass particle z-position in view space to fragment shader.
	radius_and_eyez_fs.y = eye_position.z;
}
