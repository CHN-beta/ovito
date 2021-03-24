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
#include "VulkanParticlePrimitive.h"
#include "VulkanSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanParticlePrimitive::Pipelines::init(VulkanSceneRenderer* renderer)
{
    // Create pipeline for shader "cube":
    {
        std::array<VkVertexInputBindingDescription, 2> vertexBindingDesc;

        // Position + radius:
        vertexBindingDesc[0].binding = 0;
        vertexBindingDesc[0].stride = sizeof(Vector_4<float>);
        vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        // Color + alpha
        vertexBindingDesc[1].binding = 1;
        vertexBindingDesc[1].stride = sizeof(Vector_4<float>);
        vertexBindingDesc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        std::array<VkVertexInputAttributeDescription, 3> vertexAttrDesc = {
            VkVertexInputAttributeDescription{ // position:
                0, // location
                0, // binding
                VK_FORMAT_R32G32B32_SFLOAT,
                0 // offset
            },
            VkVertexInputAttributeDescription{ // radius:
                1, // location
                0, // binding
                VK_FORMAT_R32_SFLOAT,
                3 * sizeof(float) // offset
            },
            VkVertexInputAttributeDescription{ // color:
                2, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                0 // offset
            }
        };

        cube.create(*renderer->context(),
            QStringLiteral("particles/cube/cube"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_3<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            vertexBindingDesc.size(), // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            vertexAttrDesc.size(), // vertexAttributeDescriptionCount
            vertexAttrDesc.data(), 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP // topology
        );
    }
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanParticlePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	cube.release(*renderer->context());
}

/******************************************************************************
* Renders the particles.
******************************************************************************/
void VulkanParticlePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!positions() || positions()->size() == 0)
		return;

    // Compute full view-projection matrix including correction for OpenGL/Vulkan convention difference.
    QMatrix4x4 mvp = renderer->clipCorrection() * renderer->projParams().projectionMatrix * renderer->modelViewTM();

    renderBoxGeometries(renderer, pipelines, mvp);
}

/******************************************************************************
* Renders the particles using box-shaped geometry.
******************************************************************************/
void VulkanParticlePrimitive::renderBoxGeometries(VulkanSceneRenderer* renderer, const Pipelines& pipelines, const QMatrix4x4& mvp)
{
    if(!renderer->isPicking()) {

		// Bind the pipeline.
        pipelines.cube.bind(*renderer->context(), renderer->currentCommandBuffer());

		// Pass model-view-projection matrix to vertex shader as a push constant.
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.cube.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());
		// Pass normal transformation matrix to vertex shader as a push constant.
        Matrix_3<float> normalTM = Matrix_3<float>::Zero();
	    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelines.cube.layout(), VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(Matrix_3<float>), normalTM.data());

        // Put positions and radii into one combined Vulkan buffer with 4 floats per particle.
        // Radii are optional and may be substituted with a uniform radius value.
        VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> positionRadiusCacheKey{
            positions(),
            radii(),
            radii() ? FloatType(0) : uniformRadius()
        };

		// Upload vertex buffer with the particle positions and radii.
		VkBuffer positionRadiusBuffer = renderer->context()->createCachedBuffer(positionRadiusCacheKey, positions()->size() * 4 * sizeof(float), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
            ConstDataBufferAccess<Point3> positionArray(positions());
            ConstDataBufferAccess<FloatType> radiusArray(radii());
            float* dst = reinterpret_cast<float*>(buffer);
            const FloatType* radius = radiusArray ? radiusArray.cbegin() : nullptr;
            for(const Point3& pos : positionArray) {
                *dst++ = static_cast<float>(pos.x());
                *dst++ = static_cast<float>(pos.y());
                *dst++ = static_cast<float>(pos.z());
                *dst++ = static_cast<float>(radius ? *radius++ : uniformRadius());
            }
        });

        // Put colors, transparencies and selection state into one combined Vulkan buffer with 4 floats per particle.
        VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, Color> colorSelectionCacheKey{ 
            colors(),
            transparencies(),
            selection(),
            colors() ? Color(0,0,0) : uniformColor()
        };

		// Upload vertex buffer with the particle positions and radii.
		VkBuffer colorSelectionBuffer = renderer->context()->createCachedBuffer(colorSelectionCacheKey, positions()->size() * 4 * sizeof(float), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
            ConstDataBufferAccess<FloatType,true> colorArray(colors());
            ConstDataBufferAccess<FloatType> transparencyArray(transparencies());
            ConstDataBufferAccess<int> selectionArray(selection());
            const ColorT<float> uniformColor = (ColorT<float>)this->uniformColor();
            const ColorAT<float> selectionColor = (ColorAT<float>)this->selectionColor();
            const FloatType* color = colorArray ? colorArray.cbegin() : nullptr;
            const FloatType* transparency = transparencyArray ? transparencyArray.cbegin() : nullptr;
            const int* selection = selectionArray ? selectionArray.cbegin() : nullptr;
            for(float* dst = reinterpret_cast<float*>(buffer), *dst_end = dst + positions()->size() * 4; dst != dst_end;) {
                if(selection && *selection++) {
                    *dst++ = selectionColor.r();
                    *dst++ = selectionColor.g();
                    *dst++ = selectionColor.b();
                    *dst++ = selectionColor.a();
                    if(color) color += 3;
                    if(transparency) transparency += 1;
                }
                else {
                    // RGB:
                    if(color) {
                        *dst++ = static_cast<float>(*color++);
                        *dst++ = static_cast<float>(*color++);
                        *dst++ = static_cast<float>(*color++);
                    }
                    else {
                        *dst++ = uniformColor.r();
                        *dst++ = uniformColor.g();
                        *dst++ = uniformColor.b();
                    }
                    // Alpha:
                    *dst++ = transparency ? qBound(0.0f, 1.0f - static_cast<float>(*transparency++), 1.0f) : 1.0f;
                }
            }
        });

		// Bind vertex buffers.
		std::array<VkBuffer, 2> buffers = { positionRadiusBuffer, colorSelectionBuffer };
		std::array<VkDeviceSize, 2> offsets = { 0, 0 };
		renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, buffers.size(), buffers.data(), offsets.data());

        // Draw triangle strip instances.
        renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), 14, positions()->size(), 0, 0);
    }
}

/******************************************************************************
* Renders the particles using imposter quads.
******************************************************************************/
void VulkanParticlePrimitive::renderImposterGeometries(VulkanSceneRenderer* renderer, const Pipelines& pipelines, const QMatrix4x4& mvp)
{
}

}	// End of namespace
