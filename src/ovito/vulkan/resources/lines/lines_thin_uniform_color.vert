#version 440

// Push constants:
layout(push_constant) uniform constants {
    mat4 mvp;
    vec4 color;
} PushConstants;

// Inputs:
layout(location = 0) in vec4 position;

// Outputs:
out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = PushConstants.mvp * position;
}
