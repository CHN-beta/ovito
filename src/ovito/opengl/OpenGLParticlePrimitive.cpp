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

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
    switch(particleShape()) {
        case SquareCubicShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->useGeometryShaders()) {
                    if(!renderer->isPicking())
                        shader.load("cube", "particles/cube/cube.vert", "particles/cube/cube.frag");
                    else
                        shader.load("cube_picking", "particles/cube/cube_picking.vert", "particles/cube/cube_picking.frag");
                    shader.setVerticesPerInstance(14); // Cube rendered as triangle strip.
                }
                else {
                    if(!renderer->isPicking())
                        shader.load("cube", "particles/cube/cube.geom.vert", "particles/cube/cube.frag", "particles/cube/cube.geom");
                    else
                        shader.load("cube_picking", "particles/cube/cube_picking.geom.vert", "particles/cube/cube_picking.frag", "particles/cube/cube_picking.geom");
                    shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                }
            }
            else {
                if(!renderer->useGeometryShaders()) {
                    if(!renderer->isPicking()) 
                        shader.load("square", "particles/square/square.vert", "particles/square/square.frag");
                    else
                        shader.load("square_picking", "particles/square/square_picking.vert", "particles/square/square_picking.frag");
                    shader.setVerticesPerInstance(4); // Square rendered as triangle strip.
                }
                else {
                    if(!renderer->isPicking()) 
                        shader.load("square", "particles/square/square.geom.vert", "particles/square/square.frag", "particles/square/square.geom");
                    else
                        shader.load("square_picking", "particles/square/square_picking.geom.vert", "particles/square/square_picking.frag", "particles/square/square_picking.geom");
                    shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                }
            }
            break;
        case BoxShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->useGeometryShaders()) {
                    if(!renderer->isPicking())
                        shader.load("box", "particles/box/box.vert", "particles/box/box.frag");
                    else
                        shader.load("box_picking", "particles/box/box_picking.vert", "particles/box/box_picking.frag");
                    shader.setVerticesPerInstance(14); // Box rendered as triangle strip.
                }
                else {
                    if(!renderer->isPicking())
                        shader.load("box", "particles/box/box.geom.vert", "particles/box/box.frag", "particles/box/box.geom");
                    else
                        shader.load("box_picking", "particles/box/box_picking.geom.vert", "particles/box/box_picking.frag", "particles/box/box_picking.geom");
                    shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                }
            }
            else return;
            break;
        case SphericalShape:
            if(shadingMode() == NormalShading) {
                if(renderingQuality() >= HighQuality) {
                    if(!renderer->useGeometryShaders()) {
                        if(!renderer->isPicking())
                            shader.load("sphere", "particles/sphere/sphere.vert", "particles/sphere/sphere.frag");
                        else
                            shader.load("sphere_picking", "particles/sphere/sphere_picking.vert", "particles/sphere/sphere_picking.frag");
                        shader.setVerticesPerInstance(14); // Cube rendered as triangle strip.
                    }
                    else {
                        if(!renderer->isPicking())
                            shader.load("sphere", "particles/sphere/sphere.geom.vert", "particles/sphere/sphere.frag", "particles/sphere/sphere.geom");
                        else
                            shader.load("sphere_picking", "particles/sphere/sphere_picking.geom.vert", "particles/sphere/sphere_picking.frag", "particles/sphere/sphere_picking.geom");
                        shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                    }
                }
                else {
                    if(!renderer->useGeometryShaders()) {
                        if(!renderer->isPicking())
                            shader.load("imposter", "particles/imposter/imposter.vert", "particles/imposter/imposter.frag");
                        else
                            shader.load("imposter_picking", "particles/imposter/imposter_picking.vert", "particles/imposter/imposter_picking.frag");
                        shader.setVerticesPerInstance(4); // Square rendered as triangle strip.
                    }
                    else {
                        if(!renderer->isPicking())
                            shader.load("imposter", "particles/imposter/imposter.geom.vert", "particles/imposter/imposter.frag", "particles/imposter/imposter.geom");
                        else
                            shader.load("imposter_picking", "particles/imposter/imposter_picking.geom.vert", "particles/imposter/imposter_picking.frag", "particles/imposter/imposter_picking.geom");
                        shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                    }
                }
            }
            else {
                if(!renderer->useGeometryShaders()) {
                    if(!renderer->isPicking())
                        shader.load("circle", "particles/circle/circle.vert", "particles/circle/circle.frag");
                    else
                        shader.load("circle_picking", "particles/circle/circle_picking.vert", "particles/circle/circle_picking.frag");
                    shader.setVerticesPerInstance(4); // Square rendered as triangle strip.
                }
                else {
                    if(!renderer->isPicking())
                        shader.load("circle", "particles/circle/circle.geom.vert", "particles/circle/circle.frag", "particles/circle/circle.geom");
                    else
                        shader.load("circle_picking", "particles/circle/circle_picking.geom.vert", "particles/circle/circle_picking.frag", "particles/circle/circle_picking.geom");
                    shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                }
            }
            break;
        case EllipsoidShape:
            if(!renderer->useGeometryShaders()) {
                if(!renderer->isPicking())
                    shader.load("ellipsoid", "particles/ellipsoid/ellipsoid.vert", "particles/ellipsoid/ellipsoid.frag");
                else
                    shader.load("ellipsoid_picking", "particles/ellipsoid/ellipsoid_picking.vert", "particles/ellipsoid/ellipsoid_picking.frag");
                shader.setVerticesPerInstance(14); // Box rendered as triangle strip.
            }
            else {
                if(!renderer->isPicking())
                    shader.load("ellipsoid", "particles/ellipsoid/ellipsoid.geom.vert", "particles/ellipsoid/ellipsoid.frag", "particles/ellipsoid/ellipsoid.geom");
                else
                    shader.load("ellipsoid_picking", "particles/ellipsoid/ellipsoid_picking.geom.vert", "particles/ellipsoid/ellipsoid_picking.frag", "particles/ellipsoid/ellipsoid_picking.geom");
                shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
            }
            break;
        case SuperquadricShape:
            if(!renderer->useGeometryShaders()) {
                if(!renderer->isPicking())
                    shader.load("superquadric", "particles/superquadric/superquadric.vert", "particles/superquadric/superquadric.frag");
                else
                    shader.load("superquadric_picking", "particles/superquadric/superquadric_picking.vert", "particles/superquadric/superquadric_picking.frag");
                shader.setVerticesPerInstance(14); // Box rendered as triangle strip.
            }
            else {
                if(!renderer->isPicking())
                    shader.load("superquadric", "particles/superquadric/superquadric.geom.vert", "particles/superquadric/superquadric.frag", "particles/superquadric/superquadric.geom");
                else
                    shader.load("superquadric_picking", "particles/superquadric/superquadric_picking.geom.vert", "particles/superquadric/superquadric_picking.frag", "particles/superquadric/superquadric_picking.geom");
                shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
            }
            break;
        default:
            return;
    }

    // The effective number of particles being rendered:
	shader.setInstanceCount(indices() ? indices()->size() : positions()->size());

    // Are we rendering semi-transparent particles?
    bool useBlending = !renderer->isPicking() && (transparencies() != nullptr);
	if(useBlending) shader.enableBlending();

	// Pass picking base ID to shader.
	if(renderer->isPicking()) {
		shader.setPickingBaseId(renderer->registerSubObjectIDs(positions()->size(), indices()));
	}
	OVITO_REPORT_OPENGL_ERRORS(renderer);

    // Put positions and radii into one combined Vulkan buffer with 4 floats per particle.
    // Radii are optional and may be substituted with a uniform radius value.
    RendererResourceKey<OpenGLParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> positionRadiusCacheKey{
        indices(),
        positions(),
        radii(),
        radii() ? FloatType(0) : uniformRadius()
    };

    // Upload vertex buffer with the particle positions and radii.
    QOpenGLBuffer positionRadiusBuffer = shader.createCachedBuffer(positionRadiusCacheKey, sizeof(Vector_4<float>), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
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

	// Radius attribute is only required for certain particle shapes.
    if(particleShape() != BoxShape && particleShape() != EllipsoidShape && particleShape() != SuperquadricShape) {
		shader.bindBuffer(positionRadiusBuffer, "radius", GL_FLOAT, 1, sizeof(Vector_4<float>), sizeof(Vector_3<float>), OpenGLShaderHelper::PerInstance);
	}

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
        QOpenGLBuffer colorSelectionBuffer = shader.createCachedBuffer(colorSelectionCacheKey, sizeof(ColorAT<float>), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
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

    // For box-shaped and ellipsoid particles, we need the shape/orientation vertex attribute.
    if(particleShape() == BoxShape || particleShape() == EllipsoidShape || particleShape() == SuperquadricShape) {

        // Combine aspherical shape property and orientation property into one combined buffer containing a 4x4 transformation matrix per particle.
        RendererResourceKey<OpenGLParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> shapeOrientationCacheKey{ 
            indices(),
            asphericalShapes(),
            orientations(),
            radii(),
            radii() ? FloatType(0) : uniformRadius()
        };

        // Upload vertex buffer with the particle transformation matrices.
        QOpenGLBuffer shapeOrientationBuffer = shader.createCachedBuffer(shapeOrientationCacheKey, sizeof(Matrix_4<float>), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
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
		GLuint attrIndex = shader.shaderObject().attributeLocation("shape_orientation");
		for(int i = 0; i < 4; i++)
			shader.bindBuffer(shapeOrientationBuffer, attrIndex + i, GL_FLOAT, 4, sizeof(Matrix_4<float>), i * sizeof(Vector_4<float>), OpenGLShaderHelper::PerInstance);
	}

    // For superquadric particles, we need to prepare the roundness vertex attribute.
    if(particleShape() == SuperquadricShape) {

        RendererResourceKey<OpenGLParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr> roundnessCacheKey{ 
            indices(),
            roundness()
        };

        // Upload vertex buffer with the roundness values.
        QOpenGLBuffer roundnessBuffer = shader.createCachedBuffer(roundnessCacheKey, sizeof(Vector_2<float>), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
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
                std::fill(dst, dst + shader.instanceCount(), Vector_2<float>(1,1));
            }
        });

        // Bind vertex buffer.
		shader.bindBuffer(roundnessBuffer, "roundness", GL_FLOAT, 2, sizeof(Vector_2<float>), 0, OpenGLShaderHelper::PerInstance);
    }

    if(!useBlending) {
        // Draw triangle strip instances in regular storage order (not sorted).
		shader.drawArrays(GL_TRIANGLE_STRIP);
    }
    else {
        // Render the particles in back-to-front order. 
        OVITO_ASSERT(!renderer->isPicking());

        // Viewing direction in object space:
        const Vector3 direction = renderer->modelViewTM().inverse().column(2);

        // The caching key for the particle ordering.
        RendererResourceKey<OpenGLParticlePrimitive, ConstDataBufferPtr, ConstDataBufferPtr, Vector3, int> orderingCacheKey{
            indices(),
            positions(),
            direction,
            shader.verticesPerInstance()
        };

        // Render primitives.
        shader.drawArraysOrdered(GL_TRIANGLE_STRIP, orderingCacheKey, [&]() {

            // First, compute distance of each particle from the camera along the viewing direction (=camera z-axis).
            std::vector<FloatType> distances(shader.instanceCount());
            if(!indices()) {
                boost::transform(boost::irange<size_t>(0, shader.instanceCount()), distances.begin(), [direction, positionsArray = ConstDataBufferAccess<Vector3>(positions())](size_t i) {
                    return direction.dot(positionsArray[i]);
                });
            }
            else {
                boost::transform(ConstDataBufferAccess<int>(indices()), distances.begin(), [direction, positionsArray = ConstDataBufferAccess<Vector3>(positions())](size_t i) {
                    return direction.dot(positionsArray[i]);
                });
            }

            // Create index array with all particle indices.
            std::vector<uint32_t> sortedIndices(shader.instanceCount());
            std::iota(sortedIndices.begin(), sortedIndices.end(), (uint32_t)0);

            // Sort particle indices with respect to distance (back-to-front order).
            std::sort(sortedIndices.begin(), sortedIndices.end(), [&](uint32_t a, uint32_t b) {
                return distances[a] < distances[b];
            });

            return sortedIndices;
        });
	}
}

}	// End of namespace
