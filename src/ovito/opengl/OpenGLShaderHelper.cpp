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
#include "OpenGLShaderHelper.h"

namespace Ovito {

/******************************************************************************
* Uploads some data to a new OpenGL buffer object.
******************************************************************************/
QOpenGLBuffer OpenGLShaderHelper::createCachedBufferImpl(GLsizei elementSize, QOpenGLBuffer::Type usage, VertexInputRate inputRate, std::function<void(void*)>&& fillMemoryFunc)
{
	// This method must be called from the main thread.
	OVITO_ASSERT(QThread::currentThread() == QOpenGLContext::currentContext()->thread());

	// Per-element data size must be positive.
	OVITO_ASSERT(elementSize > 0);
    // Drawing counts must have been specified.
	OVITO_ASSERT(verticesPerInstance() > 0);
	OVITO_ASSERT(instanceCount() > 0);

    // Prepare the OpenGL buffer object.
    QOpenGLBuffer bufferObject(usage);
	bufferObject.setUsagePattern(QOpenGLBuffer::StaticDraw);
	if(!bufferObject.create())
		throw Exception(QStringLiteral("Failed to create OpenGL buffer object."));

	if(!bufferObject.bind()) {
		qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLShaderHelper::createCachedBufferImpl()";
		throw Exception(QStringLiteral("Failed to bind OpenGL buffer object."));
	}

    GLsizei bufferSize;
    // Does the OpenGL implementation support instanced arrays (requires OpenGL 3.3+)?
    if(_renderer->glversion() >= QT_VERSION_CHECK(3, 3, 0)) {
        if(inputRate == PerVertex)
        	bufferSize = elementSize * verticesPerInstance();
        else
        	bufferSize = elementSize * instanceCount();
    }
    else {
        // On older GL contexts, we have to emulate instanced arrays by duplicating all vertex data N times.
        bufferSize = elementSize * verticesPerInstance() * instanceCount();
    }

	// Allocate the buffer memory.
    bufferObject.allocate(bufferSize);
	
    // Fill the buffer with data.
    void* p = bufferObject.map(QOpenGLBuffer::WriteOnly);
    if(p == nullptr)
        throw Exception(QStringLiteral("Failed to map memory of newly created OpenGL buffer object of size %1 bytes.").arg(bufferSize));
        
    // Call the user-supplied function that fills the buffer with data to be uploaded to GPU memory.
    std::move(fillMemoryFunc)(p);

    // On older GL contexts, we have to emulate instanced arrays by duplicating all vertex data N times.
    if(_renderer->glversion() < QT_VERSION_CHECK(3, 3, 0)) {
        if(inputRate == PerVertex) {
            if(instanceCount() > 1) {
                size_t chunkSize = elementSize * verticesPerInstance();
                for(int i = 1; i < instanceCount(); i++)
                    std::memcpy(reinterpret_cast<char*>(p) + (i * chunkSize), p, chunkSize);
            }
        }
        else {
            if(verticesPerInstance() > 1) {
                // Note: Doing copies in reverse order to not overwrite input data.
                for(int j = instanceCount() - 1; j >= 0; j--) {
                    const char* src = reinterpret_cast<const char*>(p) + (j * elementSize);
                    char* dst = reinterpret_cast<char*>(p) + (j * elementSize * verticesPerInstance());
                    for(int i = 0; i < verticesPerInstance(); i++, dst += elementSize) {
                        OVITO_ASSERT(dst >= src);
                        std::memcpy(dst, src, elementSize);
                    }
                }
            }
        }
    }

	bufferObject.unmap();
	bufferObject.release();
	OVITO_ASSERT(bufferObject.isCreated());

    return bufferObject;
}

/******************************************************************************
* Uploads the data of an OVITO DataBuffer to an OpenGL buffer object.
******************************************************************************/
QOpenGLBuffer OpenGLShaderHelper::uploadDataBuffer(const ConstDataBufferPtr& dataBuffer, VertexInputRate inputRate, QOpenGLBuffer::Type usage)
{
    OVITO_ASSERT(dataBuffer);

    // Determine the required buffer size.
    GLsizei elementSize;
    if(dataBuffer->dataType() == DataBuffer::Float) {
        if(inputRate == PerVertex) {
            elementSize = dataBuffer->size() * dataBuffer->componentCount() * sizeof(float) / verticesPerInstance();
            OVITO_ASSERT(elementSize * verticesPerInstance() == dataBuffer->size() * dataBuffer->componentCount() * sizeof(float));
        }
        else {
            elementSize = dataBuffer->size() * dataBuffer->componentCount() * sizeof(float) / instanceCount();
            OVITO_ASSERT(elementSize * instanceCount() == dataBuffer->size() * dataBuffer->componentCount() * sizeof(float));
        }
    }
    else {
        OVITO_ASSERT(false);
        dataBuffer->throwException(QStringLiteral("Cannot create OpenGL buffer object for DataBuffer with data type %1.").arg(dataBuffer->dataType()));
    }

    // Create an OpenGL buffer object and fill it with the data from the OVITO DataBuffer object. 
    return createCachedBuffer(dataBuffer, elementSize, usage, inputRate, [&](void* p) {
        if(dataBuffer->dataType() == DataBuffer::Float) {
            // Convert from FloatType to float data type.
            ConstDataBufferAccess<FloatType, true> arrayAccess(dataBuffer);
            size_t srcStride = dataBuffer->componentCount();
            float* dst = static_cast<float*>(p);
            size_t dstStride = dataBuffer->componentCount();
            if(dstStride == srcStride && dataBuffer->stride() == sizeof(FloatType) * srcStride) {
                // Strides are the same for source and destination. Need only a single loop for copying.
                for(const FloatType* src = arrayAccess.cbegin(); src != arrayAccess.cend(); ++src, ++dst)
                    *dst = static_cast<float>(*src);
            }
            else {
                // Strides are the different for source and destination. Need nested loops for copying.
                for(const FloatType* src = arrayAccess.cbegin(); src != arrayAccess.cend(); src += srcStride, dst += dstStride) {
                    for(size_t i = 0; i < srcStride; i++)
                        dst[i] = static_cast<float>(src[i]);
                }
            }
        }
    });
}

/******************************************************************************
* Issues a regular drawing command.
******************************************************************************/
void OpenGLShaderHelper::drawArrays(GLenum mode)
{
    OVITO_ASSERT(verticesPerInstance() > 0);

    // Does the OpenGL context support instanced arrays (requires OpenGL 3.3+)?
    if(_renderer->glversion() >= QT_VERSION_CHECK(3, 3, 0)) {
        if(instanceCount() == 1) {
            // Use native command for non-instanced drawing.
            OVITO_CHECK_OPENGL(_renderer, _renderer->glDrawArrays(mode, 0, verticesPerInstance()));
        }
        else if(instanceCount() > 1) {
            // Use native command for instanced drawing.
            OVITO_CHECK_OPENGL(_renderer, _renderer->glDrawArraysInstanced(mode, 0, verticesPerInstance(), instanceCount()));
        }
    }
    else {
        // Fall-back implementation if glDrawArraysInstanced() or instanced arrays are not available.
        drawArraysOpenGL2(mode);
    }
}

/******************************************************************************
* Issues a drawing command with an ordering of the instances.
******************************************************************************/
void OpenGLShaderHelper::drawArraysOrderedOpenGL4(GLenum mode, QOpenGLBuffer& indirectBuffer, std::function<std::vector<uint32_t>()>&& computeOrderingFunc) 
{
    // On OpenGL 4.3+ contexts, use glMultiDrawArraysIndirect() to render the instances in a prescribed order.
    OVITO_ASSERT(_renderer->glversion() >= QT_VERSION_CHECK(4, 3, 0));
    OVITO_ASSERT(_renderer->glMultiDrawArraysIndirect != nullptr);

    // Data structure used by the glMultiDrawArraysIndirect() command:
    struct DrawArraysIndirectCommand {
        GLuint count;
        GLuint instanceCount;
        GLuint first;
        GLuint baseInstance;
    };

    // Check if this indirect drawing buffer has already been uploaded to the GPU.
    if(!indirectBuffer.isCreated()) {
        indirectBuffer = createCachedBufferImpl(sizeof(DrawArraysIndirectCommand), static_cast<QOpenGLBuffer::Type>(GL_DRAW_INDIRECT_BUFFER), PerInstance, [&](void* buffer) {
            // Call user function to generate the element ordering.
            std::vector<uint32_t> sortedIndices = std::move(computeOrderingFunc)();
            OVITO_ASSERT(sortedIndices.size() == instanceCount());

            // Fill the buffer with DrawArraysIndirectCommand records.
            DrawArraysIndirectCommand* dst = reinterpret_cast<DrawArraysIndirectCommand*>(buffer);
            for(uint32_t index : sortedIndices) {
                dst->count = verticesPerInstance();
                dst->instanceCount = 1;
                dst->first = 0;
                dst->baseInstance = index;
                ++dst;
            }
        });
    }

    // Bind the indirect drawing GL buffer.
    if(!indirectBuffer.bind())
        _renderer->throwException(QStringLiteral("Failed to bind OpenGL indirect drawing buffer for shader '%1'.").arg(shaderObject().objectName()));

    // Draw instances in sorted order.
    OVITO_CHECK_OPENGL(_renderer, _renderer->glMultiDrawArraysIndirect(mode, nullptr, instanceCount(), 0));

    // Done.
    indirectBuffer.release();
}

/******************************************************************************
* Makes the gl_VertexID and gl_InstanceID special variables available in 
* older OpenGL implementations.
******************************************************************************/
void OpenGLShaderHelper::setupVertexAndInstanceIDOpenGL2()
{
    if(_renderer->glversion() < QT_VERSION_CHECK(3, 0, 0)) {
        // In GLSL 1.20, the 'gl_VertexID' and 'gl_InstanceID' special variables are not available.
        // Then we have to emulate them by providing a buffer-backed vertex attribute named 'vertexID' instead.
        
        struct VertexIDCacheKey {};
        std::pair<QOpenGLBuffer, GLsizei>& buf = OpenGLResourceManager::instance()->lookup<std::pair<QOpenGLBuffer, GLsizei>>(RendererResourceKey<VertexIDCacheKey>{}, _renderer->currentResourceFrame());

        if(!buf.first.isCreated() || buf.second < verticesPerInstance() * instanceCount()) {
            buf.second = verticesPerInstance() * instanceCount();
            buf.first = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
            buf.first.setUsagePattern(QOpenGLBuffer::StaticDraw);
            if(!buf.first.create() || !buf.first.bind())
                throw Exception(QStringLiteral("Failed to create OpenGL buffer object."));
            OVITO_CHECK_OPENGL(_renderer, buf.first.allocate(buf.second * sizeof(float)));
            void* p = buf.first.map(QOpenGLBuffer::WriteOnly);
            if(p == nullptr)
                throw Exception(QStringLiteral("Failed to map memory of newly created OpenGL vertexID buffer of size %1 bytes.").arg(buf.second * sizeof(float)));
            std::iota(reinterpret_cast<float*>(p), reinterpret_cast<float*>(p) + buf.second, 0);
            OVITO_CHECK_OPENGL(_renderer, buf.first.unmap());
        }
        else {
            OVITO_CHECK_OPENGL(_renderer, buf.first.bind());
        }
        OVITO_CHECK_OPENGL(_renderer, _shader->setAttributeBuffer("vertexID", GL_FLOAT, 0, 1, 0));
        OVITO_CHECK_OPENGL(_renderer, _shader->enableAttributeArray("vertexID"));
        OVITO_CHECK_OPENGL(_renderer, buf.first.release());
    }

    // This is needed to compute the special shader variable 'gl_VertexID' correctly when instanced arrays are not supported:
	if(_renderer->glversion() < QT_VERSION_CHECK(3, 3, 0)) {
        setUniformValue("vertices_per_instance", verticesPerInstance());
    }
}

/******************************************************************************
* Implemention of the drawArrays() method for OpenGL 2.x.
******************************************************************************/
void OpenGLShaderHelper::drawArraysOpenGL2(GLenum mode)
{
    // Makes the gl_VertexID and gl_InstanceID special variables available in  older OpenGL implementations.
    setupVertexAndInstanceIDOpenGL2();

    if(instanceCount() == 1) {
        // Non-instanced drawing command:
        OVITO_CHECK_OPENGL(_renderer, _renderer->glDrawArrays(mode, 0, verticesPerInstance()));
    }
    else if(instanceCount() > 1) {
        // Use glMultiDrawArrays() if available to draw all instanced in one go.
        if(_renderer->glMultiDrawArrays) {
            RendererResourceKey<OpenGLShaderHelper, GLsizei, GLsizei> indexArrayCacheKey{ instanceCount(), verticesPerInstance() };
            std::pair<std::vector<GLint>, std::vector<GLsizei>>& indexArrays = OpenGLResourceManager::instance()->lookup<std::pair<std::vector<GLint>, std::vector<GLsizei>>>(indexArrayCacheKey, _renderer->currentResourceFrame());
            if(indexArrays.first.empty()) {
                // Fill the two arrays needed for glMultiDrawArrays().
                indexArrays.second.resize(instanceCount(), verticesPerInstance());
                indexArrays.first.resize(instanceCount());
                GLint index = 0;
                for(GLint& f : indexArrays.first)
                    f = (index++) * verticesPerInstance();
            }
            OVITO_ASSERT(indexArrays.first.size() == instanceCount());
            OVITO_ASSERT(indexArrays.second.size() == instanceCount());
            OVITO_ASSERT(indexArrays.second.front() == verticesPerInstance());
            OVITO_CHECK_OPENGL(_renderer, _renderer->glMultiDrawArrays(mode, indexArrays.first.data(), indexArrays.second.data(), instanceCount()));
        }
        else {
            // If glMultiDrawArrays() is not available, fall back to drawing each instance with an individual glDrawArrays() call.
            for(GLsizei i = 0; i < instanceCount(); i++) {
                OVITO_CHECK_OPENGL(_renderer, _renderer->glDrawArrays(mode, i * verticesPerInstance(), verticesPerInstance()));
            }
        }
    }
}

/******************************************************************************
* Issues a drawing command with an ordering of the instances.
******************************************************************************/
void OpenGLShaderHelper::drawArraysOrderedOpenGL2or3(GLenum mode, std::pair<std::vector<GLint>, std::vector<GLsizei>>& indirectBuffers, std::function<std::vector<uint32_t>()>&& computeOrderingFunc) 
{
    // This method is called for OpenGL <4.3, when the glMultiDrawArraysIndirect() function is not available.

    // Check if the indirect drawing buffers have already been filled.
    if(indirectBuffers.first.empty()) {
        // Call user function to generate the element ordering.
        std::vector<uint32_t> sortedIndices = std::move(computeOrderingFunc)();
        OVITO_ASSERT(sortedIndices.size() == instanceCount());

        // Fill the two arrays needed for glMultiDrawArrays().
        indirectBuffers.second.resize(instanceCount(), verticesPerInstance());
        indirectBuffers.first.resize(instanceCount());
        auto index = sortedIndices.cbegin();
        for(GLint& f : indirectBuffers.first)
            f = (*index++) * verticesPerInstance();
    }
    OVITO_ASSERT(indirectBuffers.first.size() == instanceCount());
    OVITO_ASSERT(indirectBuffers.second.size() == instanceCount());
    OVITO_ASSERT(indirectBuffers.second.front() == verticesPerInstance());

    // Makes the gl_VertexID and gl_InstanceID special variables available in  older OpenGL implementations.
    setupVertexAndInstanceIDOpenGL2();

    // On older GL contexts, emulate instanced arrays by duplicating all vertex data N times.
    if(_renderer->glversion() < QT_VERSION_CHECK(3, 3, 0)) {
        // Use glMultiDrawArrays() if available to draw all instanced in one go.
        if(_renderer->glMultiDrawArrays) {
            OVITO_CHECK_OPENGL(_renderer, _renderer->glMultiDrawArrays(mode, indirectBuffers.first.data(), indirectBuffers.second.data(), instanceCount()));
        }
        else {
            // If glMultiDrawArrays() is not available, fall back to drawing each instance with an individual glDrawArrays() call.
            for(GLsizei i = 0; i < instanceCount(); i++) {
                OVITO_CHECK_OPENGL(_renderer, _renderer->glDrawArrays(mode, indirectBuffers.first[i], indirectBuffers.second[i]));
            }
        }
    }
}

}	// End of namespace
