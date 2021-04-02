////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include "OpenGLParticlePrimitive.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

#include <boost/range/irange.hpp>

namespace Ovito {

/******************************************************************************
* Renders the particles.
******************************************************************************/
void OpenGLParticlePrimitive::render(OpenGLSceneRenderer* renderer)
{
	OVITO_REPORT_OPENGL_ERRORS(renderer);

    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!positions() || positions()->size() == 0)
		return;
	if(indices() && indices()->size() == 0)
        return;

    // The effective number of particles being rendered:
    uint32_t particleCount = indices() ? indices()->size() : positions()->size();
    uint32_t verticesPerParticle = 0;

    // Are we rendering semi-transparent particles?
    bool useBlending = !renderer->isPicking() && (transparencies() != nullptr);

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
    switch(particleShape()) {
        case SquareCubicShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->isPicking())
					shader.load("cube", "particles/cube/cube.vs", "particles/cube/cube.fs");
                else
					shader.load("cube_picking", "particles/cube/cube_picking.vs", "particles/cube/cube_picking.fs");
                verticesPerParticle = 14; // Cube rendered as triangle strip.
            }
            else {
                if(!renderer->isPicking()) 
					shader.load("square", "particles/square/square.vs", "particles/square/square.fs");
                else
					shader.load("square_picking", "particles/square/square_picking.vs", "particles/square/square_picking.fs");
                verticesPerParticle = 4; // Square rendered as triangle strip.
            }
            break;
        case BoxShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->isPicking())
					shader.load("box", "particles/box/box.vs", "particles/box/box.fs");
                else
					shader.load("box_picking", "particles/box/box_picking.vs", "particles/box/box_picking.fs");
                verticesPerParticle = 14; // Box rendered as triangle strip.
            }
            else return;
            break;
        case SphericalShape:
            if(shadingMode() == NormalShading) {
                if(renderingQuality() >= HighQuality) {
                    if(!renderer->isPicking())
						shader.load("sphere", "particles/sphere/sphere.vs", "particles/sphere/sphere.fs");
                    else
						shader.load("sphere_picking", "particles/sphere/sphere_picking.vs", "particles/sphere/sphere_picking.fs");
                    verticesPerParticle = 14; // Cube rendered as triangle strip.
                }
                else {
                    if(!renderer->isPicking())
						shader.load("imposter", "particles/imposter/imposter.vs", "particles/imposter/imposter.fs");
                    else
						shader.load("imposter_picking", "particles/imposter/imposter_picking.vs", "particles/imposter/imposter_picking.fs");
                    verticesPerParticle = 4; // Square rendered as triangle strip.
                }
            }
            else {
                if(!renderer->isPicking())
					shader.load("circle", "particles/circle/circle.vs", "particles/circle/circle.fs");
                else
					shader.load("circle_picking", "particles/circle/circle_picking.vs", "particles/circle/circle_picking.fs");
                verticesPerParticle = 4; // Square rendered as triangle strip.
            }
            break;
        case EllipsoidShape:
            if(!renderer->isPicking())
				shader.load("ellipsoid", "particles/ellipsoid/ellipsoid.vs", "particles/ellipsoid/ellipsoid.fs");
            else
				shader.load("ellipsoid_picking", "particles/ellipsoid/ellipsoid_picking.vs", "particles/ellipsoid/ellipsoid_picking.fs");
            verticesPerParticle = 14; // Box rendered as triangle strip.
            break;
        case SuperquadricShape:
            if(!renderer->isPicking())
				shader.load("superquadric", "particles/superquadric/superquadric.vs", "particles/superquadric/superquadric.fs");
            else
				shader.load("superquadric_picking", "particles/superquadric/superquadric_picking.vs", "particles/superquadric/superquadric_picking.fs");
            verticesPerParticle = 14; // Box rendered as triangle strip.
            break;
        default:
            return;
    }

	// Pass picking base ID to shader.
	if(renderer->isPicking()) {
		shader.setPickingBaseId(renderer->registerSubObjectIDs(positions()->size(), indices()));
	}

    // Put positions and radii into one combined Vulkan buffer with 4 floats per particle.
    // Radii are optional and may be substituted with a uniform radius value.
    RendererResourceKey<OpenGLParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> positionRadiusCacheKey{
        indices(),
        positions(),
        radii(),
        radii() ? FloatType(0) : uniformRadius()
    };

    // Upload vertex buffer with the particle positions and radii.
    QOpenGLBuffer positionRadiusBuffer = shader.createCachedBuffer(positionRadiusCacheKey, particleCount * sizeof(Vector_4<float>), renderer->currentResourceFrame(), QOpenGLBuffer::VertexBuffer, [&](void* buffer) {
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

	// Bind vertex buffer to vertex attributes.
	shader.bindBuffer(positionRadiusBuffer, "position", GL_FLOAT, 3, sizeof(Vector_4<float>), 0, OpenGLShaderHelper::PerInstance);
	shader.bindBuffer(positionRadiusBuffer, "radius", GL_FLOAT, 1, sizeof(Vector_4<float>), sizeof(Vector_3<float>), OpenGLShaderHelper::PerInstance);

    if(!renderer->isPicking()) {

        // Put colors, transparencies and selection state into one combined Vulkan buffer with 4 floats per particle.
        RendererResourceKey<OpenGLParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, Color> colorSelectionCacheKey{ 
            indices(),
            colors(),
            transparencies(),
            selection(),
            colors() ? Color(0,0,0) : uniformColor()
        };

        // Upload vertex buffer with the particle colors.
        QOpenGLBuffer colorSelectionBuffer = shader.createCachedBuffer(colorSelectionCacheKey, particleCount * 4 * sizeof(float), renderer->currentResourceFrame(), QOpenGLBuffer::VertexBuffer, [&](void* buffer) {
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
		shader.bindBuffer(colorSelectionBuffer, "color", GL_FLOAT, 4, sizeof(ColorAT<float>), 0, OpenGLShaderHelper::PerInstance);
    }

	// Issue instance drawing command.
	OVITO_CHECK_OPENGL(renderer, renderer->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, verticesPerParticle, particleCount));
}

#if 0
/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLParticlePrimitive::render(OpenGLSceneRenderer* renderer)
{
	if(indices())
		_particleCount = indices()->size();
	else if(positions())
		_particleCount = positions()->size();
	else
		_particleCount = 0;

	if(particleCount() <= 0 || !renderer)
		return;

	// Upload data to OpenGL VBOs.
	_positionsBuffer.uploadData<Point3>(positions(), _verticesPerParticle);
	_radiiBuffer.uploadData<FloatType>(radii(), _verticesPerParticle);
	_colorsBuffer.uploadData<Color>(effectiveColors, _verticesPerParticle);
	_transparenciesBuffer.uploadData<FloatType>(transparencies(), _verticesPerParticle);
	_selectionBuffer.uploadData<int>(effectiveSelection, _verticesPerParticle);
	if(particleShape() == BoxShape || particleShape() == EllipsoidShape || particleShape() == SuperquadricShape) {
		_shapeBuffer.uploadData<Vector3>(asphericalShapes(), _verticesPerParticle);
		_orientationBuffer.uploadData<Quaternion>(orientations(), _verticesPerParticle);
		if(particleShape() == SuperquadricShape) {
			_roundnessBuffer.uploadData<Vector2>(roundness(), _verticesPerParticle);
		}
	}

	// Get the OpenGL shader program.
	QOpenGLShaderProgram* shader = _shader;
	if(!shader->bind())
		renderer->throwException(QStringLiteral("Failed to bind OpenGL shader program."));

	// Need to render only the front facing sides of the cubes.
	renderer->glCullFace(GL_BACK);
	renderer->glEnable(GL_CULL_FACE);

	// Set shader uniforms.
	shader->setUniformValue("is_picking_mode", (bool)renderer->isPicking());
	shader->setUniformValue("projection_matrix", (QMatrix4x4)renderer->projParams().projectionMatrix);
	shader->setUniformValue("inverse_projection_matrix", (QMatrix4x4)renderer->projParams().inverseProjectionMatrix);
	shader->setUniformValue("modelview_matrix", (QMatrix4x4)renderer->modelViewTM());
	shader->setUniformValue("modelviewprojection_matrix", (QMatrix4x4)(renderer->projParams().projectionMatrix * renderer->modelViewTM()));
	shader->setUniformValue("is_perspective", (bool)renderer->projParams().isPerspective);
	shader->setUniformValue("radius_scalingfactor", (float)pow(renderer->modelViewTM().determinant(), FloatType(1.0/3.0)));
	shader->setUniformValue("selection_color", selectionColor().r(), selectionColor().g(), selectionColor().b(), 1.0);

	if(!renderer->useGeometryShaders()) {
		if(_renderingTechnique == BOX_GEOMETRY) {
			// This is to draw the cube with a single triangle strip.
			// The cube vertices:
			static const QVector3D cubeVerts[14] = {
				{ 1,  1,  1},
				{ 1, -1,  1},
				{ 1,  1, -1},
				{ 1, -1, -1},
				{-1, -1, -1},
				{ 1, -1,  1},
				{-1, -1,  1},
				{ 1,  1,  1},
				{-1,  1,  1},
				{ 1,  1, -1},
				{-1,  1, -1},
				{-1, -1, -1},
				{-1,  1,  1},
				{-1, -1,  1},
			};
			OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("cubeVerts", cubeVerts, 14));
		}
		else if(_renderingTechnique == IMPOSTER_QUADS) {
			// The texture coordinates of a quad made of two triangles.
			static const QVector2D texcoords[6] = {{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
			OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("imposter_texcoords", texcoords, 6));

			// The coordinate offsets of the six vertices of a quad made of two triangles.
			static const QVector4D voffsets[6] = {{-1,-1,0,0},{1,-1,0,0},{1,1,0,0},{-1,-1,0,0},{1,1,0,0},{-1,1,0,0}};
			OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("imposter_voffsets", voffsets, 6));
		}
	}

	if(particleShape() != SphericalShape && !renderer->isPicking() && _renderingTechnique == BOX_GEOMETRY) {
		Matrix3 normal_matrix = renderer->modelViewTM().linear().inverse().transposed();
		normal_matrix.column(0).normalize();
		normal_matrix.column(1).normalize();
		normal_matrix.column(2).normalize();
		shader->setUniformValue("normal_matrix", (QMatrix3x3)normal_matrix);
		if(!renderer->useGeometryShaders()) {
			// The normal vectors for the cube triangle strip.
			static const QVector3D normals[14] = {
				{ 1,  0,  0},
				{ 1,  0,  0},
				{ 1,  0,  0},
				{ 1,  0,  0},
				{ 0,  0, -1},
				{ 0, -1,  0},
				{ 0, -1,  0},
				{ 0,  0,  1},
				{ 0,  0,  1},
				{ 0,  1,  0},
				{ 0,  1,  0},
				{ 0,  0, -1},
				{-1,  0,  0},
				{-1,  0,  0}
			};
			OVITO_CHECK_OPENGL(renderer, shader->setUniformValueArray("normals", normals, 14));
		}
	}

	GLint viewportCoords[4];
	renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords);
	shader->setUniformValue("viewport_origin", (float)viewportCoords[0], (float)viewportCoords[1]);
	shader->setUniformValue("inverse_viewport_size", 2.0f / (float)viewportCoords[2], 2.0f / (float)viewportCoords[3]);

	// Enable OpenGL blending mode when rendering semi-transparent particles.
	if(!renderer->isPicking() && transparencies()) {
		renderer->glEnable(GL_BLEND);
		renderer->glBlendEquation(GL_FUNC_ADD);
		renderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
	}

	if(renderer->isPicking()) {
		GLint pickingBaseID = renderer->registerSubObjectIDs(positions()->size());
		shader->setUniformValue("picking_base_id", pickingBaseID);
	}

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, shader);
	if(_shapeBuffer.isCreated())
		_shapeBuffer.bind(renderer, shader, "shape", GL_FLOAT, 0, 3);
	if(_orientationBuffer.isCreated())
		_orientationBuffer.bind(renderer, shader, "orientation", GL_FLOAT, 0, 4);
	if(_roundnessBuffer.isCreated())
		_roundnessBuffer.bind(renderer, shader, "roundness", GL_FLOAT, 0, 2);
	if(_radiiBuffer.isCreated())
		_radiiBuffer.bind(renderer, shader, "particle_radius", GL_FLOAT, 0, 1);
	else
		shader->setAttributeValue("particle_radius", (GLfloat)uniformRadius());
	if(_transparenciesBuffer.isCreated())
		_transparenciesBuffer.bind(renderer, shader, "transparency", GL_FLOAT, 0, 1);
	else
		shader->setAttributeValue("transparency", 0.0f);
	if(_selectionBuffer.isCreated())
		_selectionBuffer.bind(renderer, shader, "selection", GL_INT, 0, 1);
	else
		shader->setAttributeValue("selection", 0.0f);
	if(!renderer->isPicking()) {
		if(_colorsBuffer.isCreated())
			_colorsBuffer.bindColors(renderer, shader, 3);
		else
			_colorsBuffer.setUniformColor(renderer, shader, uniformColor());
	}

	if(renderer->useGeometryShaders()) {
		// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts when particles overlap.
		if(!transparencies() || renderer->isPicking()) {
			_indexBuffer.uploadData<int>(indices(), _verticesPerParticle);
		}
		else {
			// Create OpenGL index buffer which can be used with glDrawElements.
			_indexBuffer.create(QOpenGLBuffer::StaticDraw, particleCount());
			_indexBuffer.uploadData<int>(determineRenderingOrder(renderer), _verticesPerParticle);
		}

		if(!_indexBuffer.isCreated()) {
			// Fully opaque particles can be rendered in unsorted storage order.
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, particleCount()));
		}
		else {
			// Use indexed mode when rendering only a subset of particles.
			_indexBuffer.oglBuffer().bind();
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_POINTS, particleCount(), GL_UNSIGNED_INT, nullptr));
			_indexBuffer.oglBuffer().release();
		}
	}
	else if(!QOpenGLContext::currentContext()->isOpenGLES()) {		
		if(_renderingTechnique == BOX_GEOMETRY) {
			// Render a set of boxes using a glMultiDrawArrays() call.
			renderBoxGeometries(renderer);
		}
		else if(_renderingTechnique == IMPOSTER_QUADS) {
			// Render a set of imposters using triangles geometry.
			renderImposterGeometries(renderer);
		}
	}
	else {
#if 0
		// glMultiDrawArrays() is not available in OpenGL ES. Use glDrawElements() instead.
		int indicesPerElement = 3 * 12; // (3 vertices per triangle) * (12 triangles per cube).
		if(!renderer->isPicking() && transparencies()) {
			auto indices = determineRenderingOrder(renderer);
			_trianglePrimitiveVertexIndices.clear();
			_trianglePrimitiveVertexIndices.resize(particleCount() * indicesPerElement);
			auto pvi = _trianglePrimitiveVertexIndices.begin();
			for(const auto& index : indices) {
				int baseIndex = index * 14;
				for(int u = 2; u < 14; u++) {
					if((u & 1) == 0) {
						*pvi++ = baseIndex + u - 2;
						*pvi++ = baseIndex + u - 1;
						*pvi++ = baseIndex + u - 0;
					}
					else {
						*pvi++ = baseIndex + u - 0;
						*pvi++ = baseIndex + u - 1;
						*pvi++ = baseIndex + u - 2;
					}
				}
			}
			OVITO_ASSERT(pvi == _trianglePrimitiveVertexIndices.end());
		}
		else if(_trianglePrimitiveVertexIndices.size() < particleCount() * indicesPerElement) {
			_trianglePrimitiveVertexIndices.clear();
			_trianglePrimitiveVertexIndices.resize(particleCount() * indicesPerElement);
			auto pvi = _trianglePrimitiveVertexIndices.begin();
			for(int index = 0, baseIndex = 0; index < particleCount(); index++, baseIndex += 14) {
				for(int u = 2; u < 14; u++) {
					if((u & 1) == 0) {
						*pvi++ = baseIndex + u - 2;
						*pvi++ = baseIndex + u - 1;
						*pvi++ = baseIndex + u - 0;
					}
					else {
						*pvi++ = baseIndex + u - 0;
						*pvi++ = baseIndex + u - 1;
						*pvi++ = baseIndex + u - 2;
					}
				}
			}
			OVITO_ASSERT(pvi == _trianglePrimitiveVertexIndices.end());
		}
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, particleCount() * indicesPerElement, GL_UNSIGNED_INT, _trianglePrimitiveVertexIndices.data()));
#endif
	}

	// Detach VBOs.
	_positionsBuffer.detachPositions(renderer, shader);
	if(!renderer->isPicking() && _colorsBuffer.isCreated())
		_colorsBuffer.detachColors(renderer, shader);
	if(_shapeBuffer.isCreated())
		_shapeBuffer.detach(renderer, shader, "shape");
	if(_orientationBuffer.isCreated())
		_orientationBuffer.detach(renderer, shader, "orientation");
	if(_roundnessBuffer.isCreated())
		_roundnessBuffer.detach(renderer, shader, "roundness");
	if(_transparenciesBuffer.isCreated())
		_transparenciesBuffer.detach(renderer, shader, "transparency");
	if(_radiiBuffer.isCreated())
		_radiiBuffer.detach(renderer, shader, "particle_radius");
	if(_selectionBuffer.isCreated())
		_selectionBuffer.detach(renderer, shader, "selection");

	// Reset state.
	shader->release();
	renderer->glDisable(GL_CULL_FACE);
	renderer->glDisable(GL_BLEND);
}

