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
#include <ovito/core/dataset/DataSet.h>
#include "VulkanMeshPrimitive.h"
#include "VulkanSceneRenderer.h"

#include <boost/range/irange.hpp>

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
VulkanPipeline& VulkanMeshPrimitive::Pipelines::create(VulkanSceneRenderer* renderer, VulkanPipeline& pipeline)
{
    if(pipeline.isCreated())
        return pipeline;

    // Are extended dynamic states supported by the Vulkan device?
    // If yes, we use the feature to dynamically turn back-face culling on and off.
    uint32_t extraDynamicStateCount = 0;
    std::array<VkDynamicState, 2> extraDynamicState;
    extraDynamicState[extraDynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    if(renderer->context()->supportsExtendedDynamicState()) {
        extraDynamicState[extraDynamicStateCount++] = VK_DYNAMIC_STATE_CULL_MODE_EXT;
    }

    std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = { renderer->globalUniformsDescriptorSetLayout() };

    VkVertexInputBindingDescription vertexBindingDesc[3];
    vertexBindingDesc[0].binding = 0;
    vertexBindingDesc[0].stride = sizeof(ColoredVertexWithNormal);
    vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexBindingDesc[1].binding = 1;
    vertexBindingDesc[1].stride = 3 * sizeof(Vector_4<float>);
    vertexBindingDesc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    vertexBindingDesc[2].binding = 2;
    vertexBindingDesc[2].stride = sizeof(ColorAT<float>);
    vertexBindingDesc[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position:
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(ColoredVertexWithNormal, position) // offset
        },
        { // normal:
            1, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(ColoredVertexWithNormal, normal) // offset
        },
        { // color:
            2, // location
            0, // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(ColoredVertexWithNormal, color) // offset
        },
        { // instance transformation (row 1):
            3, // location
            1, // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,
            0 * sizeof(Vector_4<float>) // offset
        },
        { // instance transformation (row 2):
            4, // location
            1, // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,
            1 * sizeof(Vector_4<float>) // offset
        },
        { // instance transformation (row 3):
            5, // location
            1, // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,
            2 * sizeof(Vector_4<float>) // offset
        },
        { // instance color:
            6, // location
            2, // binding
            VK_FORMAT_R32G32B32A32_SFLOAT,
            0 // offset
        },
    };

    if(&pipeline == &mesh)
        mesh.create(*renderer->context(),
            QStringLiteral("mesh/mesh"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            3, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data(),
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data(), // pSetLayouts
            true // enableDepthOffset
        );

    if(&pipeline == &mesh_picking)
        mesh_picking.create(*renderer->context(),
            QStringLiteral("mesh/mesh_picking"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            1, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data(),
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data(), // pSetLayouts
            true // enableDepthOffset
        );

    if(&pipeline == &mesh_instanced)
        mesh_instanced.create(*renderer->context(),
            QStringLiteral("mesh/mesh_instanced"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            6, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data(),
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data(), // pSetLayouts
            true // enableDepthOffset
        );

    if(&pipeline == &mesh_instanced_picking) {
        VkVertexInputAttributeDescription vertexAttrDesc[] = {
            { // position:
                0, // location
                0, // binding
                VK_FORMAT_R32G32B32_SFLOAT,
                offsetof(ColoredVertexWithNormal, position) // offset
            },
            { // instance transformation (row 1):
                1, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                0 * sizeof(Vector_4<float>) // offset
            },
            { // instance transformation (row 2):
                2, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                1 * sizeof(Vector_4<float>) // offset
            },
            { // instance transformation (row 3):
                3, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                2 * sizeof(Vector_4<float>) // offset
            }
        };
        mesh_instanced_picking.create(*renderer->context(),
            QStringLiteral("mesh/mesh_instanced_picking"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(uint32_t), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            4, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data(),
            false, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data(), // pSetLayouts
            true // enableDepthOffset
        );
    }

    if(&pipeline == &mesh_instanced_with_colors)
        mesh_instanced_with_colors.create(*renderer->context(),
            QStringLiteral("mesh/mesh_instanced_with_colors"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>) + sizeof(Matrix_4<float>), // vertexPushConstantSize
            0, // fragmentPushConstantSize
            3, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            7, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
            extraDynamicStateCount,
            extraDynamicState.data(),
            true, // supportAlphaBlending
            descriptorSetLayouts.size(), // setLayoutCount
            descriptorSetLayouts.data(), // pSetLayouts
            true // enableDepthOffset
        );

    if(&pipeline == &mesh_wireframe) {
        VkVertexInputBindingDescription vertexBindingDesc[1];
        vertexBindingDesc[0].binding = 0;
        vertexBindingDesc[0].stride = sizeof(Point_3<float>);
        vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        VkVertexInputAttributeDescription vertexAttrDesc = { // position:
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0 // offset
        };
        mesh_wireframe.create(*renderer->context(),
            QStringLiteral("mesh/mesh_wireframe"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>), // vertexPushConstantSize
            sizeof(ColorAT<float>),  // fragmentPushConstantSize
            1, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            1, // vertexAttributeDescriptionCount
            &vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true // supportAlphaBlending
        );
    }

    if(&pipeline == &mesh_wireframe_instanced) {
        VkVertexInputBindingDescription vertexBindingDesc[2];
        vertexBindingDesc[0].binding = 0;
        vertexBindingDesc[0].stride = sizeof(Point_3<float>);
        vertexBindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertexBindingDesc[1].binding = 1;
        vertexBindingDesc[1].stride = 3 * sizeof(Vector_4<float>);
        vertexBindingDesc[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        VkVertexInputAttributeDescription vertexAttrDesc[] = {
            { // position:
                0, // location
                0, // binding
                VK_FORMAT_R32G32B32_SFLOAT,
                offsetof(ColoredVertexWithNormal, position) // offset
            },
            { // instance transformation (row 1):
                1, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                0 * sizeof(Vector_4<float>) // offset
            },
            { // instance transformation (row 2):
                2, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                1 * sizeof(Vector_4<float>) // offset
            },
            { // instance transformation (row 3):
                3, // location
                1, // binding
                VK_FORMAT_R32G32B32A32_SFLOAT,
                2 * sizeof(Vector_4<float>) // offset
            }
        };
        mesh_wireframe_instanced.create(*renderer->context(),
            QStringLiteral("mesh/mesh_wireframe_instanced"),
            renderer->defaultRenderPass(),
            sizeof(Matrix_4<float>), // vertexPushConstantSize
            sizeof(ColorAT<float>),  // fragmentPushConstantSize
            2, // vertexBindingDescriptionCount
            vertexBindingDesc, 
            4, // vertexAttributeDescriptionCount
            vertexAttrDesc, 
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST, // topology
            0, // extraDynamicStateCount
            nullptr, // pExtraDynamicStates
            true // supportAlphaBlending
        );
    }

    return pipeline; 
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanMeshPrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
	mesh.release(*renderer->context());
	mesh_picking.release(*renderer->context());
	mesh_wireframe.release(*renderer->context());
	mesh_wireframe_instanced.release(*renderer->context());
	mesh_instanced.release(*renderer->context());
	mesh_instanced_picking.release(*renderer->context());
	mesh_instanced_with_colors.release(*renderer->context());
}

/******************************************************************************
* Renders the mesh geometry.
******************************************************************************/
void VulkanMeshPrimitive::render(VulkanSceneRenderer* renderer, Pipelines& pipelines)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(faceCount() == 0)
		return;
    if(useInstancedRendering() && perInstanceTMs()->size() == 0)
        return;

    // Compute full view-projection matrix including correction for OpenGL/Vulkan convention difference.
    QMatrix4x4 mvp = renderer->clipCorrection() * renderer->projParams().projectionMatrix * renderer->modelViewTM();

    // Render wireframe lines.
    if(emphasizeEdges() && !renderer->isPicking())
        renderWireframe(renderer, pipelines, mvp);

    // Apply optional positive depth-offset to mesh faces to make the wireframe lines fully visible.
    renderer->deviceFunctions()->vkCmdSetDepthBias(renderer->currentCommandBuffer(), emphasizeEdges() ? 1.0f : 0.0f, 0.0f, emphasizeEdges() ? 1.0f : 0.0f);

    // Are we rendering a semi-transparent mesh?
    bool useBlending = !renderer->isPicking() && !isFullyOpaque();

    // Bind the right pipeline.
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    if(!useInstancedRendering()) {
        if(!renderer->isPicking()) {
            pipelines.create(renderer, pipelines.mesh).bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
            pipelineLayout = pipelines.mesh.layout();
        }
        else {
            pipelines.create(renderer, pipelines.mesh_picking).bind(*renderer->context(), renderer->currentCommandBuffer());
            pipelineLayout = pipelines.mesh_picking.layout();
        }
    }
    else {
        if(!renderer->isPicking()) {
            if(!perInstanceColors()) {
                pipelines.create(renderer, pipelines.mesh_instanced).bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                pipelineLayout = pipelines.mesh_instanced.layout();
            }
            else {
                pipelines.create(renderer, pipelines.mesh_instanced_with_colors).bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
                pipelineLayout = pipelines.mesh_instanced_with_colors.layout();
            }
        }
        else {
            pipelines.create(renderer, pipelines.mesh_instanced_picking).bind(*renderer->context(), renderer->currentCommandBuffer());
            pipelineLayout = pipelines.mesh_instanced_picking.layout();
        }
    }

    // Turn back-face culling on/off if Vulkan implementation supports it.
    if(renderer->context()->supportsExtendedDynamicState()) {
        renderer->context()->vkCmdSetCullModeEXT(renderer->currentCommandBuffer(), cullFaces() ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE);
    }

    // Pass model-view-projection matrix to vertex shader as a push constant.
    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

    if(!renderer->isPicking()) {
        // Pass normal transformation matrix to vertex shader as a push constant.
        Matrix3 normal_matrix;
        if(renderer->modelViewTM().linear().inverse(normal_matrix)) {
            normal_matrix.column(0).normalize();
            normal_matrix.column(1).normalize();
            normal_matrix.column(2).normalize();
        }
        else normal_matrix.setIdentity();
        // It's almost impossible to pass a mat3 to the shader with the correct memory layout. 
        // Better use a mat4 to be safe:
        Matrix_4<float> normal_matrix4(normal_matrix.toDataType<float>().transposed());
        renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(normal_matrix4), normal_matrix4.data());
    }
    else {
        // Pass picking base ID to vertex shader as a push constant.
        uint32_t pickingBaseId = renderer->registerSubObjectIDs(useInstancedRendering() ? perInstanceTMs()->size() : faceCount());
        renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Matrix_4<float>), sizeof(pickingBaseId), &pickingBaseId);
    }

    // Bind the descriptor set to the pipeline.
    VkDescriptorSet globalUniformsSet = renderer->getGlobalUniformsDescriptorSet();
    renderer->deviceFunctions()->vkCmdBindDescriptorSets(renderer->currentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalUniformsSet, 0, nullptr);

    // The look-up key for the Vulkan buffer cache.
    RendererResourceKey<VulkanMeshPrimitive, std::shared_ptr<VulkanMeshPrimitive>, int, std::vector<ColorA>, ColorA> meshCacheKey{
        shared_from_this(),
        faceCount(),
        materialColors(),
        uniformColor()
    };

    // Upload vertex buffer to GPU memory.
    VkBuffer meshBuffer = renderer->context()->createCachedBuffer(meshCacheKey, faceCount() * 3 * sizeof(ColoredVertexWithNormal), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
        ColoredVertexWithNormal* renderVertices = reinterpret_cast<ColoredVertexWithNormal*>(buffer);
        if(!mesh().hasNormals()) {
            quint32 allMask = 0;

            // Compute face normals.
            std::vector<Vector_3<float>> faceNormals(mesh().faceCount());
            auto faceNormal = faceNormals.begin();
            for(auto face = mesh().faces().constBegin(); face != mesh().faces().constEnd(); ++face, ++faceNormal) {
                const Point3& p0 = mesh().vertex(face->vertex(0));
                Vector3 d1 = mesh().vertex(face->vertex(1)) - p0;
                Vector3 d2 = mesh().vertex(face->vertex(2)) - p0;
                *faceNormal = d1.cross(d2).toDataType<float>();
                if(*faceNormal != Vector_3<float>::Zero()) {
                    allMask |= face->smoothingGroups();
                }
            }

            // Initialize render vertices.
            ColoredVertexWithNormal* rv = renderVertices;
            faceNormal = faceNormals.begin();
            ColorAT<float> defaultVertexColor = static_cast<ColorAT<float>>(uniformColor());
            for(auto face = mesh().faces().constBegin(); face != mesh().faces().constEnd(); ++face, ++faceNormal) {

                // Initialize render vertices for this face.
                for(size_t v = 0; v < 3; v++, rv++) {
                    if(face->smoothingGroups())
                        rv->normal = Vector_3<float>::Zero();
                    else
                        rv->normal = *faceNormal;
                    rv->position = mesh().vertex(face->vertex(v)).toDataType<float>();
                    if(mesh().hasVertexColors()) {
                        rv->color = static_cast<ColorAT<float>>(mesh().vertexColor(face->vertex(v)));
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(mesh().hasFaceColors()) {
                        rv->color = static_cast<ColorAT<float>>(mesh().faceColor(face - mesh().faces().constBegin()));
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(face->materialIndex() < materialColors().size() && face->materialIndex() >= 0) {
                        rv->color = static_cast<ColorAT<float>>(materialColors()[face->materialIndex()]);
                    }
                    else {
                        rv->color = defaultVertexColor;
                    }
                }
            }

            if(allMask) {
                std::vector<Vector_3<float>> groupVertexNormals(mesh().vertexCount());
                for(int group = 0; group < OVITO_MAX_NUM_SMOOTHING_GROUPS; group++) {
                    quint32 groupMask = quint32(1) << group;
                    if((allMask & groupMask) == 0)
                        continue;	// Group is not used.

                    // Reset work arrays.
                    std::fill(groupVertexNormals.begin(), groupVertexNormals.end(), Vector_3<float>::Zero());

                    // Compute vertex normals at original vertices for current smoothing group.
                    faceNormal = faceNormals.begin();
                    for(auto face = mesh().faces().constBegin(); face != mesh().faces().constEnd(); ++face, ++faceNormal) {
                        // Skip faces that do not belong to the current smoothing group.
                        if((face->smoothingGroups() & groupMask) == 0) continue;

                        // Add face's normal to vertex normals.
                        for(size_t fv = 0; fv < 3; fv++)
                            groupVertexNormals[face->vertex(fv)] += *faceNormal;
                    }

                    // Transfer vertex normals from original vertices to render vertices.
                    rv = renderVertices;
                    for(const auto& face : mesh().faces()) {
                        if(face.smoothingGroups() & groupMask) {
                            for(size_t fv = 0; fv < 3; fv++, ++rv)
                                rv->normal += groupVertexNormals[face.vertex(fv)];
                        }
                        else rv += 3;
                    }
                }
            }
        }
        else {
            // Use normals stored in the mesh.
            ColoredVertexWithNormal* rv = renderVertices;
            const Vector3* faceNormal = mesh().normals().begin();
            ColorAT<float> defaultVertexColor = static_cast<ColorAT<float>>(uniformColor());
            for(auto face = mesh().faces().constBegin(); face != mesh().faces().constEnd(); ++face) {
                // Initialize render vertices for this face.
                for(size_t v = 0; v < 3; v++, rv++) {
                    rv->normal = (*faceNormal++).toDataType<float>();
                    rv->position = mesh().vertex(face->vertex(v)).toDataType<float>();
                    if(mesh().hasVertexColors()) {
                        rv->color = static_cast<ColorAT<float>>(mesh().vertexColor(face->vertex(v)));
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(mesh().hasFaceColors()) {
                        rv->color = static_cast<ColorAT<float>>(mesh().faceColor(face - mesh().faces().constBegin()));
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(face->materialIndex() >= 0 && face->materialIndex() < materialColors().size()) {
                        rv->color = static_cast<ColorAT<float>>(materialColors()[face->materialIndex()]);
                    }
                    else {
                        rv->color = defaultVertexColor;
                    }
                }
            }
        }
    });

    // Bind vertex buffer.
    VkDeviceSize offset = 0;
    renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 1, &meshBuffer, &offset);

    // The number of instances the Vulkan draw command should draw.
    uint32_t renderInstanceCount = 1;

    if(useInstancedRendering()) {
        renderInstanceCount = perInstanceTMs()->size();

        // Upload the per-instance TMs to GPU memory.
        VkBuffer instanceTMBuffer = getInstanceTMBuffer(renderer);

        // Bind buffer with the instance matrices to the second binding of the shader.
        VkDeviceSize offset = 0;
        renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 1, 1, &instanceTMBuffer, &offset);

        if(perInstanceColors() && !renderer->isPicking()) {
            // Upload the per-instance colors to GPU memory.
            VkBuffer instanceColorBuffer = renderer->context()->uploadDataBuffer(perInstanceColors(), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

            // Bind buffer with the instance colors to the third binding of the shader.
            VkDeviceSize offset = 0;
            renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 2, 1, &instanceColorBuffer, &offset);
        }
    }

    if(renderer->isPicking() || isFullyOpaque()) {
        // Draw triangles in regular storage order (not sorted).
        renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), faceCount() * 3, renderInstanceCount, 0, 0);
    }
    else if(_depthSortingMode == ConvexShapeMode) {
        // Assuming that the input mesh is convex, render semi-transparent triangles in two passes: 
        // First, render triangles facing away from the viewer, then render triangles facing toward the viewer.
        // Each time we pass the entire triangle list to Vulkan and use Vulkan's backface/frontfrace culling
        // option to render the right subset of triangles.
        if(!cullFaces() && renderer->context()->supportsExtendedDynamicState()) {
            // First pass is only needed if backface culling is not active.
            renderer->context()->vkCmdSetCullModeEXT(renderer->currentCommandBuffer(), VK_CULL_MODE_FRONT_BIT);
            renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), faceCount() * 3, renderInstanceCount, 0, 0);
        }
        // Now render front-facing triangles only.
        if(renderer->context()->supportsExtendedDynamicState())
            renderer->context()->vkCmdSetCullModeEXT(renderer->currentCommandBuffer(), VK_CULL_MODE_BACK_BIT);
        renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), faceCount() * 3, renderInstanceCount, 0, 0);
    }
    else if(!useInstancedRendering()) {
        // Create a buffer for an indexed drawing command to render the triangles in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = renderer->modelViewTM().inverse().column(2);

        // The caching key for the index buffer.
        RendererResourceKey<VulkanMeshPrimitive, VkBuffer, Vector3> indexBufferCacheKey{
            meshBuffer,
            direction
        };

        // Create index buffer with three entries per triangle face.
        VkBuffer indexBuffer = renderer->context()->createCachedBuffer(indexBufferCacheKey, faceCount() * 3 * sizeof(uint32_t), renderer->currentResourceFrame(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, [&](void* buffer) {

            // Compute each face's center point.
            std::vector<Vector_3<float>> faceCenters(faceCount());
            auto tc = faceCenters.begin();
            for(auto face = mesh().faces().cbegin(); face != mesh().faces().cend(); ++face, ++tc) {
                // Compute centroid of triangle.
                const auto& v1 = mesh().vertex(face->vertex(0));
                const auto& v2 = mesh().vertex(face->vertex(1));
                const auto& v3 = mesh().vertex(face->vertex(2));
                tc->x() = (float)(v1.x() + v2.x() + v3.x()) / 3.0f;
                tc->y() = (float)(v1.y() + v2.y() + v3.y()) / 3.0f;
                tc->z() = (float)(v1.z() + v2.z() + v3.z()) / 3.0f;
            }

            // Next, compute distance of each face from the camera along the viewing direction (=camera z-axis).
            std::vector<FloatType> distances(faceCount());
            boost::transform(faceCenters, distances.begin(), [direction = direction.toDataType<float>()](const Vector_3<float>& v) {
                return direction.dot(v);
            });

            // Create index array with all face indices.
            std::vector<uint32_t> sortedIndices(faceCount());
            std::iota(sortedIndices.begin(), sortedIndices.end(), (uint32_t)0);

            // Sort face indices with respect to distance (back-to-front order).
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
                return distances[a] < distances[b];
            });

            // Fill the index buffer with vertex indices to render.
            uint32_t* dst = reinterpret_cast<uint32_t*>(buffer);
            for(uint32_t index : sortedIndices) {
                *dst++ = index * 3;
                *dst++ = index * 3 + 1;
                *dst++ = index * 3 + 2;
            }
        });

        // Bind index buffer.
        renderer->deviceFunctions()->vkCmdBindIndexBuffer(renderer->currentCommandBuffer(), indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw triangles in sorted order.
        renderer->deviceFunctions()->vkCmdDrawIndexed(renderer->currentCommandBuffer(), faceCount() * 3, renderInstanceCount, 0, 0, 0);
    }
    else {
        // Create a buffer for an indirect drawing command to render the particles in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = renderer->modelViewTM().inverse().column(2);

        // The caching key for the indirect drawing command buffer.
        RendererResourceKey<VulkanMeshPrimitive, ConstDataBufferPtr, Vector3> indirectBufferCacheKey{
            perInstanceTMs(),
            direction
        };

        // Create indirect drawing buffer.
        VkBuffer indirectBuffer = renderer->context()->createCachedBuffer(indirectBufferCacheKey, renderInstanceCount * sizeof(VkDrawIndirectCommand), renderer->currentResourceFrame(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, [&](void* buffer) {

            // First, compute distance of each instance from the camera along the viewing direction (=camera z-axis).
            std::vector<FloatType> distances(renderInstanceCount);
			boost::transform(boost::irange<size_t>(0, renderInstanceCount), distances.begin(), [direction, tmArray = ConstDataBufferAccess<AffineTransformation>(perInstanceTMs())](size_t i) {
				return direction.dot(tmArray[i].translation());
			});

            // Create index array with all indices.
            std::vector<uint32_t> sortedIndices(renderInstanceCount);
            std::iota(sortedIndices.begin(), sortedIndices.end(), (uint32_t)0);

            // Sort indices with respect to distance (back-to-front order).
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
                return distances[a] < distances[b];
            });

            // Fill the buffer with VkDrawIndirectCommand records.
            VkDrawIndirectCommand* dst = reinterpret_cast<VkDrawIndirectCommand*>(buffer);
            for(uint32_t index : sortedIndices) {
                dst->vertexCount = faceCount() * 3;
                dst->instanceCount = 1;
                dst->firstVertex = 0;
                dst->firstInstance = index;
                ++dst;
            }
        });

        // Draw instances in sorted order.
        renderer->deviceFunctions()->vkCmdDrawIndirect(renderer->currentCommandBuffer(), indirectBuffer, 0, renderInstanceCount, sizeof(VkDrawIndirectCommand));
    }
}

