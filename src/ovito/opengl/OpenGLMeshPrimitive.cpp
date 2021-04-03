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
#include "OpenGLMeshPrimitive.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

#include <boost/range/irange.hpp>

namespace Ovito {

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLMeshPrimitive::render(OpenGLSceneRenderer* renderer)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(faceCount() == 0)
		return;
    if(useInstancedRendering() && perInstanceTMs()->size() == 0)
        return;

    // Render wireframe lines.
    if(emphasizeEdges() && !renderer->isPicking())
        renderWireframe(renderer);

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
    if(!useInstancedRendering()) {
        if(!renderer->isPicking())
			shader.load("mesh", "mesh/mesh.vert", "mesh/mesh.frag");
        else
			shader.load("mesh_picking", "mesh/mesh_picking.vert", "mesh/mesh_picking.frag");
    }
    else {
        if(!renderer->isPicking()) {
            if(!perInstanceColors())
				shader.load("mesh_instanced", "mesh/mesh_instanced.vert", "mesh/mesh_instanced.frag");
            else
				shader.load("mesh_instanced_with_colors", "mesh/mesh_instanced_with_colors.vert", "mesh/mesh_instanced_with_colors.frag");
        }
        else
			shader.load("mesh_instanced_picking", "mesh/mesh_instanced_picking.vert", "mesh/mesh_instanced_picking.frag");
    }

    // Are we rendering a semi-transparent mesh?
    bool useBlending = !renderer->isPicking() && !isFullyOpaque();
	if(useBlending) shader.enableBlending();

