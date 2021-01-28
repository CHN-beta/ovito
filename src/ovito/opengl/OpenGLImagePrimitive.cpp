////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <QOpenGLPaintDevice> 

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLImagePrimitive::OpenGLImagePrimitive(OpenGLSceneRenderer* renderer)
{
	QString prefix = renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles") : QStringLiteral(":/openglrenderer");

    if(!renderer->glcontext()->isOpenGLES() || renderer->glformat().majorVersion() >= 3) {

        // Initialize OpenGL shader.
        _shader = renderer->loadShaderProgram("image", prefix + "/glsl/image/image.vs", prefix + "/glsl/image/image.fs");

        // Create vertex buffer.
        if(!_vertexBuffer.create(QOpenGLBuffer::StaticDraw, 4))
            renderer->throwException(QStringLiteral("Failed to create OpenGL vertex buffer."));

        // Create OpenGL texture.
        _texture.create();
    }
}

/******************************************************************************
* Renders the image in a rectangle given in device pixel coordinates.
******************************************************************************/
void OpenGLImagePrimitive::render(OpenGLSceneRenderer* renderer)
{
	if(image().isNull() || renderer->isPicking() || windowRect().isEmpty())
		return;

    if(_texture.isCreated()) {
        OVITO_ASSERT(_texture.isCreated());
        OVITO_CHECK_OPENGL(renderer, renderer->rebindVAO());

        // Prepare texture.
        OVITO_CHECK_OPENGL(renderer, _texture.bind());

        qint64 cacheKey = image().cacheKey();
        if(cacheKey != _imageCacheKey) {
            _imageCacheKey = cacheKey;

            OVITO_REPORT_OPENGL_ERRORS(renderer);
            OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
#ifndef Q_OS_WASM
            OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));
            OVITO_CHECK_OPENGL(renderer, renderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
#endif

            // Upload texture data.
            QImage textureImage = convertToGLFormat(image());
            OVITO_CHECK_OPENGL(renderer, renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureImage.width(), textureImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage.constBits()));
        }

        // Transform rectangle to normalized device coordinates.
        Box2 b = windowRect();
        int aaLevel = renderer->antialiasingLevelInternal();
        if(aaLevel > 1) {
            b.minc.x() = (int)(b.minc.x() / aaLevel) * aaLevel;
            b.minc.y() = (int)(b.minc.y() / aaLevel) * aaLevel;
            b.maxc.x() = (int)(b.maxc.x() / aaLevel) * aaLevel;
            b.maxc.y() = (int)(b.maxc.y() / aaLevel) * aaLevel;
        }
        GLint vc[4];
        renderer->glGetIntegerv(GL_VIEWPORT, vc);
        Point_3<GLfloat>* vertices = _vertexBuffer.map();
        vertices[0] = Point_3<GLfloat>(b.minc.x() / vc[2] * 2 - 1, 1 - b.maxc.y() / vc[3] * 2, 0);
        vertices[1] = Point_3<GLfloat>(b.maxc.x() / vc[2] * 2 - 1, 1 - b.maxc.y() / vc[3] * 2, 1);
        vertices[2] = Point_3<GLfloat>(b.minc.x() / vc[2] * 2 - 1, 1 - b.minc.y() / vc[3] * 2, 2);
        vertices[3] = Point_3<GLfloat>(b.maxc.x() / vc[2] * 2 - 1, 1 - b.minc.y() / vc[3] * 2, 3);
        _vertexBuffer.unmap();

        bool wasDepthTestEnabled = renderer->glIsEnabled(GL_DEPTH_TEST);
        bool wasBlendEnabled = renderer->glIsEnabled(GL_BLEND);
        OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_DEPTH_TEST));
        OVITO_CHECK_OPENGL(renderer, renderer->glEnable(GL_BLEND));
        OVITO_CHECK_OPENGL(renderer, renderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        if(!_shader->bind())
            renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

        // Set up look-up table for texture coordinates.
        static const QVector2D uvcoords[] = {{0,0}, {1,0}, {0,1}, {1,1}};
        _shader->setUniformValueArray("uvcoords", uvcoords, 4);

        _vertexBuffer.bindPositions(renderer, _shader);
        OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        _vertexBuffer.detachPositions(renderer, _shader);

        _shader->release();

        // Restore old state.
        if(wasDepthTestEnabled) renderer->glEnable(GL_DEPTH_TEST);
        if(!wasBlendEnabled) renderer->glDisable(GL_BLEND);
    }
    else {
        // Disable depth testing and blending.
        bool wasDepthTestEnabled = renderer->glIsEnabled(GL_DEPTH_TEST);
        OVITO_CHECK_OPENGL(renderer, renderer->glDisable(GL_DEPTH_TEST));

        // Query the viewport size in device pixels.
        GLint vc[4];
        renderer->glGetIntegerv(GL_VIEWPORT, vc);

        // Use Qt's QOpenGLPaintDevice to paint the image into the framebuffer.
        QOpenGLPaintDevice paintDevice(vc[2], vc[3]);
        QPainter painter(&paintDevice);

        QRectF rect(windowRect().minc.x(), windowRect().minc.y(), windowRect().width(), windowRect().height());
        OVITO_CHECK_OPENGL(renderer, painter.drawImage(rect, image()));

        // Restore old state.
        if(wasDepthTestEnabled)
            renderer->glEnable(GL_DEPTH_TEST);
    }
}

