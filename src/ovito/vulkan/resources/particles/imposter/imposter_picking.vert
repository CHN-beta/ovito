#version 440

#include "../../picking.glsl"

// Push constants:
layout(push_constant) uniform constants {
    mat4 projection_matrix;
    layout(row_major) mat4x3 modelview_matrix;
    int pickingBaseId;
} PushConstants;

// Inputs:
layout(location = 0) in vec4 position;
layout(location = 1) in float radius;

// Outputs:
layout(location = 0) out vec4 color_fs;
layout(location = 1) out vec2 uv_fs;
layout(location = 2) out vec2 radius_and_eyez_fs;
out gl_PerVertex { vec4 gl_Position; };

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
    int particle_index = gl_InstanceIndex;

    // The index of the quad corner.
    int corner = gl_VertexIndex;

    // Transform particle center to view space.
	vec3 eye_position = PushConstants.modelview_matrix * position;

    // Apply additional scaling due to model-view transformation to particle radius. 
    radius_and_eyez_fs.x = radius * length(PushConstants.modelview_matrix[0]);

	// Project corner vertex.
    gl_Position = PushConstants.projection_matrix * (vec4(eye_position, 1.0) + vec4(quad[corner] * radius_and_eyez_fs.x, 0.0, 0.0));

    // Compute color from object ID.
    color_fs = pickingModeColor(PushConstants.pickingBaseId, particle_index);

    // Pass UV quad coordinates to fragment shader.
    uv_fs = quad[corner];

	// Pass particle z-position in view space to fragment shader.
	radius_and_eyez_fs.y = eye_position.z;
}
