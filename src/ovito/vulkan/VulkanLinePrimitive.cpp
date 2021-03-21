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
    // Are wide lines supported by the Vulkan device?
    uint32_t extraDynamicStateCount = renderer->device()->features().wideLines ? 1 : 0;
    VkDynamicState extraDynamicState = VK_DYNAMIC_STATE_LINE_WIDTH;

    // Create pipeline for shader "lines_thin_with_colors":
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

        thinWithColors.create(renderer,
            QStringLiteral("lines/lines_thin_with_colors"),
            sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            2, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            extraDynamicStateCount,
    		&extraDynamicState
        );
    }

    // Create pipeline for shader "lines_thin_uniform_color":
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

        thinUniformColor.create(renderer,
            QStringLiteral("lines/lines_thin_uniform_color"), 
            sizeof(Matrix_4<float>), // vertexPushConstantSize
            sizeof(ColorAT<float>),  // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            1, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            extraDynamicStateCount,
            &extraDynamicState
        );
    }
    
    // Create pipeline for shader "lines_thin_picking":
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

        thinPicking.create(renderer,
            QStringLiteral("lines/lines_thin_picking"), 
            sizeof(Matrix_4<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            1, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            extraDynamicStateCount,
            &extraDynamicState
        );    
    }    
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanLinePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	thinWithColors.release(renderer);
	thinUniformColor.release(renderer);
	thinPicking.release(renderer);
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void VulkanLinePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
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
void VulkanLinePrimitive::renderThinLines(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Compute full view-projection matrix including correction for OpenGL/Vulkan convention difference.
    QMatrix4x4 mvp = renderer->clipCorrection() * renderer->projParams().projectionMatrix * renderer->modelViewTM();

	if(colors() && !renderer->isPicking()) {
		// Bind the pipeline.
        pipelines.thinWithColors.bind(renderer);

        // Specify line width if the Vulkan implementation supports it. 
        if(renderer->device()->features().wideLines) {
        	FloatType effectiveLineWidth = (lineWidth() <= 0) ? renderer->devicePixelRatio() : lineWidth();
            renderer->deviceFunctions()->vkCmdSetLineWidth(renderer->currentCommandBuffer(), static_cast<float>(effectiveLineWidth));
        }

		// Pass transformation matrix to vertex shader as a push constant.
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinWithColors.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

		// Bind vertex buffers for vertex positions and colors.
		VkBuffer buffers[2] = {
			renderer->device()->uploadDataBuffer(positions(), renderer->currentResourceFrame()), 
			renderer->device()->uploadDataBuffer(colors(), renderer->currentResourceFrame())
		};
		VkDeviceSize vbOffsets[2] = { 0, 0 };
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 2, buffers, vbOffsets);
	}
	else {
        if(!renderer->isPicking()) {
            // Bind the pipeline.
            pipelines.thinUniformColor.bind(renderer);
            // Pass transformation matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinUniformColor.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());
            // Pass uniform line color to fragment shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinUniformColor.layout(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Matrix_4<float>), sizeof(ColorAT<float>), ColorAT<float>(uniformColor()).data());
        }
        else {
            // Bind the pipeline.
            pipelines.thinPicking.bind(renderer);
            // Pass transformation matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinPicking.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());
            // Pass picking base ID to vertex shader as a push constant.
    		uint32_t pickingBaseId = renderer->registerSubObjectIDs(positions()->size() / 2);
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.thinPicking.layout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(pickingBaseId), &pickingBaseId);
        }

        // Specify line width if the Vulkan implementation supports it. 
        if(renderer->device()->features().wideLines) {
        	FloatType effectiveLineWidth = (lineWidth() <= 0) ? renderer->devicePixelRatio() : lineWidth();
            renderer->deviceFunctions()->vkCmdSetLineWidth(renderer->currentCommandBuffer(), static_cast<float>(effectiveLineWidth));
        }

		// Bind vertex buffer for vertex positions.
		VkBuffer buffers[1] = { renderer->device()->uploadDataBuffer(positions(), renderer->currentResourceFrame()) };
		VkDeviceSize vbOffsets[1] = { 0 };
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 1, buffers, vbOffsets);
	}

    // Draw lines.
    renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), positions()->size(), 1, 0, 0);
}

/******************************************************************************
* Renders the lines of arbitrary width using polygons.
******************************************************************************/
void VulkanLinePrimitive::renderThickLines(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Not implemented yet.
}

}	// End of namespace
