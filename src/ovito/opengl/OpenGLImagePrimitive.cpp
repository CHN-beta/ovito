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
#include "OpenGLImagePrimitive.h"
#include "OpenGLSceneRenderer.h"
#include "OpenGLShaderHelper.h"

namespace Ovito {

/******************************************************************************
* Renders the image in a rectangle given in device pixel coordinates.
******************************************************************************/
void OpenGLImagePrimitive::render(OpenGLSceneRenderer* renderer)
{
	OVITO_REPORT_OPENGL_ERRORS(renderer);

	if(image().isNull() || renderer->isPicking() || windowRect().isEmpty())
		return;

    // Temporarily disable depth testing.
    bool wasDepthTestEnabled = renderer->glIsEnabled(GL_DEPTH_TEST);
    OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_DEPTH_TEST));

    // Activate the OpenGL shader program.
    OpenGLShaderHelper shader(renderer);
    shader.load("image", "image/image.vert", "image/image.frag");

    shader.setVerticesPerInstance(4);
    shader.setInstanceCount(1);

    // Turn the image into an OpenGL texture.
    QOpenGLTexture* texture = OpenGLResourceManager::instance()->uploadImage(image(), renderer->currentResourceFrame());
    texture->bind();

    // Transform rectangle to normalized device coordinates.
    Box2 b = windowRect();
    int aaLevel = renderer->antialiasingLevel();
    if(aaLevel > 1) {
        b.minc.x() = (int)(b.minc.x() / aaLevel) * aaLevel;
        b.minc.y() = (int)(b.minc.y() / aaLevel) * aaLevel;
        b.maxc.x() = (int)(b.maxc.x() / aaLevel) * aaLevel;
        b.maxc.y() = (int)(b.maxc.y() / aaLevel) * aaLevel;
    }
    const QRect& vpRect = renderer->viewportRect();
    Vector4 image_rect(
        b.minc.x() / vpRect.width() * 2.0 - 1.0, 
        1.0 - b.maxc.y() / vpRect.height() * 2.0,
        b.maxc.x() / vpRect.width() * 2.0 - 1.0, 
        1.0 - b.minc.y() / vpRect.height() * 2.0);
        
    // Pass the image rectangle to the shader as a uniform.
    shader.setUniformValue("image_rect", image_rect);

    // Temporarily enable alpha blending.
    shader.enableBlending();

    // Draw a quad with 4 vertices.
    shader.drawArrays(GL_TRIANGLE_STRIP);

    // Release the texture.
    texture->release();

    // Restore old context state.
    if(wasDepthTestEnabled) 
        renderer->glEnable(GL_DEPTH_TEST);
}

}	// End of namespace
