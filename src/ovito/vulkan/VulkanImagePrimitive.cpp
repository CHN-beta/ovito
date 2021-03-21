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
* Constructor.
******************************************************************************/
VulkanImagePrimitive::VulkanImagePrimitive(VulkanSceneRenderer* renderer)
{
}

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanImagePrimitive::Pipelines::init(VulkanSceneRenderer* renderer)
{
    imageQuad.create(renderer,
        QStringLiteral("image/image"),
        2 * sizeof(Point_2<float>), // vertexPushConstantSize
        0, // fragmentPushConstantSize
        0, // vertexBindingDescriptionCount
        nullptr, 
        0, // vertexAttributeDescriptionCount
        nullptr, 
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
		0, // extraDynamicStateCount
		nullptr, // pExtraDynamicStates
		true // enableAlphaBlending
    );
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanImagePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	imageQuad.release(renderer);
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void VulkanImagePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
	if(image().isNull() || renderer->isPicking() || windowRect().isEmpty())
		return;

    // Bind the pipeline.
    pipelines.imageQuad.bind(renderer);

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
