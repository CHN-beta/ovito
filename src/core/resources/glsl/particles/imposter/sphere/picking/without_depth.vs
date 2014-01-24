///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

// Inputs from calling program:
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform vec2 imposter_texcoords[6];
uniform vec4 imposter_voffsets[6];
uniform int pickingBaseID;

#if __VERSION__ >= 130

	// The input particle data:
	in vec3 position;
	in float particle_radius;
	
	// Output passed to fragment shader.
	flat out vec4 particle_color_fs;
	out vec2 texcoords;

#else

	// The input particle data:
	attribute float particle_radius;
	attribute float vertexID;
	#define gl_VertexID int(vertexID)

	#define particle_color_fs gl_FrontColor
#endif

void main()
{
	// Compute color from object ID.
	int objectID = pickingBaseID + (gl_VertexID / 6);
#if __VERSION__ >= 130
	particle_color_fs = vec4(
		float(objectID & 0xFF) / 255.0, 
		float((objectID >> 8) & 0xFF) / 255.0, 
		float((objectID >> 16) & 0xFF) / 255.0, 
		float((objectID >> 24) & 0xFF) / 255.0);
		
	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(position, 1);

	// Assign texture coordinates. 
	texcoords = imposter_texcoords[gl_VertexID % 6];

#else
	particle_color_fs = vec4(
		float(mod(objectID, 0x100)) / 255.0, 
		float(mod(objectID / 0x100, 0x100)) / 255.0, 
		float(mod(objectID / 0x10000, 0x100)) / 255.0, 
		float(mod(objectID / 0x1000000, 0x100)) / 255.0);		

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * gl_Vertex;
#endif

	// Transform and project particle position.
	gl_Position = projection_matrix * (eye_position + particle_radius * imposter_voffsets[gl_VertexID % 6]);
}