/******************************************************************************
* Prepares the Vulkan buffer with the per-instance transformation matrices.
******************************************************************************/
VkBuffer VulkanMeshPrimitive::getInstanceTMBuffer(VulkanSceneRenderer* renderer)
{
    OVITO_ASSERT(useInstancedRendering() && perInstanceTMs());

    // The look-up key for storing the per-instance TMs in the Vulkan buffer cache.
    RendererResourceKey<VulkanMeshPrimitive, ConstDataBufferPtr> instanceTMsKey{ perInstanceTMs() };

    // Upload the per-instance TMs to GPU memory.
    VkBuffer instanceTMBuffer = renderer->context()->createCachedBuffer(instanceTMsKey, perInstanceTMs()->size() * 3 * sizeof(Vector_4<float>), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, [&](void* buffer) {
        Vector_4<float>* row = reinterpret_cast<Vector_4<float>*>(buffer);
        for(const AffineTransformation& tm : ConstDataBufferAccess<AffineTransformation>(perInstanceTMs())) {
            *row++ = tm.row(0).toDataType<float>();
            *row++ = tm.row(1).toDataType<float>();
            *row++ = tm.row(2).toDataType<float>();
        }
    });

    return instanceTMBuffer;
}

/******************************************************************************
* Generates the list of wireframe line elements.
******************************************************************************/
const ConstDataBufferPtr& VulkanMeshPrimitive::wireframeLines(VulkanSceneRenderer* renderer)
{
    OVITO_ASSERT(emphasizeEdges());

    if(!_wireframeLines) {
		// Count how many polygon edge are in the mesh.
		size_t numVisibleEdges = 0;
		for(const TriMeshFace& face : mesh().faces()) {
			for(size_t e = 0; e < 3; e++)
				if(face.edgeVisible(e)) numVisibleEdges++;
		}

		// Allocate storage buffer for line elements.
        DataBufferAccessAndRef<Point3> lines = DataOORef<DataBuffer>::create(renderer->dataset(), ExecutionContext::Scripting, numVisibleEdges * 2, DataBuffer::Float, 3, 0, false);

		// Generate line elements.
        Point3* outVert = lines.begin();
		for(const TriMeshFace& face : mesh().faces()) {
			for(size_t e = 0; e < 3; e++) {
				if(face.edgeVisible(e)) {
					*outVert++ = mesh().vertex(face.vertex(e));
					*outVert++ = mesh().vertex(face.vertex((e+1)%3));
				}
			}
		}
        OVITO_ASSERT(outVert == lines.end());

        _wireframeLines = lines.take();
    }

    return _wireframeLines;
}