    // Turn back-face culling off if requested.
	if(!cullFaces()) {
		OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_CULL_FACE));
	}

    // Apply optional positive depth-offset to mesh faces to make the wireframe lines fully visible.
	if(emphasizeEdges() && !renderer->isPicking()) {
		OVITO_CHECK_OPENGL(renderer, renderer->glEnable(GL_POLYGON_OFFSET_FILL));
		OVITO_CHECK_OPENGL(renderer, renderer->glPolygonOffset(1.0f, 1.0f));
	}

	// Pass picking base ID to shader.
	if(renderer->isPicking()) {
		shader.setPickingBaseId(renderer->registerSubObjectIDs(useInstancedRendering() ? perInstanceTMs()->size() : faceCount()));
	}

    // The look-up key for the buffer cache.
    RendererResourceKey<OpenGLMeshPrimitive, const TriMesh*, int, std::vector<ColorA>, ColorA> meshCacheKey{
        &mesh(),
        faceCount(),
        materialColors(),
        uniformColor()
    };

    // Upload vertex buffer to GPU memory.
    QOpenGLBuffer meshBuffer = shader.createCachedBuffer(meshCacheKey, faceCount() * 3 * sizeof(ColoredVertexWithNormal), renderer->currentResourceFrame(), QOpenGLBuffer::VertexBuffer, [&](void* buffer) {
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
                *faceNormal = static_cast<Vector_3<float>>(d1.cross(d2));
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
                    rv->position = static_cast<Point_3<float>>(mesh().vertex(face->vertex(v)));
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
                    rv->normal = static_cast<Vector_3<float>>(*faceNormal++);
                    rv->position = static_cast<Point_3<float>>(mesh().vertex(face->vertex(v)));
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

	// Bind vertex buffer to vertex attributes.
	shader.bindBuffer(meshBuffer, "position", GL_FLOAT, 3, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, position), OpenGLShaderHelper::PerVertex);
	shader.bindBuffer(meshBuffer, "normal",   GL_FLOAT, 3, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, normal),   OpenGLShaderHelper::PerVertex);
	shader.bindBuffer(meshBuffer, "color",    GL_FLOAT, 4, sizeof(ColoredVertexWithNormal), offsetof(ColoredVertexWithNormal, color),    OpenGLShaderHelper::PerVertex);

    // The number of instances the drawing command should draw.
    uint32_t renderInstanceCount = 1;

    if(useInstancedRendering()) {
        renderInstanceCount = perInstanceTMs()->size();

        // Upload the per-instance TMs to GPU memory.
        QOpenGLBuffer instanceTMBuffer = getInstanceTMBuffer(renderer, shader);

        // Bind buffer with the instance matrices.
		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceTMBuffer, "instance_tm_row1", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 0 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance));
		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceTMBuffer, "instance_tm_row2", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 1 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance));
		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceTMBuffer, "instance_tm_row3", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 2 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance));

        if(perInstanceColors() && !renderer->isPicking()) {
            // Upload the per-instance colors to GPU memory.
            QOpenGLBuffer instanceColorBuffer = shader.uploadDataBuffer(perInstanceColors(), renderer->currentResourceFrame(), QOpenGLBuffer::VertexBuffer);

            // Bind buffer with the instance colors.
      		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceColorBuffer, "instance_color", GL_FLOAT, 4, sizeof(ColorAT<float>), 0, OpenGLShaderHelper::PerInstance));
        }
    }

    if(renderer->isPicking() || isFullyOpaque()) {
		// Draw triangles in regular storage order (not sorted).
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawArraysInstanced(GL_TRIANGLES, 0, faceCount() * 3, renderInstanceCount));
	}
    else if(_depthSortingMode == ConvexShapeMode) {
        // Assuming that the input mesh is convex, render semi-transparent triangles in two passes: 
        // First, render triangles facing away from the viewer, then render triangles facing toward the viewer.
        // Each time we pass the entire triangle list to OpenGL and use OpenGL's backface/frontfrace culling
        // option to render the right subset of triangles.
        if(!cullFaces()) {
            // First pass is only needed if backface culling is not active.
			renderer->glCullFace(GL_FRONT);
			renderer->glEnable(GL_CULL_FACE);
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawArraysInstanced(GL_TRIANGLES, 0, faceCount() * 3, renderInstanceCount));
        }
        // Now render front-facing triangles only.
		renderer->glCullFace(GL_BACK);
		renderer->glEnable(GL_CULL_FACE);
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawArraysInstanced(GL_TRIANGLES, 0, faceCount() * 3, renderInstanceCount));
    }
    else if(!useInstancedRendering()) {
        // Create a buffer for an indexed drawing command to render the triangles in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = renderer->modelViewTM().inverse().column(2);

        // The caching key for the index buffer.
        RendererResourceKey<OpenGLMeshPrimitive, const TriMesh*, int, Vector3> indexBufferCacheKey{
			&mesh(),
        	faceCount(),
			direction
        };

        // Create index buffer with three entries per triangle face.
        QOpenGLBuffer indexBuffer = shader.createCachedBuffer(indexBufferCacheKey, faceCount() * 3 * sizeof(uint32_t), renderer->currentResourceFrame(), QOpenGLBuffer::IndexBuffer, [&](void* buffer) {

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
            boost::transform(faceCenters, distances.begin(), [direction = Vector_3<float>(direction)](const Vector_3<float>& v) {
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
		if(!indexBuffer.bind())
			renderer->throwException(QStringLiteral("Failed to bind OpenGL index buffer for shader '%1'.").arg(shader.shaderObject().objectName()));

        // Draw triangles in sorted order.
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElementsInstanced(GL_TRIANGLES, faceCount() * 3, GL_UNSIGNED_INT, nullptr, renderInstanceCount));

		indexBuffer.release();
    }
	else {
        // Create a buffer for an indirect drawing command to render the mesh instances in back-to-front order. 

        // Viewing direction in object space:
        const Vector3 direction = renderer->modelViewTM().inverse().column(2);

        // The caching key for the indirect drawing command buffer.
        RendererResourceKey<OpenGLMeshPrimitive, ConstDataBufferPtr, Vector3> indirectBufferCacheKey{
            perInstanceTMs(),
            direction
        };

		// Data structure used by the glMultiDrawArraysIndirect() command:
		struct DrawArraysIndirectCommand {
			GLuint count;
			GLuint instanceCount;
			GLuint first;
			GLuint baseInstance;
		};

        // Create indirect drawing buffer.
        QOpenGLBuffer indirectBuffer = shader.createCachedBuffer(indirectBufferCacheKey, renderInstanceCount * sizeof(DrawArraysIndirectCommand), renderer->currentResourceFrame(), static_cast<QOpenGLBuffer::Type>(GL_DRAW_INDIRECT_BUFFER), [&](void* buffer) {

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

            // Fill the buffer with DrawArraysIndirectCommand records.
            DrawArraysIndirectCommand* dst = reinterpret_cast<DrawArraysIndirectCommand*>(buffer);
            for(uint32_t index : sortedIndices) {
                dst->count = faceCount() * 3;
                dst->instanceCount = 1;
                dst->first = 0;
                dst->baseInstance = index;
                ++dst;
            }
        });

		// Bind the GL buffer.
		if(!indirectBuffer.bind())
			renderer->throwException(QStringLiteral("Failed to bind OpenGL indirect drawing buffer for shader '%1'.").arg(shader.shaderObject().objectName()));

        // Draw triangle strip instances in sorted order.
		OVITO_CHECK_OPENGL(renderer, renderer->glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, renderInstanceCount, 0));

		indirectBuffer.release();		
	}

	// Reset depth offset.
	if(emphasizeEdges() && !renderer->isPicking()) {
		renderer->glDisable(GL_POLYGON_OFFSET_FILL);
	}
}

/******************************************************************************
* Prepares the Vulkan buffer with the per-instance transformation matrices.
******************************************************************************/
QOpenGLBuffer OpenGLMeshPrimitive::getInstanceTMBuffer(OpenGLSceneRenderer* renderer, OpenGLShaderHelper& shader)
{
    OVITO_ASSERT(useInstancedRendering() && perInstanceTMs());

    // The look-up key for storing the per-instance TMs in the Vulkan buffer cache.
    RendererResourceKey<OpenGLMeshPrimitive, ConstDataBufferPtr> instanceTMsKey{ perInstanceTMs() };

    // Upload the per-instance TMs to GPU memory.
    return shader.createCachedBuffer(instanceTMsKey, perInstanceTMs()->size() * 3 * sizeof(Vector_4<float>), renderer->currentResourceFrame(), QOpenGLBuffer::VertexBuffer, [&](void* buffer) {
        Vector_4<float>* row = reinterpret_cast<Vector_4<float>*>(buffer);
        for(const AffineTransformation& tm : ConstDataBufferAccess<AffineTransformation>(perInstanceTMs())) {
            *row++ = static_cast<Vector_4<float>>(tm.row(0));
            *row++ = static_cast<Vector_4<float>>(tm.row(1));
            *row++ = static_cast<Vector_4<float>>(tm.row(2));
        }
    });
}

/******************************************************************************
* Generates the list of wireframe line elements.
******************************************************************************/
const ConstDataBufferPtr& OpenGLMeshPrimitive::wireframeLines(SceneRenderer* renderer)
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
void OpenGLMeshPrimitive::renderWireframe(OpenGLSceneRenderer* renderer)
{
    OVITO_ASSERT(!renderer->isPicking());

	OpenGLShaderHelper shader(renderer);
    if(!useInstancedRendering())
		shader.load("mesh_wireframe", "mesh/mesh_wireframe.vert", "mesh/mesh_wireframe.frag");
	else
		shader.load("mesh_wireframe_instanced", "mesh/mesh_wireframe_instanced.vert", "mesh/mesh_wireframe_instanced.frag");

    bool useBlending = (uniformColor().a() < 1.0);
	if(useBlending) shader.enableBlending();

	// Pass uniform line color to fragment shader as a uniform value.
    ColorA wireframeColor(0.1, 0.1, 0.1, uniformColor().a());
	shader.setUniformValue("color", wireframeColor);

    // Bind vertex buffer for wireframe vertex positions.
    QOpenGLBuffer buffer = shader.uploadDataBuffer(wireframeLines(renderer), renderer->currentResourceFrame(), QOpenGLBuffer::VertexBuffer);
	shader.bindBuffer(buffer, "position", GL_FLOAT, 3, sizeof(Point_3<float>), 0, OpenGLShaderHelper::PerVertex);

    // Bind vertex buffer for instance TMs.
    if(useInstancedRendering()) {
        // Upload the per-instance TMs to GPU memory.
        QOpenGLBuffer instanceTMBuffer = getInstanceTMBuffer(renderer, shader);
        // Bind buffer with the instance matrices.
		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceTMBuffer, "instance_tm_row1", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 0 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance));
		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceTMBuffer, "instance_tm_row2", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 1 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance));
		OVITO_CHECK_OPENGL(renderer, shader.bindBuffer(instanceTMBuffer, "instance_tm_row3", GL_FLOAT, 4, 3 * sizeof(Vector_4<float>), 2 * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance));
    }

    // Draw lines.
	OVITO_CHECK_OPENGL(renderer, renderer->glDrawArraysInstanced(GL_LINES, 0, wireframeLines(renderer)->size(), useInstancedRendering() ? perInstanceTMs()->size() : 1));
}

}	// End of namespace
