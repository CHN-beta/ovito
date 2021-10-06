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

#include "../global_uniforms.glsl"
#include "../shading.glsl"

// Inputs:
flat in vec4 color_fs;
flat in vec3 cylinder_view_base;		// Transformed cylinder position in view coordinates
flat in vec3 cylinder_view_axis;		// Transformed cylinder axis in view coordinates
flat in float cylinder_radius_sq_fs;	// The squared radius of the cylinder
flat in float cylinder_length;			// The length of the cylinder
noperspective in vec3 ray_origin;
noperspective in vec3 ray_dir;

// Outputs:
out vec4 fragColor;

void main()
{
	vec3 ray_dir_norm = normalize(ray_dir);

	// Perform ray-cylinder intersection test.
	vec3 n = cross(ray_dir_norm, cylinder_view_axis);
	float ln = length(n);
	vec3 RC = ray_origin - cylinder_view_base;

	vec3 view_intersection_pnt = ray_origin;
	vec3 surface_normal;

	bool skip = false;

	if(ln < 1e-7 * cylinder_length) {
		// Handle case where view ray is parallel to cylinder axis:

		float t = dot(RC, ray_dir_norm);
		float v = dot(RC, RC);
		if(v-t*t > cylinder_radius_sq_fs) {
			discard;
		}
		else {
			view_intersection_pnt -= t * ray_dir_norm;
			surface_normal = -cylinder_view_axis;
			float tfar = dot(cylinder_view_axis, ray_dir_norm);
			if(tfar < 0.0) {
				view_intersection_pnt += tfar * ray_dir_norm;
				surface_normal = cylinder_view_axis;
			}
		}
	}
	else {

		n /= ln;
		float d = dot(RC,n);
		d *= d;

		// Test if ray missed the cylinder.
		if(d > cylinder_radius_sq_fs) {
			discard;
		}
		else {

			// Calculate closest intersection position.
			float t = dot(cross(cylinder_view_axis, RC), n) / ln;
			float s = abs(sqrt(cylinder_radius_sq_fs - d) / dot(cross(n, cylinder_view_axis), ray_dir_norm) * cylinder_length);
			float tnear = t - s;

			// Calculate intersection point in view coordinate system.
			view_intersection_pnt += tnear * ray_dir_norm;

			// Find intersection position along cylinder axis.
			float anear = dot(view_intersection_pnt - cylinder_view_base, cylinder_view_axis) / (cylinder_length*cylinder_length);
			if(anear >= 0.0 && anear <= 1.0) {

				// Calculate surface normal in view coordinate system.
				surface_normal = (view_intersection_pnt - (cylinder_view_base + anear * cylinder_view_axis));
			}
			else {
				// Calculate second intersection point.
				float tfar = t + s;
				vec3 far_view_intersection_pnt = ray_origin + tfar * ray_dir_norm;
				float afar = dot(far_view_intersection_pnt - cylinder_view_base, cylinder_view_axis) / (cylinder_length*cylinder_length);

				// Compute intersection with cylinder caps.
				if(anear < 0.0 && afar > 0.0) {
					view_intersection_pnt += (anear / (anear - afar) * 2.0 * s + 1e-6 * ln) * ray_dir_norm;
					surface_normal = -cylinder_view_axis;
				}
				else if(anear > 1.0 && afar < 1.0) {
					view_intersection_pnt += ((anear - 1.0) / (anear - afar) * 2.0 * s + 1e-6 * ln) * ray_dir_norm;
					surface_normal = cylinder_view_axis;
				}
				else {
					discard;
				}
			}
		}
	}

	// Output the ray-cylinder intersection point as the fragment depth
	// rather than the depth of the bounding box polygons.
	// The eye coordinate Z value must be transformed to normalized device
	// coordinates before being assigned as the final fragment depth.
	vec4 projected_intersection = projection_matrix * vec4(view_intersection_pnt, 1.0);
	gl_FragDepth = (projected_intersection.z / projected_intersection.w + 1.0) * 0.5;

    fragColor = shadeSurfaceColorDir(normalize(surface_normal), color_fs, ray_dir_norm);
}