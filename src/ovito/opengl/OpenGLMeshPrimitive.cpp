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
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

#include <boost/range/irange.hpp>

namespace Ovito {

/******************************************************************************
* Renders a triangle mesh.
******************************************************************************/
void OpenGLSceneRenderer::renderMeshImplementation(const MeshPrimitive& primitive)
{
    QOpenGLTexture* colorMapTexture = nullptr;

	// Stores data of a single vertex passed to the shader.
	struct ColoredVertexWithNormal {
		Point_3<float> position;
		Vector_3<float> normal;
		ColorAT<float> color;
	};

    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!primitive.mesh() || primitive.mesh()->faceCount() == 0)
		return;
    if(primitive.useInstancedRendering() && primitive.perInstanceTMs()->size() == 0)
        return;

	rebindVAO();

    // Render wireframe lines.
    if(primitive.emphasizeEdges() && !isPicking())
        renderMeshWireframeImplementation(primitive);

    // The mesh object to be rendered.
    const TriMeshObject& mesh = *primitive.mesh();

    // Decide whether per-pixel pseudo-color mapping is used.
    bool renderWithPseudoColorMapping = false;
    if(primitive.pseudoColorMapping().isValid() && !isPicking() && !primitive.useInstancedRendering()) {
        if(!mesh.hasVertexColors() && mesh.hasVertexPseudoColors())
            renderWithPseudoColorMapping = true;
        else if(!mesh.hasFaceColors() && mesh.hasFacePseudoColors())
            renderWithPseudoColorMapping = true;
    }

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(this);
    if(!primitive.useInstancedRendering()) {
        if(isPicking())
			shader.load("mesh_picking", "mesh/mesh_picking.vert", "mesh/mesh_picking.frag");
        else if(renderWithPseudoColorMapping)
			shader.load("mesh_color_mapping", "mesh/mesh_color_mapping.vert", "mesh/mesh_color_mapping.frag");
        else
			shader.load("mesh", "mesh/mesh.vert", "mesh/mesh.frag");
        shader.setInstanceCount(1);
    }
    else {
        OVITO_ASSERT(!renderWithPseudoColorMapping); // Note: Color mapping has not been implemented yet for instanced mesh primtives.
        if(!isPicking()) {
            if(!primitive.perInstanceColors())
				shader.load("mesh_instanced", "mesh/mesh_instanced.vert", "mesh/mesh_instanced.frag");
            else
				shader.load("mesh_instanced_with_colors", "mesh/mesh_instanced_with_colors.vert", "mesh/mesh_instanced_with_colors.frag");
        }
        else {
			shader.load("mesh_instanced_picking", "mesh/mesh_instanced_picking.vert", "mesh/mesh_instanced_picking.frag");
        }
        shader.setInstanceCount(primitive.perInstanceTMs()->size());
    }
    shader.setVerticesPerInstance(mesh.faceCount() * 3);

    // Are we rendering a semi-transparent mesh?
    bool useBlending = !isPicking() && !primitive.isFullyOpaque() && !orderIndependentTransparency();
	if(useBlending) shader.enableBlending();

    // Turn back-face culling off if requested.
	if(!primitive.cullFaces()) {
		OVITO_CHECK_OPENGL(this, glDisable(GL_CULL_FACE));
	}

    // Apply optional positive depth-offset to mesh faces to make the wireframe lines fully visible.
	if(primitive.emphasizeEdges() && !isPicking()) {
		OVITO_CHECK_OPENGL(this, glEnable(GL_POLYGON_OFFSET_FILL));
		OVITO_CHECK_OPENGL(this, glPolygonOffset(1.0f, 1.0f));
	}

	// Pass picking base ID to shader.
	if(isPicking()) {
		shader.setPickingBaseId(registerSubObjectIDs(primitive.useInstancedRendering() ? primitive.perInstanceTMs()->size() : mesh.faceCount()));
	}