/******************************************************************************
* Returns an array of particle indices, sorted back-to-front, which is used
* to render translucent particles.
******************************************************************************/
ConstDataBufferPtr OpenGLParticlePrimitive::determineRenderingOrder(OpenGLSceneRenderer* renderer) const
{
	OVITO_ASSERT(positions() && positions()->size() != 0);

	// Viewing direction in object space:
	const Vector3 direction = renderer->modelViewTM().inverse().column(2);

	if(!indices()) {

		// First, compute distance of each particle from the camera along the viewing direction (=camera z-axis).
		std::vector<FloatType> distances(particleCount());
		boost::transform(boost::irange<size_t>(0, particleCount()), distances.begin(), [direction, positionsArray = ConstDataBufferAccess<Vector3>(positions())](size_t i) {
			return direction.dot(positionsArray[i]);
		});

		// Create index array with all particle indices.
		DataBufferAccessAndRef<int> sortedIndices(new DataBuffer(renderer->dataset(), particleCount(), DataBuffer::Int, 1, 0, false));
		std::iota(sortedIndices.begin(), sortedIndices.end(), 0);

		// Sort particle indices with respect to distance (back-to-front order).
		std::sort(sortedIndices.begin(), sortedIndices.end(), [&distances](GLuint a, GLuint b) {
			return distances[a] < distances[b];
		});

		return sortedIndices.take();
	}
	else {

		// First, compute distance of each particle from the camera along the viewing direction (=camera z-axis).
		std::vector<FloatType> distances(particleCount());
		OVITO_ASSERT(indices()->size() == distances.size());
		boost::transform(ConstDataBufferAccess<int>(indices()), distances.begin(), [direction, positionsArray = ConstDataBufferAccess<Vector3>(positions())](size_t i) {
			return direction.dot(positionsArray[i]);
		});

		std::vector<size_t> mapping(particleCount());
		std::iota(mapping.begin(), mapping.end(), (size_t)0);

		// Sort indices with respect to distance (back-to-front order).
		std::sort(mapping.begin(), mapping.end(), [&](size_t a, size_t b) {
			return distances[a] < distances[b];
		});

		// Create index array with the subset of particles to be rendered.
		DataBufferPtr sortedIndices = new DataBuffer(renderer->dataset(), particleCount(), DataBuffer::Int, 1, 0, false);
		indices()->mappedCopyTo(*sortedIndices, mapping); 

		return sortedIndices;
	}
}

