////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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

const float ambient = 0.4;
const float diffuse_strength = 1.0 - ambient;
const float shininess = 6.0;
const vec3 specular_lightdir = normalize(vec3(-1.8, 1.5, -0.2));

vec4 shadeSurfaceColor(in vec3 surface_normal, in vec3 ray_dir, in vec3 color, in float alpha)
{
	float diffuse = abs(surface_normal.z) * diffuse_strength;
	float specular = pow(max(0.0, dot(reflect(specular_lightdir, surface_normal), ray_dir)), shininess) * 0.25;
	return vec4(color * (diffuse + ambient) + vec3(specular), alpha);
}

vec4 shadeSurfaceColorQuick(in vec3 surface_normal, in vec3 color, in float alpha)
{
	float reflz = max(0.0, 2.0 * dot(surface_normal, specular_lightdir) * surface_normal.z - specular_lightdir.z);
	
	// Compute pow(reflz, shininess)
	float specular = reflz * reflz; 
	specular = 0.25 * specular * specular * specular;

	float diffuse = abs(surface_normal.z) * diffuse_strength;

	return vec4(color * (diffuse + ambient) + vec3(specular), alpha);
}

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

// Replacement for inverse(mat3) function, which is only available in GLSL version 1.40. 
float determinant_mat2(in mat2 matrix) {
    return matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x;
}
mat3 inverse_mat3(in mat3 matrix) {
    vec3 row0 = matrix[0];
    vec3 row1 = matrix[1];
    vec3 row2 = matrix[2];

    vec3 minors0 = vec3(
        determinant_mat2(mat2(row1.y, row1.z, row2.y, row2.z)),
        determinant_mat2(mat2(row1.z, row1.x, row2.z, row2.x)),
        determinant_mat2(mat2(row1.x, row1.y, row2.x, row2.y))
    );
    vec3 minors1 = vec3(
        determinant_mat2(mat2(row2.y, row2.z, row0.y, row0.z)),
        determinant_mat2(mat2(row2.z, row2.x, row0.z, row0.x)),
        determinant_mat2(mat2(row2.x, row2.y, row0.x, row0.y))
    );
    vec3 minors2 = vec3(
        determinant_mat2(mat2(row0.y, row0.z, row1.y, row1.z)),
        determinant_mat2(mat2(row0.z, row0.x, row1.z, row1.x)),
        determinant_mat2(mat2(row0.x, row0.y, row1.x, row1.y))
    );

    mat3 adj = transpose(mat3(minors0, minors1, minors2));

    return (1.0 / dot(row0, minors0)) * adj;
}
