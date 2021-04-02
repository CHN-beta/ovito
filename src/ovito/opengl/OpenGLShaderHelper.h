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

#pragma once


#include <ovito/core/Core.h>
#include "OpenGLSceneRenderer.h"
#include "OpenGLResourceManager.h"

namespace Ovito {

/**
 * \brief A helper class that creates and binds GLSL shader programs.
 */
class OpenGLShaderHelper
{
public:

	/// Enum specifying the rate at which vertex attributes are pulled from buffers.
	enum VertexInputRate {
		PerVertex, 		///< Specifies that vertex attribute addressing is a function of the vertex index.
		PerInstance		///< Specifies that vertex attribute addressing is a function of the instance index.
	};

	/// Constructor.
	OpenGLShaderHelper(OpenGLSceneRenderer* renderer) : _renderer(renderer) {}

	/// Loads a shader program.
	void load(const QString& id, const QString& vertexShaderFile, const QString& fragmentShaderFile, const QString& geometryShaderFile = QString()) {
		if(_shader)
			_shader->release();

		// Determine resource path prefix depending on whether we are running on a Desktop or GLES platform.
		QString prefix = _renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles/glsl/") : QStringLiteral(":/openglrenderer/glsl/");

		// Compile the shader program.
		_shader = _renderer->loadShaderProgram(id, 
			prefix + vertexShaderFile, 
			prefix + fragmentShaderFile, 
			!geometryShaderFile.isEmpty() ? prefix + geometryShaderFile : QString());
		OVITO_REPORT_OPENGL_ERRORS(_renderer);

		// Bind the OpenGL shader program.
		if(!_shader->bind())
			_renderer->throwException(QStringLiteral("Failed to bind OpenGL shader '%1'.").arg(id));
		OVITO_REPORT_OPENGL_ERRORS(_renderer);

		// Set shader uniforms.
		OVITO_CHECK_OPENGL(_renderer, _shader->setUniformValue("modelview_projection_matrix", static_cast<QMatrix4x4>(_renderer->projParams().projectionMatrix * _renderer->modelViewTM())));
		OVITO_CHECK_OPENGL(_renderer, _shader->setUniformValue("projection_matrix", static_cast<QMatrix4x4>(_renderer->projParams().projectionMatrix)));
		OVITO_CHECK_OPENGL(_renderer, _shader->setUniformValue("inverse_projection_matrix", static_cast<QMatrix4x4>(_renderer->projParams().inverseProjectionMatrix)));
		OVITO_CHECK_OPENGL(_renderer, _shader->setUniformValue("modelview_matrix", static_cast<QMatrix4x4>(Matrix4(_renderer->modelViewTM()))));

		// Get current viewport rectangle.
		GLint viewportCoords[4];
		OVITO_CHECK_OPENGL(_renderer, _renderer->glGetIntegerv(GL_VIEWPORT, viewportCoords));
		OVITO_CHECK_OPENGL(_renderer, _shader->setUniformValue("viewport_origin", (float)viewportCoords[0], (float)viewportCoords[1]));
		OVITO_CHECK_OPENGL(_renderer, _shader->setUniformValue("inverse_viewport_size", 2.0f / (float)viewportCoords[2], 2.0f / (float)viewportCoords[3]));
	}

	/// Destructor.
	~OpenGLShaderHelper() {
		if(_shader) {
			// Reset attribute states.
			for(GLuint attrIndex : _instanceAttributes)
				_renderer->glVertexAttribDivisor(attrIndex, 0);

			// Unbind the shader program.
			_shader->release();
		}
	}

	/// Binds an OpenGL buffer to a vertex attribute of the shader.
	void bindBuffer(QOpenGLBuffer& buffer, const char* attributeName, GLenum type, int tupleSize, int stride = 0, int offset = 0, VertexInputRate inputRate = PerVertex) {
		OVITO_REPORT_OPENGL_ERRORS(_renderer);
		OVITO_ASSERT(buffer.isCreated());
		if(!buffer.bind()) {
			qWarning() << "OpenGLShaderHelper::bindBuffer() failed for shader" << _shader->objectName();
			_renderer->throwException(QStringLiteral("Failed to bind OpenGL vertex buffer for shader '%1'.").arg(_shader->objectName()));
		}
		GLuint attrIndex = _shader->attributeLocation(attributeName);
		OVITO_ASSERT(attrIndex >= 0);
		OVITO_CHECK_OPENGL(_renderer, _shader->setAttributeBuffer(attrIndex, type, offset, tupleSize, stride));
		OVITO_CHECK_OPENGL(_renderer, _shader->enableAttributeArray(attrIndex));
		if(inputRate == PerInstance) {
			_renderer->glVertexAttribDivisor(attrIndex, 1);
			_instanceAttributes.push_back(attrIndex);
		}
		buffer.release();
	}

	/// Passes the base object ID to the shader in picking mode.
	void setPickingBaseId(GLint baseId) {
		OVITO_ASSERT(_renderer->isPicking());
		_shader->setUniformValue("picking_base_id", baseId);
	}

	/// Passes a uniform value to the shader.
	void setUniformValue(const char* name, const ColorA& color) {
		_shader->setUniformValue(name, color.r(), color.g(), color.b(), color.a());
	}

	/// Passes a uniform value to the shader.
	void setUniformValue(const char* name, const Vector4& vec) {
		_shader->setUniformValue(name, vec.x(), vec.y(), vec.z(), vec.w());
	}

	/// Passes a uniform value to the shader.
	void setUniformValue(const char* name, FloatType value) {
		_shader->setUniformValue(name, static_cast<GLfloat>(value));
	}

	/// Uploads some data to the Vulkan device as a buffer object and caches it.
	template<typename KeyType>
	static QOpenGLBuffer createCachedBuffer(KeyType&& cacheKey, int bufferSize, OpenGLResourceManager::ResourceFrameHandle resourceFrame, QOpenGLBuffer::Type usage, std::function<void(void*)>&& fillMemoryFunc) {
		// Check if this OVITO data buffer has already been uploaded to the GPU.
		QOpenGLBuffer& bufferObject = OpenGLResourceManager::instance()->lookup<QOpenGLBuffer>(std::forward<KeyType>(cacheKey), resourceFrame);

		// If not, do it now.
		if(!bufferObject.isCreated())
			bufferObject = createCachedBufferImpl(bufferSize, usage, std::move(fillMemoryFunc));

		return bufferObject;
	}

	/// Uploads an OVITO DataBuffer to the Vulkan device.
	static QOpenGLBuffer uploadDataBuffer(const ConstDataBufferPtr& dataBuffer, OpenGLResourceManager::ResourceFrameHandle resourceFrame, QOpenGLBuffer::Type usage = QOpenGLBuffer::VertexBuffer);

private:

	/// Uploads some data to a new OpenGL buffer object.
	static QOpenGLBuffer createCachedBufferImpl(int bufferSize, QOpenGLBuffer::Type usage, std::function<void(void*)>&& fillMemoryFunc);

	QOpenGLShaderProgram* _shader = nullptr;
	OpenGLSceneRenderer* _renderer;

	/// List of shader vertex attributes that have been marked as per-instance attributes.
	QVarLengthArray<GLuint, 4> _instanceAttributes;
};

}	// End of namespace
