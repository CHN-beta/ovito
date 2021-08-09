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

layout(points) in;
layout(triangle_strip, max_vertices=14) out;

#include "../../global_uniforms.glsl"

// Inputs:
in vec3 position_gs[1];
in float radius_gs[1];
in vec4 color_gs[1];
uniform vec3 unit_cube_triangle_strip[14];

// Outputs:
flat out vec4 color_fs;
flat out vec3 particle_view_pos_fs;
flat out float particle_radius_squared_fs;
noperspective out vec3 ray_origin;
noperspective out vec3 ray_dir;

void main()
{
    for(int corner = 0; corner < 14; corner++) 
    {
        // Apply model-view-projection matrix to particle position displaced by the cube vertex position.
        gl_Position = modelview_projection_matrix * vec4(position_gs[0] + unit_cube_triangle_strip[corner] * radius_gs[0], 1.0);

        // Forward particle color to fragment shader.
        color_fs = color_gs[0];

        // Pass particle radius and center position to fragment shader.
        float radius_scalingfactor = length(modelview_matrix[0]);
        particle_radius_squared_fs = (radius_gs[0] * radius_gs[0]) * (radius_scalingfactor * radius_scalingfactor);
        particle_view_pos_fs = (modelview_matrix * vec4(position_gs[0], 1.0)).xyz;

        // Calculate ray passing through the vertex (in view space).
        calculate_view_ray(vec2(gl_Position.x / gl_Position.w, gl_Position.y / gl_Position.w), ray_origin, ray_dir);

        EmitVertex();
    }
    EndPrimitive();
}