static inline QRgb qt_gl_convertToGLFormatHelper(QRgb src_pixel, GLenum texture_format)
{
    if(texture_format == 0x80E1 /*GL_BGRA*/) {
        if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            return ((src_pixel << 24) & 0xff000000)
                   | ((src_pixel >> 24) & 0x000000ff)
                   | ((src_pixel << 8) & 0x00ff0000)
                   | ((src_pixel >> 8) & 0x0000ff00);
        }
        else {
            return src_pixel;
        }
    }
    else {  // GL_RGBA
        if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            return (src_pixel << 8) | ((src_pixel >> 24) & 0xff);
        }
        else {
            return ((src_pixel << 16) & 0xff0000)
                   | ((src_pixel >> 16) & 0xff)
                   | (src_pixel & 0xff00ff00);
        }
    }
}

static void convertToGLFormatHelper(QImage &dst, const QImage &img, GLenum texture_format)
{
    OVITO_ASSERT(dst.depth() == 32);
    OVITO_ASSERT(img.depth() == 32);

    if(dst.size() != img.size()) {
        int target_width = dst.width();
        int target_height = dst.height();
        qreal sx = target_width / qreal(img.width());
        qreal sy = target_height / qreal(img.height());

        quint32 *dest = (quint32 *) dst.scanLine(0); // NB! avoid detach here
        uchar *srcPixels = (uchar *) img.scanLine(img.height() - 1);
        int sbpl = img.bytesPerLine();
        int dbpl = dst.bytesPerLine();

        int ix = int(0x00010000 / sx);
        int iy = int(0x00010000 / sy);

        quint32 basex = int(0.5 * ix);
        quint32 srcy = int(0.5 * iy);

        // scale, swizzle and mirror in one loop
        while (target_height--) {
            const uint *src = (const quint32 *) (srcPixels - (srcy >> 16) * sbpl);
            int srcx = basex;
            for (int x=0; x<target_width; ++x) {
                dest[x] = qt_gl_convertToGLFormatHelper(src[srcx >> 16], texture_format);
                srcx += ix;
            }
            dest = (quint32 *)(((uchar *) dest) + dbpl);
            srcy += iy;
        }
    }
    else {
        const int width = img.width();
        const int height = img.height();
        const uint *p = (const uint*) img.scanLine(img.height() - 1);
        uint *q = (uint*) dst.scanLine(0);

        if(texture_format == 0x80E1 /*GL_BGRA*/) {
            if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                // mirror + swizzle
                for(int i=0; i < height; ++i) {
                    const uint *end = p + width;
                    while(p < end) {
                        *q = ((*p << 24) & 0xff000000)
                             | ((*p >> 24) & 0x000000ff)
                             | ((*p << 8) & 0x00ff0000)
                             | ((*p >> 8) & 0x0000ff00);
                        p++;
                        q++;
                    }
                    p -= 2 * width;
                }
            }
            else {
                const uint bytesPerLine = img.bytesPerLine();
                for (int i=0; i < height; ++i) {
                    memcpy(q, p, bytesPerLine);
                    q += width;
                    p -= width;
                }
            }
        }
        else {
            if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                for(int i=0; i < height; ++i) {
                    const uint *end = p + width;
                    while(p < end) {
                        *q = (*p << 8) | ((*p >> 24) & 0xff);
                        p++;
                        q++;
                    }
                    p -= 2 * width;
                }
            }
            else {
                for(int i=0; i < height; ++i) {
                    const uint *end = p + width;
                    while(p < end) {
                        *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) | (*p & 0xff00ff00);
                        p++;
                        q++;
                    }
                    p -= 2 * width;
                }
            }
        }
    }
}

QImage OpenGLImagePrimitive::convertToGLFormat(const QImage& img)
{
    QImage res(img.size(), QImage::Format_ARGB32);
    convertToGLFormatHelper(res, img.convertToFormat(QImage::Format_ARGB32), GL_RGBA);
    return res;
}

}	// End of namespace
