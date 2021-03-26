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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include "VulkanContext.h"
#include "VulkanPipeline.h"

namespace Ovito {

class VulkanSceneRenderer;

/**
 * \brief This class is responsible for rendering particles using Vulkan.
 */
class VulkanParticlePrimitive : public ParticlePrimitive
{
public:

	struct Pipelines {
		/// Creates the Vulkan pipelines for this rendering primitive.
		void init(VulkanSceneRenderer* renderer);
		/// Destroys the Vulkan pipelines for this rendering primitive.
		void release(VulkanSceneRenderer* renderer);

		VulkanPipeline cube;
		VulkanPipeline cube_picking;
		VulkanPipeline sphere;
		VulkanPipeline sphere_picking;
		VulkanPipeline square;
		VulkanPipeline square_picking;
		VulkanPipeline box;
		VulkanPipeline box_picking;
		VulkanPipeline ellipsoid;
		VulkanPipeline ellipsoid_picking;
		VulkanPipeline superquadric;
		VulkanPipeline superquadric_picking;
	};

	/// Inherit constructor from base class.
	using ParticlePrimitive::ParticlePrimitive;

	/// Renders the particles.
	void render(VulkanSceneRenderer* renderer, const Pipelines& pipelines);
};

}	// End of namespace
