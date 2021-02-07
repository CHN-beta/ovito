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
#include <ovito/core/rendering/ParticlePrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief This class is responsible for rendering particle primitives using OpenGL.
 */
class OpenGLParticlePrimitive : public ParticlePrimitive
{
public:

	/// Constructor.
	OpenGLParticlePrimitive(OpenGLSceneRenderer* renderer, ShadingMode shadingMode, RenderingQuality renderingQuality, ParticleShape shape);

	/// \brief Renders the geometry.
	void render(OpenGLSceneRenderer* renderer);

	/// Returns the number of particles being rendered.
	int particleCount() const { return _particleCount; }

private:

	/// Returns an array of particle indices, sorted back-to-front, which is used to render translucent particles.
	ConstDataBufferPtr determineRenderingOrder(OpenGLSceneRenderer* renderer) const;

	/// Renders a set of boxes using a glMultiDrawArrays() call.
	void renderBoxGeometries(OpenGLSceneRenderer* renderer);

	/// Renders a set of imposters using triangle geometry.
	void renderImposterGeometries(OpenGLSceneRenderer* renderer);

	/// The implemented techniques for rendering particles.
	enum RenderingTechnique {
		IMPOSTER_QUADS,	///< Render quad geometry made of two triangles.
		BOX_GEOMETRY	///< Render a box for each particle (possibly using a raytracing fragment shader to make it look spherical).
	};

	/// The number of particles stored in the class.
	int _particleCount = -1;

	/// The internal OpenGL index buffer that stores the particle indices to be rendered.
	OpenGLBuffer<int> _indexBuffer{QOpenGLBuffer::IndexBuffer};

	/// The internal OpenGL vertex buffer that stores the particle positions.
	OpenGLBuffer<Point_3<float>> _positionsBuffer;

	/// The internal OpenGL vertex buffer that stores the particle radii.
	OpenGLBuffer<float> _radiiBuffer;

	/// The internal OpenGL vertex buffer that stores the particle colors.
	OpenGLBuffer<ColorT<float>> _colorsBuffer;

	/// The internal OpenGL vertex buffer that stores the particle transparencies.
	OpenGLBuffer<float> _transparenciesBuffer;

	/// The internal OpenGL vertex buffer that stores the particle selection flags.
	OpenGLBuffer<int> _selectionBuffer;

	/// The internal OpenGL vertex buffer that stores the shape of aspherical particles.
	OpenGLBuffer<Vector_3<float>> _shapeBuffer;

	/// The internal OpenGL vertex buffer that stores the orientation of aspherical particles.
	OpenGLBuffer<QuaternionT<float>> _orientationBuffer;

	/// The internal OpenGL vertex buffer that stores the roundness values of superquadric particles.
	OpenGLBuffer<Vector_2<float>> _roundnessBuffer;

	/// Start indices of primitives passed to glMultiDrawArrays().
	std::vector<GLint> _primitiveStartIndices;

	/// Vertex counts of primitives passed to glMultiDrawArrays().
	std::vector<GLsizei> _primitiveVertexCounts;

	/// Part of the caching mechanism for the indices/counts arrays for glMultiDrawArrays().
	ConstDataObjectRef _primitiveIndicesSource{};

	/// OpenGL ES only: Vertex indices passed to glDrawElements() using GL_TRIANGLES primitives.
	std::vector<GLuint> _trianglePrimitiveVertexIndices;

	/// The OpenGL shader program that is used to render the particles.
	QOpenGLShaderProgram* _shader = nullptr;

	/// The technique used to render particles. This depends on settings such as rendering quality, shading etc.
	RenderingTechnique _renderingTechnique;

	/// Number of OpenGL vertices per particle that must be rendered.
	int _verticesPerParticle;
};

}	// End of namespace
