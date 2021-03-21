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
#include "VulkanDevice.h"

namespace Ovito {

class VulkanSceneRenderer;

/**
 * \brief Thin wrapper for Vulkan pipeline objects.
 */
class VulkanPipeline
{
public:

	/// Creates the Vulkan pipeline.
	void create(VulkanSceneRenderer* renderer,
	    const QString& shaderName, 
		uint32_t vertexPushConstantSize,
		uint32_t fragmentPushConstantSize,
		uint32_t vertexBindingDescriptionCount,
		const VkVertexInputBindingDescription* pVertexBindingDescriptions,
		uint32_t vertexAttributeDescriptionCount,
		const VkVertexInputAttributeDescription* pVertexAttributeDescriptions,
		VkPrimitiveTopology topology,
		uint32_t extraDynamicStateCount = 0,
		const VkDynamicState* pExtraDynamicStates = nullptr,
		bool enableAlphaBlending = false,
		uint32_t setLayoutCount = 0,
		const VkDescriptorSetLayout* pSetLayouts = nullptr);

	/// Destroys the Vulkan pipeline.
	void release(VulkanSceneRenderer* renderer);

	/// Binds the pipeline.
	void bind(VulkanSceneRenderer* renderer) const;

	/// Returns the pipeline's layout.
	VkPipelineLayout layout() const { return _layout; }

private:

	VkPipelineLayout _layout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;
};

}	// End of namespace