/******************************************************************************
* Render a set of boxes using a glMultiDrawArrays() call.
******************************************************************************/
void OpenGLParticlePrimitive::renderBoxGeometries(OpenGLSceneRenderer* renderer)
{
	// Prepare arrays required for glMultiDrawArrays().
	if(!transparencies() || renderer->isPicking()) {
		if(!indices()) {
			if(_primitiveStartIndices.size() < particleCount() || _primitiveIndicesSource) {
				_primitiveStartIndices.clear();
				_primitiveStartIndices.resize(particleCount());
				_primitiveVertexCounts.clear();
				_primitiveVertexCounts.resize(particleCount());
				GLint index = 0;
				for(GLint& s : _primitiveStartIndices) {
					s = index;
					index += _verticesPerParticle;
				}
				std::fill(_primitiveVertexCounts.begin(), _primitiveVertexCounts.end(), _verticesPerParticle);
			}
		}
		else if(indices() != _primitiveIndicesSource) {
			ConstDataBufferAccess<int> indicesArray(indices());
			_primitiveStartIndices.clear();
			_primitiveStartIndices.resize(particleCount());
			std::transform(indicesArray.begin(), indicesArray.end(), _primitiveStartIndices.begin(), [this](GLuint i) { return i * _verticesPerParticle; });
			if(_primitiveVertexCounts.size() != particleCount()) {
				_primitiveVertexCounts.clear();
				_primitiveVertexCounts.resize(particleCount());
				std::fill(_primitiveVertexCounts.begin(), _primitiveVertexCounts.end(), _verticesPerParticle);
			}
		}
		_primitiveIndicesSource = indices();
	}
	else {
		// When rendering translucent particles, render them in back to front order to avoid visual artifacts at overlapping particles.
		ConstDataBufferAccessAndRef<int> sortedIndices = determineRenderingOrder(renderer);
		_primitiveStartIndices.clear();
		_primitiveStartIndices.resize(particleCount());
		std::transform(sortedIndices.begin(), sortedIndices.end(), _primitiveStartIndices.begin(), [this](GLuint i) { return i * _verticesPerParticle; });
		if(_primitiveVertexCounts.size() != particleCount()) {
			_primitiveVertexCounts.clear();
			_primitiveVertexCounts.resize(particleCount());
			std::fill(_primitiveVertexCounts.begin(), _primitiveVertexCounts.end(), _verticesPerParticle);
		}
	}

	OVITO_CHECK_OPENGL(renderer, 
		renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP,
			_primitiveStartIndices.data(),
			_primitiveVertexCounts.data(),
			particleCount()));	
}

