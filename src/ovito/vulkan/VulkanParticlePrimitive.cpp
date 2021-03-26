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

#include <boost/range/irange.hpp>

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanParticlePrimitive::Pipelines::init(VulkanSceneRenderer* renderer)
{
    {
        std::array<VkVertexInputBindingDescription, 4> vertexBindingDesc;

        // Position + radius:
        vertexBindingDesc[0].binding = 0;
        vertexBindingDesc[0].stride = sizeof(Vector_4<float>);
        vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        // Color + alpha
        vertexBindingDesc[1].binding = 1;
        vertexBindingDesc[1].stride = sizeof(Vector_4<float>);
        vertexBindingDesc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        // Shape + orientation
        vertexBindingDesc[2].binding = 2;
        vertexBindingDesc[2].stride = sizeof(Matrix_4<float>);
        vertexBindingDesc[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        // Roundness
        vertexBindingDesc[3].binding = 3;
        vertexBindingDesc[3].stride = sizeof(Vector_2<float>);
        vertexBindingDesc[3].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        VkVertexInputAttributeDescription vertexAttrDesc[] = {
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
            },
            VkVertexInputAttributeDescription{ // shape_orientation matrix (column 1):
                3, // location
                2, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                0 * sizeof(Matrix_4<float>::column_type) // offset
            },
            VkVertexInputAttributeDescription{ // shape_orientation matrix (column 2):
                4, // location
                2, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                1 * sizeof(Matrix_4<float>::column_type) // offset
            },
            VkVertexInputAttributeDescription{ // shape_orientation matrix (column 3):
                5, // location
                2, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                2 * sizeof(Matrix_4<float>::column_type) // offset
            },
            VkVertexInputAttributeDescription{ // shape_orientation matrix (column 4):
                6, // location
                2, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                3 * sizeof(Matrix_4<float>::column_type) // offset
            },
            VkVertexInputAttributeDescription{ // roundness:
                7, // location
                3, // binding
                VK_FORMAT_R32G32_SFLOAT,
                0 // offset
            },
        };

        std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = { renderer->globalUniformsDescriptorSetLayout() };

        cube.create(*renderer->context(),
            QStringLiteral("particles/cube/cube"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            3, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
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
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        sphere.create(*renderer->context(),
            QStringLiteral("particles/sphere/sphere"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            3, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
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
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        square.create(*renderer->context(),
            QStringLiteral("particles/square/square"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            3, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        square_picking.create(*renderer->context(),
            QStringLiteral("particles/square/square_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            2, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        circle.create(*renderer->context(),
            QStringLiteral("particles/circle/circle"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            3, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        circle_picking.create(*renderer->context(),
            QStringLiteral("particles/circle/circle_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            2, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );        

        box.create(*renderer->context(),
            QStringLiteral("particles/box/box"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            3, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            7, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        VkVertexInputBindingDescription vertexBindingDescBoxPicking[3] {
            vertexBindingDesc[0], vertexBindingDesc[2]
        };
        vertexBindingDescBoxPicking[1].binding = 1;
        VkVertexInputAttributeDescription vertexAttrDescBoxPicking[7] = {
            vertexAttrDesc[0], vertexAttrDesc[1],
            vertexAttrDesc[3], vertexAttrDesc[4],
            vertexAttrDesc[5], vertexAttrDesc[6]
        };
        vertexAttrDescBoxPicking[2].binding = vertexAttrDescBoxPicking[3].binding = vertexAttrDescBoxPicking[4].binding = vertexAttrDescBoxPicking[5].binding = 1;
        box_picking.create(*renderer->context(),
            QStringLiteral("particles/box/box_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDescBoxPicking, 
            6, // vertexAttributeDescriptionCount
            vertexAttrDescBoxPicking, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        ellipsoid.create(*renderer->context(),
            QStringLiteral("particles/ellipsoid/ellipsoid"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            3, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            7, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        ellipsoid_picking.create(*renderer->context(),
            QStringLiteral("particles/ellipsoid/ellipsoid_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDescBoxPicking, 
            6, // vertexAttributeDescriptionCount
            vertexAttrDescBoxPicking, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        superquadric.create(*renderer->context(),
            QStringLiteral("particles/superquadric/superquadric"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            4, // vertexBindingDescriptionCount
            vertexBindingDesc.data(), 
            8, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data()
        );

        // Roundness
        vertexBindingDescBoxPicking[2].binding = 2;
        vertexBindingDescBoxPicking[2].stride = sizeof(Vector_2<float>);
        vertexBindingDescBoxPicking[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        vertexAttrDescBoxPicking[6] =
            VkVertexInputAttributeDescription{ // roundness:
                7, // location
                2, // binding
                VK_FORMAT_R32G32_SFLOAT,
                0 // offset
            };
        superquadric_picking.create(*renderer->context(),
            QStringLiteral("particles/superquadric/superquadric_picking"), 
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(AffineTransformationT<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            3, // vertexBindingDescriptionCount
            vertexBindingDescBoxPicking, 
            7, // vertexAttributeDescriptionCount
            vertexAttrDescBoxPicking, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            false, // supportAlphaBlending
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
	square.release(*renderer->context());
	square_picking.release(*renderer->context());
	circle.release(*renderer->context());
	circle_picking.release(*renderer->context());
	box.release(*renderer->context());
	box_picking.release(*renderer->context());
	ellipsoid.release(*renderer->context());
	ellipsoid_picking.release(*renderer->context());
	superquadric.release(*renderer->context());
	superquadric_picking.release(*renderer->context());
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

    // The effective number of particles being rendered:
    uint32_t particleCount = indices() ? indices()->size() : positions()->size();
    uint32_t verticesPerParticle = 0;

    // Are we rendering semi-transparent particles?
    bool useBlending = !renderer->isPicking() && (transparencies() != nullptr);

    // Bind the right Vulkan pipeline.
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    switch(particleShape()) {
        case SquareCubicShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->isPicking()) {
                    pipelineLayout = pipelines.cube.layout();
                    pipelines.cube.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                }
                else {
                    pipelineLayout = pipelines.cube_picking.layout();
                    pipelines.cube_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
                }
                verticesPerParticle = 14; // Cube rendered as triangle strip.
            }
            else {
                if(!renderer->isPicking()) {
                    pipelineLayout = pipelines.square.layout();
                    pipelines.square.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                }
                else {
                    pipelineLayout = pipelines.square_picking.layout();
                    pipelines.square_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
                }
                verticesPerParticle = 4; // Square rendered as triangle strip.
            }
            break;
        case BoxShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->isPicking()) {
                    pipelineLayout = pipelines.box.layout();
                    pipelines.box.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                }
                else {
                    pipelineLayout = pipelines.box_picking.layout();
                    pipelines.box_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
                }
                verticesPerParticle = 14; // Box rendered as triangle strip.
            }
            else return;
            break;
        case SphericalShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->isPicking()) {
                    pipelineLayout = pipelines.sphere.layout();
                    pipelines.sphere.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                }
                else {
                    pipelineLayout = pipelines.sphere_picking.layout();
                    pipelines.sphere_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
                }
                verticesPerParticle = 14; // Cube rendered as triangle strip.
            }
            else {
                if(!renderer->isPicking()) {
                    pipelineLayout = pipelines.circle.layout();
                    pipelines.circle.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                }
                else {
                    pipelineLayout = pipelines.circle_picking.layout();
                    pipelines.circle_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
                }
                verticesPerParticle = 4; // Square rendered as triangle strip.
            }
            break;
        case EllipsoidShape:
            if(!renderer->isPicking()) {
                pipelineLayout = pipelines.ellipsoid.layout();
                pipelines.ellipsoid.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
            }
            else {
                pipelineLayout = pipelines.ellipsoid_picking.layout();
                pipelines.ellipsoid_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
            }
            verticesPerParticle = 14; // Box rendered as triangle strip.
            break;
        case SuperquadricShape:
            if(!renderer->isPicking()) {
                pipelineLayout = pipelines.superquadric.layout();
                pipelines.superquadric.bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
            }
            else {
                pipelineLayout = pipelines.superquadric_picking.layout();
                pipelines.superquadric_picking.bind(*renderer->context(), renderer->currentCommandBuffer());
            }
            verticesPerParticle = 14; // Box rendered as triangle strip.
            break;
        default:
            return;
    }

    // Set up push constants.
    switch(particleShape()) {
        case SquareCubicShape:

            if(shadingMode() == NormalShading) {
                // Pass model-view-projection matrix to vertex shader as a push constant.
                renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

                if(!renderer->isPicking()) {
                    // Pass normal transformation matrix to vertex shader as a push constant.
                    Matrix_3<float> normal_matrix = Matrix_3<float>(renderer->modelViewTM().linear().inverse().transposed());
                    normal_matrix.column(0).normalize();
                    normal_matrix.column(1).normalize();
                    normal_matrix.column(2).normalize();
                    // It's almost impossible to pass a mat3 to the shader with the correct memory layout. 
                    // Better use a mat4 to be safe:
                    Matrix_4<float> normal_matrix4(normal_matrix);
                    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(normal_matrix4), normal_matrix4.data());
                }
                else {
                    // Pass picking base ID to vertex shader as a push constant.
                    uint32_t pickingBaseId = renderer->registerSubObjectIDs(positions()->size(), indices());
                    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(pickingBaseId), &pickingBaseId);
                }
            }
            else {
                // Pass projection matrix to vertex shader as a push constant.
                renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), Matrix_4<float>(renderer->clipCorrection() * renderer->projParams().projectionMatrix).data());

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
            }

            break;

        case BoxShape:

            // Pass model-view-projection matrix to vertex shader as a push constant.
            renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

            if(!renderer->isPicking()) {
                // Pass normal transformation matrix to vertex shader as a push constant.
                Matrix_3<float> normal_matrix = Matrix_3<float>(renderer->modelViewTM().linear().inverse().transposed());
                normal_matrix.column(0).normalize();
                normal_matrix.column(1).normalize();
                normal_matrix.column(2).normalize();
                // It's almost impossible to pass a mat3 to the shader with the correct memory layout. 
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
        case EllipsoidShape:
        case SuperquadricShape:

            if(shadingMode() == NormalShading) {
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
            }
            else {
                // Pass projection matrix to vertex shader as a push constant.
                renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), Matrix_4<float>(renderer->clipCorrection() * renderer->projParams().projectionMatrix).data());

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
        OVITO_ASSERT(!radii() || radii()->size() == positions()->size());
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

    // The list of buffers that will be bound to vertex attributes.
    // We will bind the particle positions and radii for sure. More buffers may be added to the list below.
    std::array<VkBuffer, 4> buffers = { positionRadiusBuffer };
    std::array<VkDeviceSize, 4> offsets = { 0, 0, 0, 0 };
    uint32_t buffersCount = 1;

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
            OVITO_ASSERT(!transparencies() || transparencies()->size() == positions()->size());
            OVITO_ASSERT(!selection() || selection()->size() == positions()->size());
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

        // Bind color vertex buffer.
        buffers[buffersCount++] = colorSelectionBuffer;
    }

    // For box-shaped and ellipsoid particles, we need the shape/orientation vertex attribute.
    if(particleShape() == BoxShape || particleShape() == EllipsoidShape || particleShape() == SuperquadricShape) {

        // Combine aspherical shape property and orientation property into one combined Vulkan buffer containing a 4x4 transformation matrix per particle.
        VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> shapeOrientationCacheKey{ 
            indices(),
            asphericalShapes(),
            orientations(),
            radii(),
            radii() ? FloatType(0) : uniformRadius()
        };

        // Upload vertex buffer with the particle transformation matrices.
        VkBuffer shapeOrientationBuffer = renderer->context()->createCachedBuffer(shapeOrientationCacheKey, particleCount * sizeof(Matrix_4<float>), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
            ConstDataBufferAccess<Vector3> asphericalShapeArray(asphericalShapes());
            ConstDataBufferAccess<Quaternion> orientationArray(orientations());
            ConstDataBufferAccess<FloatType> radiusArray(radii());
            OVITO_ASSERT(!asphericalShapes() || asphericalShapes()->size() == positions()->size());
            OVITO_ASSERT(!orientations() || orientations()->size() == positions()->size());
            if(!indices()) {
                const Vector3* shape = asphericalShapeArray ? asphericalShapeArray.cbegin() : nullptr;
                const Quaternion* orientation = orientationArray ? orientationArray.cbegin() : nullptr;
                const FloatType* radius = radiusArray ? radiusArray.cbegin() : nullptr;
                for(Matrix_4<float>* dst = reinterpret_cast<Matrix_4<float>*>(buffer), *dst_end = dst + positions()->size(); dst != dst_end; ++dst) {
                    Vector_3<float> axes;
                    if(shape) {
                        if(*shape != Vector3::Zero()) {
                            axes = Vector_3<float>(*shape);
                        }
                        else {
                            axes = Vector_3<float>(static_cast<float>(radius ? (*radius) : uniformRadius()));
                        }
                        ++shape;
                    }
                    else {
                        axes = Vector_3<float>(static_cast<float>(radius ? (*radius) : uniformRadius()));
                    }
                    if(radius)
                        ++radius;

                    if(orientation) {
                        QuaternionT<float> quat = QuaternionT<float>(*orientation++);
                        float c = sqrt(quat.dot(quat));
                        if(c <= (float)FLOATTYPE_EPSILON)
                            quat.setIdentity();
                        else
                            quat /= c;
                        *dst = Matrix_4<float>(
                                quat * Vector_3<float>(axes.x(), 0.0f, 0.0f),
                                quat * Vector_3<float>(0.0f, axes.y(), 0.0f),
                                quat * Vector_3<float>(0.0f, 0.0f, axes.z()),
                                Vector_3<float>::Zero());
                    }
                    else {
                        *dst = Matrix_4<float>(
                                axes.x(), 0.0f, 0.0f, 0.0f,
                                0.0f, axes.y(), 0.0f, 0.0f,
                                0.0f, 0.0f, axes.z(), 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);
                    }
                }
            }
            else {
                Matrix_4<float>* dst = reinterpret_cast<Matrix_4<float>*>(buffer);
                for(int index : ConstDataBufferAccess<int>(indices())) {
                    Vector_3<float> axes;
                    if(asphericalShapeArray && asphericalShapeArray[index] != Vector3::Zero()) {
                        axes = Vector_3<float>(asphericalShapeArray[index]);
                    }
                    else {
                        axes = Vector_3<float>(static_cast<float>(radiusArray ? radiusArray[index] : uniformRadius()));
                    }

                    if(orientationArray) {
                        QuaternionT<float> quat = QuaternionT<float>(orientationArray[index]);
                        float c = sqrt(quat.dot(quat));
                        if(c <= (float)FLOATTYPE_EPSILON)
                            quat.setIdentity();
                        else
                            quat /= c;
                        *dst = Matrix_4<float>(
                                quat * Vector_3<float>(axes.x(), 0.0f, 0.0f),
                                quat * Vector_3<float>(0.0f, axes.y(), 0.0f),
                                quat * Vector_3<float>(0.0f, 0.0f, axes.z()),
                                Vector_3<float>::Zero());
                    }
                    else {
                        *dst = Matrix_4<float>(
                                axes.x(), 0.0f, 0.0f, 0.0f,
                                0.0f, axes.y(), 0.0f, 0.0f,
                                0.0f, 0.0f, axes.z(), 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);
                    }
                    ++dst;
                }
            }
        });

        // Bind shape/orientation vertex buffer.
        buffers[buffersCount++] = shapeOrientationBuffer;
    }

    // For superquadric particles, we need to prepare the roundness vertex attribute.
    if(particleShape() == SuperquadricShape) {

        VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr> roundnessCacheKey{ 
            indices(),
            roundness()
        };

        // Upload vertex buffer with the roundness values.
        VkBuffer roundnessBuffer = renderer->context()->createCachedBuffer(roundnessCacheKey, particleCount * sizeof(Vector_2<float>), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
            Vector_2<float>* dst = reinterpret_cast<Vector_2<float>*>(buffer);
            if(roundness()) {
                OVITO_ASSERT(roundness()->size() == positions()->size());
                if(!indices()) {
                    for(const Vector2& r : ConstDataBufferAccess<Vector2>(roundness())) {
                        *dst++ = Vector_2<float>(r);
                    }
                }
                else {
                    ConstDataBufferAccess<Vector2> roundnessArray(roundness());
                    for(int index : ConstDataBufferAccess<int>(indices())) {
                        *dst++ = Vector_2<float>(roundnessArray[index]);
                    }
                }
            }
            else {
                std::fill(dst, dst + particleCount, Vector_2<float>(1,1));
            }
        });

        // Bind vertex buffer.
        buffers[buffersCount++] = roundnessBuffer;
    }    

    // Bind vertex buffers.
    renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, buffersCount, buffers.data(), offsets.data());

    // Check indirect drawing capabilities of Vulkan device, which are needed for depth-sorted rendering.
    bool indirectDrawingSupported = renderer->context()->supportsMultiDrawIndirect()
        && renderer->context()->supportsDrawIndirectFirstInstance()
        && particleCount <= renderer->context()->physicalDeviceProperties()->limits.maxDrawIndirectCount;

    if(!useBlending || !indirectDrawingSupported) {
        // Draw triangle strip instances in regular storage order (not sorted).
        renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), verticesPerParticle, particleCount, 0, 0);
    }
    else {
        // Create a buffer for an indirect drawing command to render the particles in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = renderer->modelViewTM().inverse().column(2);

        // The caching key for the indirect drawing command buffer.
        VulkanResourceKey<VulkanParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, Vector3, int> indirectBufferCacheKey{
            indices(),
            positions(),
            direction,
            verticesPerParticle
        };

        // Create indirect drawing buffer.
        VkBuffer indirectBuffer = renderer->context()->createCachedBuffer(indirectBufferCacheKey, particleCount * sizeof(VkDrawIndirectCommand), renderer->currentResourceFrame(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, [&](void* buffer) {

            // First, compute distance of each particle from the camera along the viewing direction (=camera z-axis).
            std::vector<FloatType> distances(particleCount);
            if(!indices()) {
                boost::transform(boost::irange<size_t>(0, particleCount), distances.begin(), [direction, positionsArray = ConstDataBufferAccess<Vector3>(positions())](size_t i) {
                    return direction.dot(positionsArray[i]);
                });
            }
            else {
                boost::transform(ConstDataBufferAccess<int>(indices()), distances.begin(), [direction, positionsArray = ConstDataBufferAccess<Vector3>(positions())](size_t i) {
                    return direction.dot(positionsArray[i]);
                });
            }

            // Create index array with all particle indices.
            std::vector<uint32_t> sortedIndices(particleCount);
            std::iota(sortedIndices.begin(), sortedIndices.end(), (uint32_t)0);

            // Sort particle indices with respect to distance (back-to-front order).
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
                return distances[a] < distances[b];
            });

            // Fill the buffer with VkDrawIndirectCommand records.
            VkDrawIndirectCommand* dst = reinterpret_cast<VkDrawIndirectCommand*>(buffer);
            for(int index : sortedIndices) {
                dst->vertexCount = verticesPerParticle;
                dst->instanceCount = 1;
                dst->firstVertex = 0;
                dst->firstInstance = index;
                ++dst;
            }
        });

        // Draw triangle strip instances in sorted order.
        renderer->deviceFunctions()->vkCmdDrawIndirect(renderer->currentCommandBuffer(), indirectBuffer, 0, particleCount, sizeof(VkDrawIndirectCommand));
    }
}

}	// End of namespace
