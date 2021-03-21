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
#include "VulkanPipeline.h"
#include "VulkanSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipeline.
******************************************************************************/
void VulkanPipeline::create(VulkanSceneRenderer* renderer, 
    const QString& shaderName, 
    uint32_t vertexPushConstantSize,
    uint32_t fragmentPushConstantSize,
    uint32_t vertexBindingDescriptionCount,
    const VkVertexInputBindingDescription* pVertexBindingDescriptions,
    uint32_t vertexAttributeDescriptionCount,
    const VkVertexInputAttributeDescription* pVertexAttributeDescriptions,
    VkPrimitiveTopology topology,
    uint32_t extraDynamicStateCount,
    const VkDynamicState* pExtraDynamicStates,
    bool enableAlphaBlending,
    uint32_t setLayoutCount,
    const VkDescriptorSetLayout* pSetLayouts)
{
    OVITO_ASSERT(_layout == VK_NULL_HANDLE);
    OVITO_ASSERT(_pipeline == VK_NULL_HANDLE);

    // Set up push constants used by the shader.
    VkPushConstantRange pushConstantRanges[2];
	pushConstantRanges[0].offset = 0;
	pushConstantRanges[0].size = vertexPushConstantSize;
	pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRanges[1].offset = vertexPushConstantSize;
	pushConstantRanges[1].size = fragmentPushConstantSize;
	pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.pushConstantRangeCount = (vertexPushConstantSize != 0 ? 1 : 0) + (fragmentPushConstantSize != 0 ? 1 : 0);
    pipelineLayoutInfo.pPushConstantRanges = (vertexPushConstantSize != 0) ? &pushConstantRanges[0] : &pushConstantRanges[1];
    pipelineLayoutInfo.setLayoutCount = setLayoutCount;
    pipelineLayoutInfo.pSetLayouts = pSetLayouts;
    VkResult err = renderer->deviceFunctions()->vkCreatePipelineLayout(renderer->logicalDevice(), &pipelineLayoutInfo, nullptr, &_layout);
    if(err != VK_SUCCESS)
        renderer->throwException(VulkanSceneRenderer::tr("Failed to create Vulkan pipeline layout (error code %1) for shader '%2'.").arg(err).arg(shaderName));

    // Shaders
    VkShaderModule vertShaderModule = renderer->device()->createShader(QStringLiteral(":/vulkanrenderer/%1.vert.spv").arg(shaderName));
    VkShaderModule fragShaderModule = renderer->device()->createShader(QStringLiteral(":/vulkanrenderer/%1.frag.spv").arg(shaderName));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderModule,
            "main",
            nullptr
        }
    };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
    vertexInputInfo.pVertexBindingDescriptions = pVertexBindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
    vertexInputInfo.pVertexAttributeDescriptions = pVertexAttributeDescriptions;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = topology;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    // Enable multisampling if requested.
    ms.rasterizationSamples = renderer->sampleCount();
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    if(!enableAlphaBlending) {
        // No blend, write out all of RGBA.
        VkPipelineColorBlendAttachmentState att;
        memset(&att, 0, sizeof(att));
        att.colorWriteMask = 0xF;
        cb.attachmentCount = 1;
        cb.pAttachments = &att;
    }
    else {
        // Enable standard alpha blending.
        VkPipelineColorBlendAttachmentState att = {
            VK_TRUE,                              // VkBool32                 blendEnable
            VK_BLEND_FACTOR_SRC_ALPHA,            // VkBlendFactor            srcColorBlendFactor
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,  // VkBlendFactor            dstColorBlendFactor
            VK_BLEND_OP_ADD,                      // VkBlendOp                colorBlendOp
            VK_BLEND_FACTOR_SRC_ALPHA,            // VkBlendFactor            srcAlphaBlendFactor
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,  // VkBlendFactor            dstAlphaBlendFactor
            VK_BLEND_OP_ADD,                      // VkBlendOp                alphaBlendOp
            VK_COLOR_COMPONENT_R_BIT |            // VkColorComponentFlags    colorWriteMask
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
        };
        cb.attachmentCount = 1;
        cb.pAttachments = &att;
    }
    pipelineInfo.pColorBlendState = &cb;

    VkPipelineDynamicStateCreateInfo dyn = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    std::vector<VkDynamicState> dynEnableVector;
    if(extraDynamicStateCount == 0) {
        VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        dyn.dynamicStateCount = 2;
        dyn.pDynamicStates = dynEnable;
    }
    else {
        dynEnableVector.resize(2 + extraDynamicStateCount);
        dynEnableVector[0] = VK_DYNAMIC_STATE_VIEWPORT;
        dynEnableVector[1] = VK_DYNAMIC_STATE_SCISSOR;
        std::copy(pExtraDynamicStates, pExtraDynamicStates + extraDynamicStateCount, dynEnableVector.begin() + 2);
        dyn.dynamicStateCount = (uint32_t)dynEnableVector.size();
        dyn.pDynamicStates = dynEnableVector.data();
    }
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = _layout;
    pipelineInfo.renderPass = renderer->defaultRenderPass();

    err = renderer->deviceFunctions()->vkCreateGraphicsPipelines(renderer->logicalDevice(), renderer->device()->pipelineCache(), 1, &pipelineInfo, nullptr, &_pipeline);
    if(err != VK_SUCCESS)
        renderer->throwException(VulkanSceneRenderer::tr("Failed to create Vulkan graphics pipeline (error code %1) for shader '%2'").arg(err).arg(shaderName));

    if(vertShaderModule)
        renderer->deviceFunctions()->vkDestroyShaderModule(renderer->logicalDevice(), vertShaderModule, nullptr);
    if(fragShaderModule)
        renderer->deviceFunctions()->vkDestroyShaderModule(renderer->logicalDevice(), fragShaderModule, nullptr);
}

/******************************************************************************
* Destroys the Vulkan pipeline.
******************************************************************************/
void VulkanPipeline::release(VulkanSceneRenderer* renderer)
{
	renderer->deviceFunctions()->vkDestroyPipeline(renderer->logicalDevice(), _pipeline, nullptr);
	renderer->deviceFunctions()->vkDestroyPipelineLayout(renderer->logicalDevice(), _layout, nullptr);
}

/******************************************************************************
* Binds the pipeline.
******************************************************************************/
void VulkanPipeline::bind(VulkanSceneRenderer* renderer) const
{
    renderer->deviceFunctions()->vkCmdBindPipeline(renderer->currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
}

}	// End of namespace
