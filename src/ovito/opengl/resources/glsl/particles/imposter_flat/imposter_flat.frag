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
#include <shading.frag>

// Inputs:
flat in vec4 color_fs;
in vec2 uv_fs;

void main()
{
	// Test if fragment is within the unit circle.
	float rsq = dot(uv_fs, uv_fs);
	if(rsq >= 1.0) discard;

	// Calculate surface normal in view coordinate system.
	vec3 surface_normal = vec3(uv_fs, sqrt(1.0 - rsq));

	// Compute local surface color.
	outputShadedRay(color_fs, surface_normal, vec3(0.0, 0.0, -1.0));
}
