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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include "OpenGLHelpers.h"
#include "OpenGLSceneRenderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <qopengl.h>

namespace Ovito {

/**
 * \brief A wrapper for the QOpenGLBuffer class, which adds more features.
 */
template<typename T>
class OpenGLBuffer
{
public:

	typedef T value_type;

	/// Constructor.
	OpenGLBuffer(QOpenGLBuffer::Type type = QOpenGLBuffer::VertexBuffer) : _elementCount(0), _verticesPerElement(0), _buffer(type) {}

	/// Transfers a local OVITO data buffer into the OpenGL VBO.
	template<typename U>
	void uploadData(const ConstDataBufferPtr& sourceBuffer, int verticesPerElement = 1) {
		if(_sourceBuffer == sourceBuffer && isCreated() && _verticesPerElement == verticesPerElement) {
			OVITO_ASSERT(!sourceBuffer || sourceBuffer->size() == _elementCount);
			return;
		}
		_sourceBuffer = sourceBuffer;
		if(_sourceBuffer) {
			create(QOpenGLBuffer::StaticDraw, _sourceBuffer->size(), verticesPerElement);
			fill(ConstDataBufferAccess<U>(_sourceBuffer).cbegin());
		}
		else {
			destroy();
		}
	}

	/// Creates the buffer object in the OpenGL server. This function must be called with a current QOpenGLContext.
	/// The buffer will be bound to and can only be used in that context (or any other context that is shared with it).
	bool create(QOpenGLBuffer::UsagePattern usagePattern, int elementCount, int verticesPerElement = 1) {
		OVITO_ASSERT(verticesPerElement >= 1);
		OVITO_ASSERT(elementCount >= 0);
		OVITO_ASSERT(elementCount < std::numeric_limits<int>::max() / sizeof(T) / verticesPerElement);
		if(_elementCount != elementCount || _verticesPerElement != verticesPerElement) {
			_elementCount = elementCount;
			_verticesPerElement = verticesPerElement;
			if(!_buffer.isCreated()) {
				_isOpenGLES = QOpenGLContext::currentContext()->isOpenGLES();
				if(!_buffer.create())
					throw Exception(QStringLiteral("Failed to create OpenGL vertex buffer."));
				_buffer.setUsagePattern(usagePattern);
			}
			if(!_buffer.bind()) {
				qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLBuffer::create()";
				qWarning() << "Parameters: usagePattern =" << usagePattern << "elementCount =" << elementCount << "verticesPerElement =" << verticesPerElement;
				throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
			}
			_buffer.allocate(sizeof(T) * _elementCount * _verticesPerElement);
			_buffer.release();
			return true;
		}
		else {
			OVITO_ASSERT(isCreated());
			return false;
		}
	}

	/// Returns true if this buffer has been created; false otherwise.
	bool isCreated() const { return _buffer.isCreated(); }

	/// Returns the number of elements stored in this buffer.
	int elementCount() const { return _elementCount; }

	/// Returns the number of vertices rendered per element.
	int verticesPerElement() const { return _verticesPerElement; }

	/// Provides access to the internal OpenGL vertex buffer object.
	QOpenGLBuffer& oglBuffer() { return _buffer; }

	/// Destroys this buffer object, including the storage being used in the OpenGL server.
	void destroy() {
		_buffer.destroy();
		_elementCount = 0;
		_verticesPerElement = 0;
	}

	/// Maps the contents of this buffer into the application's memory space and returns a pointer to it.
	T* map(QOpenGLBuffer::Access access = QOpenGLBuffer::WriteOnly) {
		OVITO_ASSERT(isCreated());
		if(elementCount() == 0)
			return nullptr;
		if(!_isOpenGLES) {
			if(!_buffer.bind()) {
				qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLBuffer::map()";
				qWarning() << "Parameters: access =" << access << "elementCount =" << _elementCount << "verticesPerElement =" << _verticesPerElement;
				throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
			}
			T* data = static_cast<T*>(_buffer.map(access));
			if(!data)
				throw Exception(QStringLiteral("Failed to map OpenGL vertex buffer to memory."));
			return data;
		}
		else {
			// WebGL 1/OpenGL ES 2.0 does not support mapping a GL buffer to memory.
			// Need to emulate the map() method by providing a temporary memory buffer on the host. 
			OVITO_ASSERT(access == QOpenGLBuffer::WriteOnly);
			_temporaryBuffer.resize(elementCount() * verticesPerElement());
			return _temporaryBuffer.data();
		}
	}

	/// Unmaps the buffer after it was mapped into the application's memory space with a previous call to map().
	void unmap() {
		if(elementCount() == 0)
			return;
		if(!_isOpenGLES) {
			if(!_buffer.unmap())
				throw Exception(QStringLiteral("Failed to unmap OpenGL vertex buffer from memory."));
			_buffer.release();
		}
		else {
			// Upload the data in the temporary memory buffer to graphics memory.
			if(!_buffer.bind()) {
				qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLBuffer::unmap()";
				qWarning() << "Parameters: elementCount =" << _elementCount << "verticesPerElement =" << _verticesPerElement;
				throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
			}
			OVITO_ASSERT(_temporaryBuffer.size() == _elementCount * _verticesPerElement);
			_buffer.write(0, _temporaryBuffer.data(), sizeof(T) * _elementCount * _verticesPerElement);
			_buffer.release();
			// Free temporary buffer.
			decltype(_temporaryBuffer)().swap(_temporaryBuffer);
		}
	}

