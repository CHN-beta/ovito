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

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
VulkanPipeline& VulkanLinePrimitive::Pipelines::create(VulkanSceneRenderer* renderer, VulkanPipeline& pipeline)
{
    if(pipeline.isCreated())
        return pipeline;

    uint32_t extraDynamicStateCount = 0;
    std::array<VkDynamicState, 2> extraDynamicState;

    // Are wide lines supported by the Vulkan device?
    if(renderer->context()->supportsWideLines())
        extraDynamicState[extraDynamicStateCount++] = VK_DYNAMIC_STATE_LINE_WIDTH;

    // Are extended dynamic states supported by the Vulkan device?
    if(renderer->context()->supportsExtendedDynamicState())
        extraDynamicState[extraDynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT;

    // Create pipeline for shader "thin_with_colors":
    if(&pipeline == &thinWithColors)
    {
        VkVertexInputBindingDescription vertexBindingDesc[2];
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

        thinWithColors.create(*renderer->context(),
            QStringLiteral("lines/thin_with_colors"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            2, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            extraDynamicStateCount,
    		extraDynamicState.data()
        );
    }

    // Create pipeline for shader "thin_uniform_color":
    if(&pipeline == &thinUniformColor)
    {
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

        thinUniformColor.create(*renderer->context(),
            QStringLiteral("lines/thin_uniform_color"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>), // vertexPushConstantSize
            sizeof(ColorAT<float>),  // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            1, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data()
        );
    }
    
    // Create pipeline for shader "thin_picking":
    if(&pipeline == &thinPicking)
    {
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

        thinPicking.create(*renderer->context(),
            QStringLiteral("lines/thin_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            1, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data()
        );    
    }

    return pipeline; 
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanLinePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	thinWithColors.release(*renderer->context());
	thinUniformColor.release(*renderer->context());
	thinPicking.release(*renderer->context());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void VulkanLinePrimitive::render(VulkanSceneRenderer* renderer, Pipelines& pipelines)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!positions() || positions()->size() == 0)
		return;

#if 1
    // For now always rely on the line drawing capabilities of the Vulkan implementation,
    // even for lines wides than 1 pixel.
    renderThinLines(renderer, pipelines);
#else
	if(lineWidth() == 1 || (lineWidth() <= 0 && renderer->devicePixelRatio() <= 1))
		renderThinLines(renderer, pipelines);
	else
		renderThickLines(renderer, pipelines);
#endif
}

/******************************************************************************
* Renders the lines exactly one pixel wide.
******************************************************************************/
void VulkanLinePrimitive::renderThinLines(VulkanSceneRenderer* renderer, Pipelines& pipelines)
{
    // Bind the right pipeline.
	if(colors() && !renderer->isPicking()) {
        pipelines.create(renderer, pipelines.thinWithColors).bind(*renderer->context(), renderer->currentCommandBuffer());
    }
    else {
        if(!renderer->isPicking()) {
            pipelines.create(renderer, pipelines.thinUniformColor).bind(*renderer->context(), renderer->currentCommandBuffer());
        }
        else {
            pipelines.create(renderer, pipelines.thinPicking).bind(*renderer->context(), renderer->currentCommandBuffer());
        }
    }

    // Specify line width if the Vulkan implementation supports it. 
    if(renderer->context()->supportsWideLines()) {
        FloatType effectiveLineWidth = (lineWidth() <= 0) ? renderer->devicePixelRatio() : lineWidth();
        renderer->deviceFunctions()->vkCmdSetLineWidth(renderer->currentCommandBuffer(), static_cast<float>(effectiveLineWidth));
    }

    // Specify dynamic depth-test enabled state if Vulkan implementation supports it.
    if(renderer->context()->supportsExtendedDynamicState()) {
        renderer->context()->vkCmdSetDepthTestEnableEXT(renderer->currentCommandBuffer(), renderer->_depthTestEnabled);
    }

    // Compute full view-projection matrix including correction for OpenGL/Vulkan convention difference.
    QMatrix4x4 mvp = renderer->clipCorrection() * renderer->projParams().projectionMatrix * renderer->modelViewTM();

	if(colors() && !renderer->isPicking()) {
		// Pass transformation matrix to vertex shader as a push constant.
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinWithColors.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

		// Bind vertex buffers for vertex positions and colors.
		std::array<VkBuffer, 2> buffers = {
			renderer->context()->uploadDataBuffer(positions(), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), 
			renderer->context()->uploadDataBuffer(colors(), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		};
		std::array<VkDeviceSize, 2> offsets = { 0, 0 };
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, buffers.size(), buffers.data(), offsets.data());
	}
	else {
        if(!renderer->isPicking()) {
            // Pass transformation matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinUniformColor.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());
            // Pass uniform line color to fragment shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinUniformColor.layout(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Matrix_4<float>), sizeof(ColorAT<float>), ColorAT<float>(uniformColor()).data());
        }
        else {
            // Pass transformation matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinPicking.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());
            // Pass picking base ID to vertex shader as a push constant.
    		uint32_t pickingBaseId = renderer->registerSubObjectIDs(positions()->size() / 2);
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinPicking.layout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(pickingBaseId), &pickingBaseId);
        }

		// Bind vertex buffer for vertex positions.
		VkBuffer buffer = renderer->context()->uploadDataBuffer(positions(), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		VkDeviceSize offset = 0;
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 1, &buffer, &offset);
	}

    // Draw lines.
    renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), positions()->size(), 1, 0, 0);
}

/******************************************************************************
* Renders the lines of arbitrary width using polygons.
******************************************************************************/
void VulkanLinePrimitive::renderThickLines(VulkanSceneRenderer* renderer, Pipelines& pipelines)
{
    // Not implemented yet.
}

}	// End of namespace
