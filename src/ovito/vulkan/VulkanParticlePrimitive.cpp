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
    // Create pipeline for shader "sphere":
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

        std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = { renderer->globalUniformsDescriptorSetLayout() };

        cube.create(*renderer->context(),
            QStringLiteral("particles/cube/cube"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            vertexBindingDesc.size(), // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            vertexAttrDesc.size(), // vertexAttributeDescriptionCount
            vertexAttrDesc.data(), 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // enableAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        cube_picking.create(*renderer->context(),
            QStringLiteral("particles/cube/cube_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            2, // vertexAttributeDescriptionCount
            vertexAttrDesc.data(), 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // enableAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        sphere.create(*renderer->context(),
            QStringLiteral("particles/sphere/sphere"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            vertexBindingDesc.size(), // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            vertexAttrDesc.size(), // vertexAttributeDescriptionCount
            vertexAttrDesc.data(), 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // enableAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        sphere_picking.create(*renderer->context(),
            QStringLiteral("particles/sphere/sphere_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            2, // vertexAttributeDescriptionCount
            vertexAttrDesc.data(), 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // enableAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );
    }    
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanParticlePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	cube.release(*renderer->context());
	cube_picking.release(*renderer->context());
	sphere.release(*renderer->context());
	sphere_picking.release(*renderer->context());
}

/******************************************************************************
* Renders the particles.
******************************************************************************/
void VulkanParticlePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!positions() || positions()->size() == 0)
		return;
	if(indices() && indices()->size() == 0)
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
    uint32_t particleCount = indices() ? indices()->size() : positions()->size();

    // Bind the right Vulkan pipeline.
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    switch(particleShape()) {
        case SquareCubicShape:
            if(!renderer->isPicking()) {
                pipelineLayout = pipelines.cube.layout();
                pipelines.cube.bind(*renderer->context(), renderer->currentCommandBuffer());
            }
            else {
                pipelineLayout = pipelines.cube_picking.layout();
                pipelines.cube_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
            }
            break;
        case SphericalShape:
            if(!renderer->isPicking()) {
                pipelineLayout = pipelines.sphere.layout();
                pipelines.sphere.bind(*renderer->context(), renderer->currentCommandBuffer());
            }
            else {
                pipelineLayout = pipelines.sphere_picking.layout();
                pipelines.sphere_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
            }
            break;
        default:
            return;
    }

    // Set up push constants.
    switch(particleShape()) {
        case SquareCubicShape:

            // Pass model-view-projection matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

            if(!renderer->isPicking()) {
                // Pass normal transformation matrix to vertex shader as a push constant.
                Matrix_3<float> normal_matrix = Matrix_3<float>(renderer->modelViewTM().linear().inverse().transposed());
                normal_matrix.column(0).normalize();
                normal_matrix.column(1).normalize();
                normal_matrix.column(2).normalize();
                // It's almost impossible to pass a mat3 to the shader with the correct memeory layout. 
                // Better use a mat4 to be safe:
                Matrix_4<float> normal_matrix4(normal_matrix);
                renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(normal_matrix4), normal_matrix4.data());
            }
            else {
                // Pass picking base ID to vertex shader as a push constant.
                uint32_t pickingBaseId = renderer->registerSubObjectIDs(positions()->size(), indices());
                renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(pickingBaseId), &pickingBaseId);
            }

            break;
        case SphericalShape:

            // Pass model-view-projection matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

            // Pass model-view transformation matrix to vertex shader as a push constant.
            // In order to match the 16-byte alignment requirements of shader interface blocks, we convert the 3x4 matrix from column-major
            // ordering to row-major ordering, with three rows or 4 floats. The shader uses "layout(row_major) mat4x3" to read the matrix.
            std::array<float, 3*4> transposed_modelview_matrix;
            {
                auto transposed_modelview_matrix_iter = transposed_modelview_matrix.begin();
                for(size_t row = 0; row < 3; row++)
                    for(size_t col = 0; col < 4; col++)
                        *transposed_modelview_matrix_iter++ = static_cast<float>(renderer->modelViewTM()(row,col));
            }
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(transposed_modelview_matrix), transposed_modelview_matrix.data());

            if(renderer->isPicking()) {
                // Pass picking base ID to vertex shader as a push constant.
                uint32_t pickingBaseId = renderer->registerSubObjectIDs(positions()->size(), indices());
                renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>) + sizeof(transposed_modelview_matrix), sizeof(pickingBaseId), &pickingBaseId);
            }

            break;
        default:
            return;
    }

    // Bind the descriptor set to the pipeline.
    VkDescriptorSet globalUniformsSet = renderer->getGlobalUniformsDescriptorSet();
    renderer->deviceFunctions()->vkCmdBindDescriptorSets(renderer->currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalUniformsSet, 0, nullptr);

    // Put positions and radii into one combined Vulkan buffer with 4 floats per particle.
    // Radii are optional and may be substituted with a uniform radius value.
    VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> positionRadiusCacheKey{
        indices(),
        positions(),
        radii(),
        radii() ? FloatType(0) : uniformRadius()
    };

    // Upload vertex buffer with the particle positions and radii.
    VkBuffer positionRadiusBuffer = renderer->context()->createCachedBuffer(positionRadiusCacheKey, particleCount * 4 * sizeof(float), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
        ConstDataBufferAccess<Point3> positionArray(positions());
        ConstDataBufferAccess<FloatType> radiusArray(radii());
        float* dst = reinterpret_cast<float*>(buffer);
        if(!indices()) {
            const FloatType* radius = radiusArray ? radiusArray.cbegin() : nullptr;
            for(const Point3& pos : positionArray) {
                *dst++ = static_cast<float>(pos.x());
                *dst++ = static_cast<float>(pos.y());
                *dst++ = static_cast<float>(pos.z());
                *dst++ = static_cast<float>(radius ? *radius++ : uniformRadius());
            }
        }
        else {
            for(int index : ConstDataBufferAccess<int>(indices())) {
                const Point3& pos = positionArray[index];
                *dst++ = static_cast<float>(pos.x());
                *dst++ = static_cast<float>(pos.y());
                *dst++ = static_cast<float>(pos.z());
                *dst++ = static_cast<float>(radiusArray ? radiusArray[index] : uniformRadius());
            }
        }
    });

    if(!renderer->isPicking()) {

        // Put colors, transparencies and selection state into one combined Vulkan buffer with 4 floats per particle.
        VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, Color> colorSelectionCacheKey{ 
            indices(),
            colors(),
            transparencies(),
            selection(),
            colors() ? Color(0,0,0) : uniformColor()
        };

        // Upload vertex buffer with the particle colors.
        VkBuffer colorSelectionBuffer = renderer->context()->createCachedBuffer(colorSelectionCacheKey, particleCount * 4 * sizeof(float), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
            ConstDataBufferAccess<FloatType> transparencyArray(transparencies());
            ConstDataBufferAccess<int> selectionArray(selection());
            const ColorT<float> uniformColor = (ColorT<float>)this->uniformColor();
            const ColorAT<float> selectionColor = (ColorAT<float>)this->selectionColor();
            if(!indices()) {
                ConstDataBufferAccess<FloatType,true> colorArray(colors());
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
            }
            else {
                ConstDataBufferAccess<Color> colorArray(colors());
                float* dst = reinterpret_cast<float*>(buffer);
                for(int index : ConstDataBufferAccess<int>(indices())) {
                    if(selectionArray && selectionArray[index]) {
                        *dst++ = selectionColor.r();
                        *dst++ = selectionColor.g();
                        *dst++ = selectionColor.b();
                        *dst++ = selectionColor.a();
                    }
                    else {
                        // RGB:
                        if(colorArray) {
                            const Color& color = colorArray[index];
                            *dst++ = static_cast<float>(color.r());
                            *dst++ = static_cast<float>(color.g());
                            *dst++ = static_cast<float>(color.b());
                        }
                        else {
                            *dst++ = uniformColor.r();
                            *dst++ = uniformColor.g();
                            *dst++ = uniformColor.b();
                        }
                        // Alpha:
                        *dst++ = transparencyArray ? qBound(0.0f, 1.0f - static_cast<float>(transparencyArray[index]), 1.0f) : 1.0f;
                    }
                }
            }
        });

        // Bind vertex buffers.
        std::array<VkBuffer, 2> buffers = { positionRadiusBuffer, colorSelectionBuffer };
        std::array<VkDeviceSize, 2> offsets = { 0, 0 };
        renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, buffers.size(), buffers.data(), offsets.data());
    }
    else {
        // Bind vertex buffers. In picking mode, only the positions and radii are needed.
        std::array<VkBuffer, 1> buffers = { positionRadiusBuffer };
        std::array<VkDeviceSize, 1> offsets = { 0 };
        renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, buffers.size(), buffers.data(), offsets.data());
    }

    // Draw triangle strip instances.
    renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), 14, particleCount, 0, 0);
}

/******************************************************************************
* Renders the particles using imposter quads.
******************************************************************************/
void VulkanParticlePrimitive::renderImposterGeometries(VulkanSceneRenderer* renderer, const Pipelines& pipelines, const QMatrix4x4& mvp)
{
}

}	// End of namespace
