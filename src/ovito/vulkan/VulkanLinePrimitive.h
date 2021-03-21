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
#include <ovito/core/rendering/LinePrimitive.h>
#include "VulkanDevice.h"
#include "VulkanPipeline.h"

namespace Ovito {

class VulkanSceneRenderer;

/**
 * \brief This class is responsible for rendering line primitives using Vulkan.
 */
class VulkanLinePrimitive : public LinePrimitive
{
public:

	struct Pipelines {
		/// Creates the Vulkan pipelines for this rendering primitive.
		void init(VulkanSceneRenderer* renderer);
		/// Destroys the Vulkan pipelines for this rendering primitive.
		void release(VulkanSceneRenderer* renderer);

		VulkanPipeline thinWithColors;
		VulkanPipeline thinUniformColor;
		VulkanPipeline thinPicking;
	};

	/// Constructor.
	explicit VulkanLinePrimitive(VulkanSceneRenderer* renderer);

	/// Renders the geometry.
	void render(VulkanSceneRenderer* renderer, const Pipelines& pipelines);

protected:

	/// Renders the lines exactly one pixel wide.
	void renderThinLines(VulkanSceneRenderer* renderer, const Pipelines& pipelines);

	/// Renders the lines of arbitrary width using polygons.
	void renderThickLines(VulkanSceneRenderer* renderer, const Pipelines& pipelines);
};

}	// End of namespace