/******************************************************************************
* Renders a set of imposters using triangle geometry.
******************************************************************************/
void OpenGLParticlePrimitive::renderImposterGeometries(OpenGLSceneRenderer* renderer)
{
	if(!transparencies() || renderer->isPicking()) {
		if(!indices()) {
			// Fully opaque particles can be rendered in arbitrary order.
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_TRIANGLES, 0, particleCount() * _verticesPerParticle));
		}
		else {
			if(indices() != _primitiveIndicesSource) {
				_primitiveIndicesSource = indices();
				_indexBuffer.create(QOpenGLBuffer::StaticDraw, _verticesPerParticle * particleCount());
				int* p = _indexBuffer.map();
				for(int idx : ConstDataBufferAccess<int>(indices())) {
					std::iota(p, p + _verticesPerParticle, idx * _verticesPerParticle);
					p += _verticesPerParticle;
				}
				_indexBuffer.unmap();
			}

			// Use indexed mode when rendering only a subset of particles.
			_indexBuffer.oglBuffer().bind();
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, particleCount() * _verticesPerParticle, GL_UNSIGNED_INT, nullptr));
			_indexBuffer.oglBuffer().release();
		}
	}
	else {
		// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
		ConstDataBufferAccessAndRef<int> sortedIndices = determineRenderingOrder(renderer);
		// Create OpenGL index buffer which can be used with glDrawElements.
		_indexBuffer.create(QOpenGLBuffer::StaticDraw, _verticesPerParticle * particleCount());
		int* p = _indexBuffer.map();
		for(int idx : sortedIndices) {
			std::iota(p, p + _verticesPerParticle, idx * _verticesPerParticle);
			p += _verticesPerParticle;
		}
		_indexBuffer.unmap();
		_indexBuffer.oglBuffer().bind();
		OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, particleCount() * _verticesPerParticle, GL_UNSIGNED_INT, nullptr));
		_indexBuffer.oglBuffer().release();
	}
}
#endif

}	// End of namespace
