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
#include "VulkanLinePrimitive.h"
#include "VulkanSceneRenderer.h"

#include <QVulkanDeviceFunctions>

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanLinePrimitive::VulkanLinePrimitive(VulkanSceneRenderer* renderer)
{
}

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanLinePrimitive::Pipelines::init(VulkanSceneRenderer* renderer)
{
    {
    // Set up push constants used by the shader.
    VkPushConstantRange pushConstantRange;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(Matrix_4<float>);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Create the pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    VkResult err = renderer->deviceFunctions()->vkCreatePipelineLayout(renderer->logicalDevice(), &pipelineLayoutInfo, nullptr, &thinWithColorsLayout);
    if(err != VK_SUCCESS)
        renderer->throwException(VulkanSceneRenderer::tr("Failed to create Vulkan pipeline layout (error code %1).").arg(err));

    // Shaders
    VkShaderModule vertShaderModule = renderer->device()->createShader(QStringLiteral(":/vulkanrenderer/lines/lines_thin_with_colors.vert.spv"));
    VkShaderModule fragShaderModule = renderer->device()->createShader(QStringLiteral(":/vulkanrenderer/lines/lines_thin_with_colors.frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

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

    VkVertexInputBindingDescription vertexBindingDesc[2];
    memset(vertexBindingDesc, 0, sizeof(vertexBindingDesc));
    vertexBindingDesc[0].binding = 0;
    vertexBindingDesc[0].stride = sizeof(Point_3<float>);
    vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexBindingDesc[1].binding = 1;
    vertexBindingDesc[1].stride = sizeof(ColorAT<float>);
    vertexBindingDesc[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position:
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0 // offset
        },
        { // color:
            1, // location
            1, // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,
            0 // offset
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling if requested.
    ms.rasterizationSamples = renderer->sampleCount();
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // No blend, write out all of RGBA.
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = thinWithColorsLayout;
    pipelineInfo.renderPass = renderer->defaultRenderPass();

    err = renderer->deviceFunctions()->vkCreateGraphicsPipelines(renderer->logicalDevice(), renderer->device()->pipelineCache(), 1, &pipelineInfo, nullptr, &thinWithColors);
    if(err != VK_SUCCESS)
        renderer->throwException(VulkanSceneRenderer::tr("Failed to create Vulkan graphics pipeline (error code %1).").arg(err));

    if(vertShaderModule)
        renderer->deviceFunctions()->vkDestroyShaderModule(renderer->logicalDevice(), vertShaderModule, nullptr);
    if(fragShaderModule)
        renderer->deviceFunctions()->vkDestroyShaderModule(renderer->logicalDevice(), fragShaderModule, nullptr);
    }

    {
    // Set up push constants used by the shader.
    VkPushConstantRange pushConstantRanges[2];
	pushConstantRanges[0].offset = 0;
	pushConstantRanges[0].size = sizeof(Matrix_4<float>);
	pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRanges[1].offset = sizeof(Matrix_4<float>);
	pushConstantRanges[1].size = sizeof(ColorAT<float>);
	pushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Create the pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 2;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges;
    VkResult err = renderer->deviceFunctions()->vkCreatePipelineLayout(renderer->logicalDevice(), &pipelineLayoutInfo, nullptr, &thinUniformColorLayout);
    if(err != VK_SUCCESS)
        renderer->throwException(VulkanSceneRenderer::tr("Failed to create Vulkan pipeline layout (error code %1).").arg(err));

    // Shaders
    VkShaderModule vertShaderModule = renderer->device()->createShader(QStringLiteral(":/vulkanrenderer/lines/lines_thin_uniform_color.vert.spv"));
    VkShaderModule fragShaderModule = renderer->device()->createShader(QStringLiteral(":/vulkanrenderer/lines/lines_thin_uniform_color.frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

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

    VkVertexInputBindingDescription vertexBindingDesc[1];
    memset(vertexBindingDesc, 0, sizeof(vertexBindingDesc));
    vertexBindingDesc[0].binding = 0;
    vertexBindingDesc[0].stride = sizeof(Point_3<float>);
    vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position:
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0 // offset
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling if requested.
    ms.rasterizationSamples = renderer->sampleCount();
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // No blend, write out all of RGBA.
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = thinUniformColorLayout;
    pipelineInfo.renderPass = renderer->defaultRenderPass();

    err = renderer->deviceFunctions()->vkCreateGraphicsPipelines(renderer->logicalDevice(), renderer->device()->pipelineCache(), 1, &pipelineInfo, nullptr, &thinUniformColor);
    if(err != VK_SUCCESS)
        renderer->throwException(VulkanSceneRenderer::tr("Failed to create Vulkan graphics pipeline (error code %1).").arg(err));

    if(vertShaderModule)
        renderer->deviceFunctions()->vkDestroyShaderModule(renderer->logicalDevice(), vertShaderModule, nullptr);
    if(fragShaderModule)
        renderer->deviceFunctions()->vkDestroyShaderModule(renderer->logicalDevice(), fragShaderModule, nullptr);
    }    
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanLinePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	renderer->deviceFunctions()->vkDestroyPipeline(renderer->logicalDevice(), thinWithColors, nullptr);
	renderer->deviceFunctions()->vkDestroyPipelineLayout(renderer->logicalDevice(), thinWithColorsLayout, nullptr);
	renderer->deviceFunctions()->vkDestroyPipeline(renderer->logicalDevice(), thinUniformColor, nullptr);
	renderer->deviceFunctions()->vkDestroyPipelineLayout(renderer->logicalDevice(), thinUniformColorLayout, nullptr);
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void VulkanLinePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!positions() || positions()->size() == 0)
		return;

	if(lineWidth() == 1 || (lineWidth() <= 0 && renderer->devicePixelRatio() <= 1))
		renderThinLines(renderer, pipelines);
	else
		renderThickLines(renderer, pipelines);
}

/******************************************************************************
* Renders the lines exactly one pixel wide.
******************************************************************************/
void VulkanLinePrimitive::renderThinLines(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Compute full view-projection matrix including correction for OpenGL/Vulkan convention difference.
    QMatrix4x4 mvp = renderer->clipCorrection() * renderer->projParams().projectionMatrix * renderer->modelViewTM();

	if(colors()) {
		// Bind the pipeline.
		renderer->deviceFunctions()->vkCmdBindPipeline(renderer->currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.thinWithColors);

		// Pass transformation matrix to vertex shader as a push constant.
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinWithColorsLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

		// Bind vertex buffers for vertex positions and colors.
		VkBuffer buffers[2] = {
			renderer->device()->uploadDataBuffer(positions(), renderer->currentResourceFrame()), 
			renderer->device()->uploadDataBuffer(colors(), renderer->currentResourceFrame())
		};
		VkDeviceSize vbOffsets[2] = { 0, 0 };
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 2, buffers, vbOffsets);
	}
	else {
		// Bind the pipeline.
	    renderer->deviceFunctions()->vkCmdBindPipeline(renderer->currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.thinUniformColor);	

		// Pass transformation matrix to vertex shader as a push constant.
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinUniformColorLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());
		// Pass uniform line color to fragment shader as a push constant.
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinUniformColorLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Matrix_4<float>), sizeof(ColorAT<float>), ColorAT<float>(uniformColor()).data());

		// Bind vertex buffer for vertex positions.
		VkBuffer buffers[1] = { renderer->device()->uploadDataBuffer(positions(), renderer->currentResourceFrame()) };
		VkDeviceSize vbOffsets[1] = { 0 };
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 1, buffers, vbOffsets);
	}

    // Draw lines.
    renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), positions()->size(), 1, 0, 0);
}

/******************************************************************************
* Renders the lines using polygons.
******************************************************************************/
void VulkanLinePrimitive::renderThickLines(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
	// Effective line width.
	FloatType effectiveLineWidth = (lineWidth() <= 0) ? renderer->devicePixelRatio() : lineWidth();
}

}	// End of namespace
