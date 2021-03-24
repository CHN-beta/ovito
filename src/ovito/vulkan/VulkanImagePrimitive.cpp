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

#include <ovito/core/Core.h>
#include "VulkanImagePrimitive.h"
#include "VulkanSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanImagePrimitive::Pipelines::init(VulkanSceneRenderer* renderer)
{
    // Specify the descriptor layout binding for the sampler.
    VkSampler sampler = renderer->context()->samplerNearest();
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = &sampler;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create descriptor set layout.
    VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    VkResult err = renderer->deviceFunctions()->vkCreateDescriptorSetLayout(renderer->logicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
    if(err != VK_SUCCESS)
        renderer->throwException(QStringLiteral("Failed to create Vulkan descriptor set layout (error code %1).").arg(err));

    // Create pipeline.
    imageQuad.create(*renderer->context(),
        QStringLiteral("image/image"),
        renderer->defaultRenderPass(),
        2 * sizeof(Point_2<float>), // vertexPushConstantSize
        0, // fragmentPushConstantSize
        0, // vertexBindingDescriptionCount
        nullptr, 
        0, // vertexAttributeDescriptionCount
        nullptr, 
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
		0, // extraDynamicStateCount
		nullptr, // pExtraDynamicStates
		true, // enableAlphaBlending
        1, // setLayoutCount
		&descriptorSetLayout
    );
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanImagePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	imageQuad.release(*renderer->context());

    if(descriptorSetLayout != VK_NULL_HANDLE) {
        renderer->deviceFunctions()->vkDestroyDescriptorSetLayout(renderer->logicalDevice(), descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void VulkanImagePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
	if(image().isNull() || renderer->isPicking() || windowRect().isEmpty())
		return;

    // Upload the image to the GPU as a texture.
    VkImageView imageView = renderer->context()->uploadImage(image(), renderer->currentResourceFrame());

    // Bind the pipeline.
    pipelines.imageQuad.bind(*renderer->context(), renderer->currentCommandBuffer());

    // Use the QImage cache key to look up descriptor set.
    VulkanResourceKey<VulkanImagePrimitive, qint64> cacheKey{ image().cacheKey() };

    // Create or look up the descriptor set.
    std::pair<VkDescriptorSet, bool> descriptorSet = renderer->context()->createDescriptorSet(pipelines.descriptorSetLayout, cacheKey, renderer->currentResourceFrame());

    // Initialize the descriptor set if it was newly created.
    if(descriptorSet.second) {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = renderer->context()->samplerNearest();
        VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptorWrite.dstSet = descriptorSet.first;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        renderer->deviceFunctions()->vkUpdateDescriptorSets(renderer->logicalDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    // Bind the descriptor set to the pipeline.
    renderer->deviceFunctions()->vkCmdBindDescriptorSets(renderer->currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.imageQuad.layout(), 0, 1, &descriptorSet.first, 0, nullptr);

    // Pass quad rectangle to vertex shader as a push constant.
    Point_2<float> quad[2];
    quad[0].x() = (float)(windowRect().minc.x() / (FloatType)renderer->frameBufferSize().width() * 2.0 - 1.0);
    quad[0].y() = (float)(windowRect().minc.y() / (FloatType)renderer->frameBufferSize().height() * 2.0 - 1.0);
    quad[1].x() = (float)(windowRect().maxc.x() / (FloatType)renderer->frameBufferSize().width() * 2.0 - 1.0);
    quad[1].y() = (float)(windowRect().maxc.y() / (FloatType)renderer->frameBufferSize().height() * 2.0 - 1.0);
    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.imageQuad.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(quad), quad);

    // Draw quad.
    renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), 4, 1, 0, 0);
}

}	// End of namespace
