#version 440

// Push constants:
layout(push_constant) uniform constants {
    mat4 mvp;
    int pickingBaseId;
} PushConstants;

// Inputs:
layout(location = 0) in vec4 position;

// Outputs:
layout(location = 0) out vec4 color_fs;
out gl_PerVertex { vec4 gl_Position; };

vec4 pickingModeColor(in int baseID, in int primitiveID)
{
	// Compute color from object ID.
	int objectID = baseID + primitiveID;

	return vec4(
		float(objectID & 0xFF) / 255.0,
		float((objectID >> 8) & 0xFF) / 255.0,
		float((objectID >> 16) & 0xFF) / 255.0,
		float((objectID >> 24) & 0xFF) / 255.0);
}

void main()
{
    // Compute color from object ID.
    color_fs = pickingModeColor(PushConstants.pickingBaseId, gl_VertexIndex / 2);

    gl_Position = PushConstants.mvp * position;
}