	/// Fills the vertex buffer with the given data.
	template<typename U>
	void fill(const U* data) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(_elementCount >= 0);
		OVITO_ASSERT(_verticesPerElement >= 1);

		if(_verticesPerElement == 1 && std::is_same<T,U>::value) {
			if(!_buffer.bind()) {
				qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLBuffer::fill()";
				qWarning() << "Parameters: elementCount =" << _elementCount << "verticesPerElement =" << _verticesPerElement;
				throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
			}
			_buffer.write(0, data, _elementCount * sizeof(T));
			_buffer.release();
		}
		else {
			T* bufferData = map(QOpenGLBuffer::WriteOnly);
			const U* endData = data + _elementCount;
			for(; data != endData; ++data) {
				for(int i = 0; i < _verticesPerElement; i++) {
					*bufferData++ = static_cast<T>(*data);
				}
			}
			unmap();
		}
	}

	/// Fills the buffer with a constant value.
	template<typename U>
	void fillConstant(U value) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(_elementCount >= 0);
		OVITO_ASSERT(_verticesPerElement >= 1);

		if(_elementCount) {
			T* bufferData = map(QOpenGLBuffer::WriteOnly);
			std::fill(bufferData, bufferData + _elementCount * _verticesPerElement, (T)value);
			unmap();
		}
	}

	/// Binds this buffer to a vertex attribute of a vertex shader.
	void bind(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, const char* attributeName, GLenum type, int offset, int tupleSize, int stride = 0) {
		OVITO_ASSERT(isCreated());
		OVITO_ASSERT(type != GL_FLOAT || (sizeof(T) == sizeof(GLfloat)*tupleSize && stride == 0) || sizeof(T) == stride);
		OVITO_ASSERT(type != GL_INT || (sizeof(T) == sizeof(GLint)*tupleSize && stride == 0) || sizeof(T) == stride);
		if(!_buffer.bind()) {
			qWarning() << "QOpenGLBuffer::bind() failed in function OpenGLBuffer::bind()";
			qWarning() << "Parameters: attributeName =" << attributeName << "elementCount =" << _elementCount << "verticesPerElement =" << _verticesPerElement << "type =" << type << "offset =" << offset << "tupleSize =" << tupleSize << "stride =" << stride;
			throw Exception(QStringLiteral("Failed to bind OpenGL vertex buffer."));
		}
		if(stride == 0) stride = sizeof(T);
		OVITO_CHECK_OPENGL(renderer, shader->enableAttributeArray(attributeName));
		OVITO_CHECK_OPENGL(renderer, shader->setAttributeBuffer(attributeName, type, offset, tupleSize, stride));
		_buffer.release();
	}

	/// After rendering is done, release the binding of the buffer to a shader attribute.
	void detach(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, const char* attributeName) {
		OVITO_CHECK_OPENGL(renderer, shader->disableAttributeArray(attributeName));
	}

	/// Binds this buffer to the vertex position attribute of a vertex shader.
	void bindPositions(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, size_t byteOffset = 0) {
		bind(renderer, shader, "position", GL_FLOAT, byteOffset, 3, sizeof(T));
	}

	/// After rendering is done, release the binding of the buffer to the vertex position attribute.
	void detachPositions(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader) {
		detach(renderer, shader, "position");
	}

	/// Binds this buffer to the vertex color attribute of a vertex shader.
	void bindColors(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, int components, size_t byteOffset = 0) {
		bind(renderer, shader, "color", GL_FLOAT, byteOffset, components, sizeof(T));
	}

	void setUniformColor(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, const Color& c) {
		shader->setAttributeValue("color", c.r(), c.g(), c.b());
	}

	void setUniformColor(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, const ColorA& c) {
		shader->setAttributeValue("color", c.r(), c.g(), c.b(), c.a());
	}

	/// After rendering is done, release the binding of the buffer to the vertex color attribute.
	void detachColors(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader) {
		detach(renderer, shader, "color");
	}

	/// Binds this buffer to the vertex normal attribute of a vertex shader.
	void bindNormals(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader, size_t byteOffset = 0) {
		bind(renderer, shader, "normal", GL_FLOAT, byteOffset, 3, sizeof(T));
	}

	/// After rendering is done, release the binding of the buffer to the vertex normal attribute.
	void detachNormals(OpenGLSceneRenderer* renderer, QOpenGLShaderProgram* shader) {
		detach(renderer, shader, "normal");
	}

private:

	/// Indicates the use of OpenGL ES instead of desktop OpenGL.
	bool _isOpenGLES = false;

	/// The OpenGL vertex buffer.
	QOpenGLBuffer _buffer;

	/// The number of elements stored in the buffer.
	int _elementCount;

	/// The number of vertices per element.
	int _verticesPerElement;

	/// OpenGL ES may not support memory mapping a GL buffer.
	/// This is a host memory buffer used to emulate the map() method on this platform.
	std::vector<T> _temporaryBuffer;

	/// The OVITO data buffer that is used to fill the VBO.
	ConstDataBufferPtr _sourceBuffer;
};

}	// End of namespace
