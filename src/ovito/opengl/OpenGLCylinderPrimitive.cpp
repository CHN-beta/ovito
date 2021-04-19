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
#include "OpenGLCylinderPrimitive.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

namespace Ovito {

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLCylinderPrimitive::render(OpenGLSceneRenderer* renderer)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!basePositions() || !headPositions() || basePositions()->size() == 0)
		return;

    // The OpenGL drawing primitive.
    GLenum primitiveDrawMode = GL_TRIANGLE_STRIP;

	// Activate the right OpenGL shader program.
	OpenGLShaderHelper shader(renderer);
    switch(shape()) {
        case CylinderShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->useGeometryShaders()) {
                    if(!renderer->isPicking())
                        shader.load("cylinder", "cylinder/cylinder.vert", "cylinder/cylinder.frag");
                    else
                        shader.load("cylinder_picking", "cylinder/cylinder_picking.vert", "cylinder/cylinder_picking.frag");
                    shader.setVerticesPerInstance(14); // Box rendered as triangle strip.
                }
                else {
                    if(!renderer->isPicking())
                        shader.load("cylinder", "cylinder/cylinder.geom.vert", "cylinder/cylinder.frag", "cylinder/cylinder.geom");
                    else
                        shader.load("cylinder_picking", "cylinder/cylinder_picking.geom.vert", "cylinder/cylinder_picking.frag", "cylinder/cylinder_picking.geom");
                    shader.setVerticesPerInstance(1); // Geometry shader generates the triangle strip from a point primitive.
                }
            }
            else {
                if(!renderer->isPicking())
					shader.load("cylinder_flat", "cylinder/cylinder_flat.vert", "cylinder/cylinder_flat.frag");
                else
					shader.load("cylinder_flat_picking", "cylinder/cylinder_flat_picking.vert", "cylinder/cylinder_flat_picking.frag");
                shader.setVerticesPerInstance(4); // Quad rendered as triangle strip.
            }
            break;

        case ArrowShape:
            if(shadingMode() == NormalShading) {
                if(!renderer->isPicking())
					shader.load("arrow_head", "cylinder/arrow_head.vert", "cylinder/arrow_head.frag");
                else
					shader.load("arrow_head_picking", "cylinder/arrow_head_picking.vert", "cylinder/arrow_head_picking.frag");
                shader.setVerticesPerInstance(14); // Box rendered as triangle strip.
            }
            else {
                if(!renderer->isPicking())
					shader.load("arrow_flat", "cylinder/arrow_flat.vert", "cylinder/arrow_flat.frag");
                else
					shader.load("arrow_flat_picking", "cylinder/arrow_flat_picking.vert", "cylinder/arrow_flat_picking.frag");
                shader.setVerticesPerInstance(7); // 2D arrow rendered as triangle fan.
                primitiveDrawMode = GL_TRIANGLE_FAN;
            }
            break;

        default:
            return;
    }

	shader.setInstanceCount(basePositions()->size());

    // Are we rendering semi-transparent cylinders?
    bool useBlending = !renderer->isPicking() && (transparencies() != nullptr);
	if(useBlending) shader.enableBlending();

	// Pass picking base ID to shader.
    GLint pickingBaseId;
	if(renderer->isPicking()) {
		pickingBaseId = renderer->registerSubObjectIDs(basePositions()->size());
		shader.setPickingBaseId(pickingBaseId);
	}
	OVITO_REPORT_OPENGL_ERRORS(renderer);

    // Pass camera viewing direction (parallel) or camera position (perspective) in object space to vertex shader.  
    if(shadingMode() == FlatShading) {
        Vector3 view_dir_eye_pos;
        if(renderer->projParams().isPerspective)
            view_dir_eye_pos = renderer->modelViewTM().inverse().column(3); // Camera position in object space
        else
            view_dir_eye_pos = renderer->modelViewTM().inverse().column(2); // Camera viewing direction in object space.
        shader.setUniformValue("view_dir_eye_pos", view_dir_eye_pos);
    }

    // Put base/head positions and radii into one combined GL buffer.
    // Radii are optional and may be substituted with a uniform radius value.
    RendererResourceKey<OpenGLCylinderPrimitive, ConstDataBufferPtr, ConstDataBufferPtr, ConstDataBufferPtr, FloatType> positionRadiusCacheKey{
        basePositions(),
        headPositions(),
        radii(),
        radii() ? FloatType(0) : uniformRadius()
    };

    struct BaseHeadRadius {
        Vector_3<float> base;
        Vector_3<float> head;
        float radius;
    };

    // Upload vertex buffer with the base and head positions and radii.
    QOpenGLBuffer positionRadiusBuffer = shader.createCachedBuffer(positionRadiusCacheKey, sizeof(BaseHeadRadius), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
        OVITO_ASSERT(!radii() || radii()->size() == basePositions()->size());
        ConstDataBufferAccess<Point3> basePositionArray(basePositions());
        ConstDataBufferAccess<Point3> headPositionArray(headPositions());
        ConstDataBufferAccess<FloatType> radiusArray(radii());
        float* dst = reinterpret_cast<float*>(buffer);
        const FloatType* radius = radiusArray ? radiusArray.cbegin() : nullptr;
        const Point3* basePos = basePositionArray.cbegin();
        const Point3* headPos = headPositionArray.cbegin();
        for(; basePos != basePositionArray.cend(); ++basePos, ++headPos) {
            *dst++ = static_cast<float>(basePos->x());
            *dst++ = static_cast<float>(basePos->y());
            *dst++ = static_cast<float>(basePos->z());
            *dst++ = static_cast<float>(headPos->x());
            *dst++ = static_cast<float>(headPos->y());
            *dst++ = static_cast<float>(headPos->z());
            *dst++ = static_cast<float>(radius ? *radius++ : uniformRadius());
        }
    });

	// Bind vertex buffer to vertex attributes.
	shader.bindBuffer(positionRadiusBuffer, "base", GL_FLOAT, 3, sizeof(BaseHeadRadius), offsetof(BaseHeadRadius, base), OpenGLShaderHelper::PerInstance);
	shader.bindBuffer(positionRadiusBuffer, "head", GL_FLOAT, 3, sizeof(BaseHeadRadius), offsetof(BaseHeadRadius, head), OpenGLShaderHelper::PerInstance);
	shader.bindBuffer(positionRadiusBuffer, "radius", GL_FLOAT, 1, sizeof(BaseHeadRadius), offsetof(BaseHeadRadius, radius), OpenGLShaderHelper::PerInstance);

    if(!renderer->isPicking()) {

        // Put colors and transparencies into one combined GL buffer with 4 floats per primitive.
        RendererResourceKey<OpenGLCylinderPrimitive, ConstDataBufferPtr, ConstDataBufferPtr, Color> colorCacheKey{ 
            colors(),
            transparencies(),
            colors() ? Color(0,0,0) : uniformColor()
        };

        // Upload vertex buffer with the color data.
        QOpenGLBuffer colorBuffer = shader.createCachedBuffer(colorCacheKey, sizeof(Vector_4<float>), QOpenGLBuffer::VertexBuffer, OpenGLShaderHelper::PerInstance, [&](void* buffer) {
            OVITO_ASSERT(!colors() || colors()->size() == basePositions()->size());
            OVITO_ASSERT(!transparencies() || transparencies()->size() == basePositions()->size());
            const ColorT<float> uniformColor = (ColorT<float>)this->uniformColor();
            ConstDataBufferAccess<FloatType,true> colorArray(colors());
            ConstDataBufferAccess<FloatType> transparencyArray(transparencies());
            const FloatType* color = colorArray ? colorArray.cbegin() : nullptr;
            const FloatType* transparency = transparencyArray ? transparencyArray.cbegin() : nullptr;
            for(float* dst = reinterpret_cast<float*>(buffer), *dst_end = dst + shader.instanceCount() * 4; dst != dst_end;) {
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
        });

        // Bind color vertex buffer.
		shader.bindBuffer(colorBuffer, "color", GL_FLOAT, 4, sizeof(ColorAT<float>), 0, OpenGLShaderHelper::PerInstance);
    }

    // Draw triangle strip or fan instances in regular storage order (not sorted).
    shader.drawArrays(primitiveDrawMode);

    // Draw cylindric part of the arrows.
    if(shape() == ArrowShape && shadingMode() == NormalShading) {
        if(!renderer->isPicking())
            shader.load("arrow_tail", "cylinder/arrow_tail.vert", "cylinder/arrow_tail.frag");
        else {
            shader.load("arrow_tail_picking", "cylinder/arrow_tail_picking.vert", "cylinder/arrow_tail_picking.frag");
    		shader.setPickingBaseId(pickingBaseId);
        }

        shader.drawArrays(GL_TRIANGLE_STRIP);
    }
}

}	// End of namespace
