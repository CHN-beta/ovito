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

uniform mat4 modelview_projection_matrix;
uniform int pickingBaseID;
uniform int verticesPerElement;

#if __VERSION__ < 130
	#define in attribute
	#define out varying
	#define flat

	attribute float vertexID;
	#define gl_VertexID int(vertexID)
#endif

in vec3 vertex_pos;

flat out vec4 vertex_color_out;

void main()
{
	// Compute color from object ID.
	int objectID = pickingBaseID + (gl_VertexID / verticesPerElement);
#if __VERSION__ >= 130
	vertex_color_out = vec4(
		float(objectID & 0xFF) / 255.0, 
		float((objectID >> 8) & 0xFF) / 255.0, 
		float((objectID >> 16) & 0xFF) / 255.0, 
		float((objectID >> 24) & 0xFF) / 255.0);
#else
	vertex_color_out = vec4(
		float(mod(objectID, 0x100)) / 255.0, 
		float(mod(objectID / 0x100, 0x100)) / 255.0, 
		float(mod(objectID / 0x10000, 0x100)) / 255.0, 
		float(mod(objectID / 0x1000000, 0x100)) / 255.0);		
#endif	
	
	gl_Position = modelview_projection_matrix * vec4(vertex_pos, 1.0);
}
