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
QOpenGLBuffer OpenGLShaderHelper::createCachedBufferImpl(int bufferSize, QOpenGLBuffer::Type usage, std::function<void(void*)>&& fillMemoryFunc)
{
	// This method must be called from the main thread.
	OVITO_ASSERT(QThread::currentThread() == QOpenGLContext::currentContext()->thread());
	// Buffer size must be positive.
	OVITO_ASSERT(bufferSize > 0);

    // Prepare the OpenGL buffer object.
    QOpenGLBuffer bufferObject(usage);
	bufferObject.setUsagePattern(QOpenGLBuffer::StaticDraw/*QOpenGLBuffer::StreamDraw*/);
	if(!bufferObject.create())
		throw Exception(QStringLiteral("Failed to create OpenGL buffer object."));

	if(!bufferObject.bind()) {
		qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLShaderHelper::createCachedBufferImpl()";
		throw Exception(QStringLiteral("Failed to bind OpenGL buffer object."));
	}

	// Allocate the buffer memory.
	bufferObject.allocate(bufferSize);
	
    // Fill the buffer with data.
    void* p = bufferObject.map(QOpenGLBuffer::WriteOnly);
    if(p == nullptr)
        throw Exception(QStringLiteral("Failed to map memory of newly created OpenGL buffer object of size %1 bytes.").arg(bufferSize));
    // Call the user-supplied function that fills the buffer with data to be uploaded to GPU memory.
    std::move(fillMemoryFunc)(p);
	bufferObject.unmap();
	bufferObject.release();
	OVITO_ASSERT(bufferObject.isCreated());

    return bufferObject;
}

/******************************************************************************
* Uploads the data of an OVITO DataBuffer to an OpenGL buffer object.
******************************************************************************/
QOpenGLBuffer OpenGLShaderHelper::uploadDataBuffer(const ConstDataBufferPtr& dataBuffer, OpenGLResourceManager::ResourceFrameHandle resourceFrame, QOpenGLBuffer::Type usage)
{
    OVITO_ASSERT(dataBuffer);

    // Determine the required buffer size.
    int bufferSize;
    if(dataBuffer->dataType() == DataBuffer::Float) {
        bufferSize = dataBuffer->size() * dataBuffer->componentCount() * sizeof(float);
    }
    else {
        OVITO_ASSERT(false);
        dataBuffer->throwException(QStringLiteral("Cannot create OpenGL buffer object for DataBuffer with data type %1.").arg(dataBuffer->dataType()));
    }

    // Create an OpenGL buffer object and fill it with the data from the OVITO DataBuffer object. 
    return createCachedBuffer(dataBuffer, bufferSize, resourceFrame, usage, [&](void* p) {
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

}	// End of namespace