    // The look-up key for the buffer cache.
    RendererResourceKey<struct MeshBufferCache, DataOORef<const TriMeshObject>, std::vector<ColorA>, ColorA, Color> meshCacheKey{
        primitive.mesh(),
        primitive.materialColors(),
        primitive.uniformColor(),
        primitive.faceSelectionColor()
    };

    // Upload vertex buffer to GPU memory.
    QOpenGLBuffer meshBuffer = shader.createCachedBuffer(std::move(meshCacheKey), sizeof(ColoredVertexWithNormal), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerVertex, [&](void* buffer) {
        ColoredVertexWithNormal* renderVertices = reinterpret_cast<ColoredVertexWithNormal*>(buffer);
        if(!mesh.hasNormals()) {
            quint32 allMask = 0;

            // Compute face normals.
            std::vector<Vector_3<float>> faceNormals(mesh.faceCount());
            auto faceNormal = faceNormals.begin();
            for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
                const Point3& p0 = mesh.vertex(face->vertex(0));
                Vector3 d1 = mesh.vertex(face->vertex(1)) - p0;
                Vector3 d2 = mesh.vertex(face->vertex(2)) - p0;
                *faceNormal = d1.cross(d2).toDataType<float>();
                if(*faceNormal != Vector_3<float>::Zero()) {
                    allMask |= face->smoothingGroups();
                }
            }

            // Initialize render vertices.
            ColoredVertexWithNormal* rv = renderVertices;
            faceNormal = faceNormals.begin();
            ColorAT<float> defaultVertexColor = primitive.uniformColor().toDataType<float>();
            for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {

                // Initialize render vertices for this face.
                for(size_t v = 0; v < 3; v++, rv++) {
                    if(face->smoothingGroups())
                        rv->normal = Vector_3<float>::Zero();
                    else
                        rv->normal = *faceNormal;
                    rv->position = mesh.vertex(face->vertex(v)).toDataType<float>();
                    if(mesh.hasVertexColors()) {
                        rv->color = mesh.vertexColor(face->vertex(v)).toDataType<float>();
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(renderWithPseudoColorMapping && mesh.hasVertexPseudoColors()) {
                        rv->color.r() = mesh.vertexPseudoColor(face->vertex(v));
                    }
                    else if(mesh.hasFaceColors()) {
                        rv->color = mesh.faceColor(face - mesh.faces().constBegin()).toDataType<float>();
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(renderWithPseudoColorMapping && mesh.hasFacePseudoColors()) {
                        rv->color.r() = mesh.facePseudoColor(face - mesh.faces().constBegin());
                    }
                    else if(face->materialIndex() < primitive.materialColors().size() && face->materialIndex() >= 0) {
                        rv->color = primitive.materialColors()[face->materialIndex()].toDataType<float>();
                    }
                    else {
                        rv->color = defaultVertexColor;
                    }

                    // Override color of faces that are selected.
                    if(face->isSelected() && isInteractive() && !isPicking()) {
                        if(!renderWithPseudoColorMapping)
                            rv->color = ColorAT<float>(primitive.faceSelectionColor());
                        else
                            rv->color.r() = std::numeric_limits<float>::infinity();
                    }
                }
            }

            if(allMask) {
                std::vector<Vector_3<float>> groupVertexNormals(mesh.vertexCount());
                for(int group = 0; group < OVITO_MAX_NUM_SMOOTHING_GROUPS; group++) {
                    quint32 groupMask = quint32(1) << group;
                    if((allMask & groupMask) == 0)
                        continue;	// Group is not used.

                    // Reset work arrays.
                    std::fill(groupVertexNormals.begin(), groupVertexNormals.end(), Vector_3<float>::Zero());

                    // Compute vertex normals at original vertices for current smoothing group.
                    faceNormal = faceNormals.begin();
                    for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
                        // Skip faces that do not belong to the current smoothing group.
                        if((face->smoothingGroups() & groupMask) == 0) continue;

                        // Add face's normal to vertex normals.
                        for(size_t fv = 0; fv < 3; fv++)
                            groupVertexNormals[face->vertex(fv)] += *faceNormal;
                    }

                    // Transfer vertex normals from original vertices to render vertices.
                    rv = renderVertices;
                    for(const auto& face : mesh.faces()) {
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
            const Vector3* faceNormal = mesh.normals().begin();
            ColorAT<float> defaultVertexColor = primitive.uniformColor().toDataType<float>();
            for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face) {
                // Initialize render vertices for this face.
                for(size_t v = 0; v < 3; v++, rv++) {
                    rv->normal = (*faceNormal++).toDataType<float>();
                    rv->position = mesh.vertex(face->vertex(v)).toDataType<float>();
                    if(mesh.hasVertexColors()) {
                        rv->color = mesh.vertexColor(face->vertex(v)).toDataType<float>();
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(renderWithPseudoColorMapping && mesh.hasVertexPseudoColors()) {
                        rv->color.r() = mesh.vertexPseudoColor(face->vertex(v));
                    }
                    else if(mesh.hasFaceColors()) {
                        rv->color = mesh.faceColor(face - mesh.faces().constBegin()).toDataType<float>();
                        if(defaultVertexColor.a() != 1) rv->color.a() = defaultVertexColor.a();
                    }
                    else if(renderWithPseudoColorMapping && mesh.hasFacePseudoColors()) {
                        rv->color.r() = mesh.facePseudoColor(face - mesh.faces().constBegin());
                    }
                    else if(face->materialIndex() >= 0 && face->materialIndex() < primitive.materialColors().size()) {
                        rv->color = primitive.materialColors()[face->materialIndex()].toDataType<float>();
                    }
                    else {
                        rv->color = defaultVertexColor;
                    }

                    // Override color of faces that are selected.
                    if(face->isSelected() && isInteractive()) {
                        if(!renderWithPseudoColorMapping)
                            rv->color = ColorAT<float>(primitive.faceSelectionColor());
                        else
                            rv->color.r() = std::numeric_limits<float>::infinity();
                    }
                }
            }
        }
    });

	// Bind vertex buffer to vertex attributes.
	shader.bindBuffer(meshBuffer, "position", GL_FLOAT, 3, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, position), OpenGLShaderHelper::PerVertex);
    if(!isPicking())
    	shader.bindBuffer(meshBuffer, "normal",   GL_FLOAT, 3, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, normal),   OpenGLShaderHelper::PerVertex);
        
	if(!renderWithPseudoColorMapping) {
        if(!isPicking() && (!primitive.useInstancedRendering() || !primitive.perInstanceColors())) {
            // Rendering with true RGBA colors.
            shader.bindBuffer(meshBuffer, "color", GL_FLOAT, 4, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, color), OpenGLShaderHelper::PerVertex);
        }
    }
    else {
        OVITO_ASSERT(!isPicking());
        
        // Rendering  with pseudo-colors and a color mapping function.
        shader.bindBuffer(meshBuffer, "pseudocolor", GL_FLOAT, 1, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, color), OpenGLShaderHelper::PerVertex);
        shader.setUniformValue("opacity", primitive.uniformColor().a());
        shader.setUniformValue("selection_color", ColorA(primitive.faceSelectionColor()));
        float minValue = primitive.pseudoColorMapping().minValue();
        float maxValue = primitive.pseudoColorMapping().maxValue();
        // Avoid division by zero due to degenerate value interval.
        if(minValue == maxValue) maxValue = std::nextafter(maxValue, std::numeric_limits<float>::max());
        shader.setUniformValue("color_range_min", minValue);
        shader.setUniformValue("color_range_max", maxValue);