/******************************************************************************
* Renders the mesh wireframe edges.
******************************************************************************/
void VulkanMeshPrimitive::renderWireframe(VulkanSceneRenderer* renderer, Pipelines& pipelines, const QMatrix4x4& mvp)
{
    bool useBlending = (uniformColor().a() < 1.0);
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    OVITO_ASSERT(!renderer->isPicking());

    // Bind the pipeline.
    if(!useInstancedRendering()) {
        pipelines.create(renderer, pipelines.mesh_wireframe).bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
        pipelineLayout = pipelines.mesh_wireframe.layout();
    }
    else {
        pipelines.create(renderer, pipelines.mesh_wireframe_instanced).bind(*renderer->context(), renderer->currentCommandBuffer(), useBlending);
        pipelineLayout = pipelines.mesh_wireframe_instanced.layout();
    }

    // Pass transformation matrix to vertex shader as a push constant.
    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Matrix_4<float>), mvp.data());

    // Pass uniform line color to fragment shader as a push constant.
    ColorAT<float> wireframeColor(0.1f, 0.1f, 0.1f, (float)uniformColor().a());
    renderer->deviceFunctions()->vkCmdPushConstants(renderer->currentCommandBuffer(), pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Matrix_4<float>), sizeof(wireframeColor), wireframeColor.data());

    // Bind vertex buffer for wireframe vertex positions.
    VkBuffer buffer = renderer->context()->uploadDataBuffer(wireframeLines(renderer), renderer->currentResourceFrame(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    VkDeviceSize offset = 0;
    renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 0, 1, &buffer, &offset);

    // Bind vertex buffer for instance TMs.
    if(useInstancedRendering()) {
        VkBuffer buffer = getInstanceTMBuffer(renderer);
        renderer->deviceFunctions()->vkCmdBindVertexBuffers(renderer->currentCommandBuffer(), 1, 1, &buffer, &offset);
    }

    // Draw lines.
    renderer->deviceFunctions()->vkCmdDraw(renderer->currentCommandBuffer(), wireframeLines(renderer)->size(), useInstancedRendering() ? perInstanceTMs()->size() : 1, 0, 0);
}

}	// End of namespace
