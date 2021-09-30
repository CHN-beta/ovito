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

// Uniforms:
uniform float color_range_min;
uniform float color_range_max;
uniform sampler1D color_map;

// Inputs:
in vec4 color_fs;

// Outputs:
out vec4 fragColor;

void main()
{
	// Interpolated input color.
	vec4 color = color_fs;

	// If pseudocolor mapping is used, apply tabulated transfer function to pseudocolor value,
	// which is stored in the R component of the input color.
	if(color_range_min != color_range_max) {
		float pseudocolor_value = (color.r - color_range_min) / (color_range_max - color_range_min);
		color.rgb = texture(color_map, pseudocolor_value).rgb;
	}

    // Flat shading:
    fragColor = color;
}