        // Upload color map as a 1-d OpenGL texture.
        colorMapTexture = OpenGLResourceManager::instance()->uploadColorMap(primitive.pseudoColorMapping().gradient(), currentResourceFrame());
        colorMapTexture->bind();
    }

    if(primitive.useInstancedRendering()) {
        // Upload the per-instance TMs to GPU memory.
        QOpenGLBuffer instanceTMBuffer = getMeshInstanceTMBuffer(primitive, shader);

        // Bind buffer with the instance matrices.
		shader.bindBuffer(instanceTMBuffer, "instance_tm_row1", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 0 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);
		shader.bindBuffer(instanceTMBuffer, "instance_tm_row2", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 1 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);
		shader.bindBuffer(instanceTMBuffer, "instance_tm_row3", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 2 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);

        if(primitive.perInstanceColors() && !isPicking()) {
            // Upload the per-instance colors to GPU memory.
            QOpenGLBuffer instanceColorBuffer = shader.uploadDataBuffer(primitive.perInstanceColors(), OpenGLShaderHelper::PerInstance, QOpenGLBuffer::VertexBuffer);

            // Bind buffer with the instance colors.
      		shader.bindBuffer(instanceColorBuffer, "instance_color", GL_FLOAT, 4, sizeof(ColorAT<float>), 0, OpenGLShaderHelper::PerInstance);
        }
    }

    if(!useBlending) {
		// Draw triangles in regular storage order (not sorted).
		shader.drawArrays(GL_TRIANGLES);
	}
    else if(primitive.depthSortingMode() == MeshPrimitive::ConvexShapeMode) {
        OVITO_ASSERT(!orderIndependentTransparency() && !isPicking());

        // Assuming that the input mesh is convex, render semi-transparent triangles in two passes: 
        // First, render triangles facing away from the viewer, then render triangles facing toward the viewer.
        // Each time we pass the entire triangle list to OpenGL and use OpenGL's backface/frontfrace culling
        // option to render the right subset of triangles.
        if(!primitive.cullFaces()) {
            // First pass is only needed if backface culling is not active.
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
    		shader.drawArrays(GL_TRIANGLES);
        }
        // Now render front-facing triangles only.
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
        shader.drawArrays(GL_TRIANGLES);
    }
    else if(!primitive.useInstancedRendering()) {
        OVITO_ASSERT(!orderIndependentTransparency() && !isPicking());

        // Create a buffer for an indexed drawing command to render the triangles in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = modelViewTM().inverse().column(2);

        // The caching key for the index buffer.
        RendererResourceKey<struct DepthSortingCache, DataOORef<const TriMeshObject>, Vector3> indexBufferCacheKey{ primitive.mesh(), direction };

        // Create index buffer with three entries per triangle face.
        QOpenGLBuffer indexBuffer = shader.createCachedBuffer(std::move(indexBufferCacheKey), sizeof(GLsizei), QOpenGLBuffer::IndexBuffer, OpenGLShaderHelper::PerVertex, [&](void* buffer) {

            // Compute each face's center point.
            std::vector<Vector_3<float>> faceCenters(mesh.faceCount());
            auto tc = faceCenters.begin();
            for(auto face = mesh.faces().cbegin(); face != mesh.faces().cend(); ++face, ++tc) {
                // Compute centroid of triangle.
                const auto& v1 = mesh.vertex(face->vertex(0));
                const auto& v2 = mesh.vertex(face->vertex(1));
                const auto& v3 = mesh.vertex(face->vertex(2));
                tc->x() = (float)(v1.x() + v2.x() + v3.x()) / 3.0f;
                tc->y() = (float)(v1.y() + v2.y() + v3.y()) / 3.0f;
                tc->z() = (float)(v1.z() + v2.z() + v3.z()) / 3.0f;
            }

            // Next, compute distance of each face from the camera along the viewing direction (=camera z-axis).
            std::vector<FloatType> distances(mesh.faceCount());
            boost::transform(faceCenters, distances.begin(), [direction = direction.toDataType<float>()](const Vector_3<float>& v) {
                return direction.dot(v);
            });

            // Create index array with all face indices.
            std::vector<uint32_t> sortedIndices(mesh.faceCount());
            std::iota(sortedIndices.begin(), sortedIndices.end(), (uint32_t)0);

            // Sort face indices with respect to distance (back-to-front order).
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
                return distances[a] < distances[b];
            });

            // Fill the index buffer with vertex indices to render.
            GLsizei* dst = reinterpret_cast<GLsizei*>(buffer);
            for(uint32_t index : sortedIndices) {
                *dst++ = index * 3;
                *dst++ = index * 3 + 1;
                *dst++ = index * 3 + 2;
            }
        });

        // Bind index buffer.
		if(!indexBuffer.bind())
			throwRendererException(QStringLiteral("Failed to bind OpenGL index buffer for shader '%1'.").arg(shader.shaderObject().objectName()));

        // Draw triangles in sorted order.
		OVITO_CHECK_OPENGL(this, glDrawElements(GL_TRIANGLES, mesh.faceCount() * 3, GL_UNSIGNED_INT, nullptr));

		indexBuffer.release();
    }
	else {
        OVITO_ASSERT(!orderIndependentTransparency() && !isPicking());
        // Render the mesh instances in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = modelViewTM().inverse().column(2);

        // The caching key for the ordering of the primitives.
        RendererResourceKey<struct OrderingCache, ConstDataBufferPtr, Vector3> orderingCacheKey{ primitive.perInstanceTMs(), direction };

        // Render the primitives.
        shader.drawArraysOrdered(GL_TRIANGLES, std::move(orderingCacheKey), [&]() {

            // First, compute distance of each instance from the camera along the viewing direction (=camera z-axis).
            std::vector<FloatType> distances(shader.instanceCount());
			boost::transform(boost::irange<size_t>(0, shader.instanceCount()), distances.begin(), [direction, tmArray = ConstDataBufferAccess<AffineTransformation>(primitive.perInstanceTMs())](size_t i) {
				return direction.dot(tmArray[i].translation());
			});

            // Create index array with all indices.
            std::vector<uint32_t> sortedIndices(shader.instanceCount());
            std::iota(sortedIndices.begin(), sortedIndices.end(), (uint32_t)0);

            // Sort indices with respect to distance (back-to-front order).
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
                return distances[a] < distances[b];
            });

            return sortedIndices;
        });
	}

	// Reset depth offset.
	if(primitive.emphasizeEdges() && !isPicking()) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

    // Unbind color mapping texture.
    if(colorMapTexture) {
        colorMapTexture->release();
    }
}

/******************************************************************************
* Prepares the OpenGL buffer with the per-instance transformation matrices for 
* rendering a set of meshes.
******************************************************************************/
QOpenGLBuffer OpenGLSceneRenderer::getMeshInstanceTMBuffer(const MeshPrimitive& primitive, OpenGLShaderHelper& shader)
{
    OVITO_ASSERT(primitive.useInstancedRendering());
    OVITO_ASSERT(primitive.perInstanceTMs());

    // The look-up key for storing the per-instance TMs in the cache.
    RendererResourceKey<struct InstanceTMCache, ConstDataBufferPtr> cacheKey(primitive.perInstanceTMs());

    // Upload the per-instance TMs to GPU memory.
    return shader.createCachedBuffer(std::move(cacheKey), 3 * sizeof(Vector_4<float>), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
        Vector_4<float>* row = reinterpret_cast<Vector_4<float>*>(buffer);
        for(const AffineTransformation& tm : ConstDataBufferAccess<AffineTransformation>(primitive.perInstanceTMs())) {
            *row++ = tm.row(0).toDataType<float>();
            *row++ = tm.row(1).toDataType<float>();
            *row++ = tm.row(2).toDataType<float>();
        }
    });
}

/******************************************************************************
* Generates the wireframe line elements for the visible edges of a mesh.
******************************************************************************/
ConstDataBufferPtr OpenGLSceneRenderer::generateMeshWireframeLines(const MeshPrimitive& primitive)
{
    OVITO_ASSERT(primitive.emphasizeEdges());
    OVITO_ASSERT(primitive.mesh());

    // Cache the wireframe geometry generated for the current mesh.
    RendererResourceKey<struct WireframeCache, DataOORef<const TriMeshObject>> cacheKey(primitive.mesh());
    ConstDataBufferPtr& wireframeLines = OpenGLResourceManager::instance()->lookup<ConstDataBufferPtr>(std::move(cacheKey), currentResourceFrame());

    if(!wireframeLines) {
        const TriMeshObject& mesh = *primitive.mesh();

		// Count how many polygon edge are in the mesh.
		size_t numVisibleEdges = 0;
		for(const TriMeshFace& face : mesh.faces()) {
			for(size_t e = 0; e < 3; e++)
				if(face.edgeVisible(e)) numVisibleEdges++;
		}

		// Allocate storage buffer for line elements.
        DataBufferAccessAndRef<Point3> lines = DataBufferPtr::create(dataset(), numVisibleEdges * 2, DataBuffer::Float, 3);

		// Generate line elements.
        Point3* outVert = lines.begin();
		for(const TriMeshFace& face : mesh.faces()) {
			for(size_t e = 0; e < 3; e++) {
				if(face.edgeVisible(e)) {
					*outVert++ = mesh.vertex(face.vertex(e));
					*outVert++ = mesh.vertex(face.vertex((e+1)%3));
				}
			}
		}
        OVITO_ASSERT(outVert == lines.end());

        wireframeLines = lines.take();
    }

    return wireframeLines;
}

/******************************************************************************
* Renders just the edges of a triangle mesh as a wireframe model.
******************************************************************************/
void OpenGLSceneRenderer::renderMeshWireframeImplementation(const MeshPrimitive& primitive)
{
    OVITO_ASSERT(!isPicking());

	OpenGLShaderHelper shader(this);
    if(!primitive.useInstancedRendering())
		shader.load("mesh_wireframe", "mesh/mesh_wireframe.vert", "mesh/mesh_wireframe.frag");
	else
		shader.load("mesh_wireframe_instanced", "mesh/mesh_wireframe_instanced.vert", "mesh/mesh_wireframe_instanced.frag");

    bool useBlending = (primitive.uniformColor().a() < 1.0) && !orderIndependentTransparency();
	if(useBlending) shader.enableBlending();

	// Pass uniform line color to fragment shader as a uniform value.
    ColorA wireframeColor(0.1, 0.1, 0.1, primitive.uniformColor().a());
	shader.setUniformValue("color", wireframeColor);

    // Get the wireframe lines geometry.
    ConstDataBufferPtr wireframeLinesBuffer = generateMeshWireframeLines(primitive);
    shader.setVerticesPerInstance(wireframeLinesBuffer->size());
    shader.setInstanceCount(primitive.useInstancedRendering() ? primitive.perInstanceTMs()->size() : 1);

    // Bind vertex buffer for wireframe vertex positions.
    QOpenGLBuffer buffer = shader.uploadDataBuffer(wireframeLinesBuffer, OpenGLShaderHelper::PerVertex, QOpenGLBuffer::VertexBuffer);
	shader.bindBuffer(buffer, "position", GL_FLOAT, 3, sizeof(Point_3<float>), 0, OpenGLShaderHelper::PerVertex);

    // Bind vertex buffer for instance TMs.
    if(primitive.useInstancedRendering()) {
        // Upload the per-instance TMs to GPU memory.
        QOpenGLBuffer instanceTMBuffer = getMeshInstanceTMBuffer(primitive, shader);
        // Bind buffer with the instance matrices.
		shader.bindBuffer(instanceTMBuffer, "instance_tm_row1", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 0 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);
		shader.bindBuffer(instanceTMBuffer, "instance_tm_row2", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 1 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);
		shader.bindBuffer(instanceTMBuffer, "instance_tm_row3", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 2 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);
    }

    // Draw lines.
	shader.drawArrays(GL_LINES);
}

}	// End of namespace
