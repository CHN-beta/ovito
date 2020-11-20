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
#include "OpenGLParticlePrimitive.h"
#include "OpenGLSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLParticlePrimitive::OpenGLParticlePrimitive(OpenGLSceneRenderer* renderer, ShadingMode shadingMode,
		RenderingQuality renderingQuality, ParticleShape shape, bool translucentParticles) :
	ParticlePrimitive(shadingMode, renderingQuality, shape, translucentParticles),
	_contextGroup(QOpenGLContextGroup::currentContextGroup())
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);
	QString prefix = renderer->glcontext()->isOpenGLES() ? QStringLiteral(":/openglrenderer_gles") : QStringLiteral(":/openglrenderer");

	// Choose rendering technique for the particles.
	if(shadingMode == FlatShading) {
		_renderingTechnique = IMPOSTER_QUADS;
	}
	else {
		if(shape == SphericalShape && renderingQuality < HighQuality) {
			_renderingTechnique = IMPOSTER_QUADS;
		}
		else {
			_renderingTechnique = BOX_GEOMETRY;
		}
	}

	// Determine the number of OpenGL vertices per particle that must be rendered.
	if(_renderingTechnique == IMPOSTER_QUADS)
		_verticesPerParticle = renderer->useGeometryShaders() ? 1 : 6;
	else if(_renderingTechnique == BOX_GEOMETRY)
		_verticesPerParticle = renderer->useGeometryShaders() ? 1 : 14;

	// Load the right OpenGL shaders.
	if(_renderingTechnique == IMPOSTER_QUADS) {
		if(shadingMode == FlatShading) {
			if(shape == SphericalShape || shape == EllipsoidShape) {
				if(renderer->useGeometryShaders()) {
					_shader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_flat",
							prefix + "/glsl/particles/imposter/sphere/without_depth.vs",
							prefix + "/glsl/particles/imposter/sphere/flat_shading.fs",
							prefix + "/glsl/particles/imposter/sphere/without_depth.gs");
				}
				else {
					_shader = renderer->loadShaderProgram("particle_imposter_spherical_flat",
							prefix + "/glsl/particles/imposter/sphere/without_depth_tri.vs",
							prefix + "/glsl/particles/imposter/sphere/flat_shading.fs");
				}
			}
			else if(shape == SquareCubicShape || shape == BoxShape) {
				if(renderer->useGeometryShaders()) {
					_shader = renderer->loadShaderProgram("particle_geomshader_imposter_square_flat",
							prefix + "/glsl/particles/imposter/sphere/without_depth.vs",
							prefix + "/glsl/particles/imposter/square/flat_shading.fs",
							prefix + "/glsl/particles/imposter/sphere/without_depth.gs");
				}
				else {
					_shader = renderer->loadShaderProgram("particle_imposter_square_flat",
							prefix + "/glsl/particles/imposter/sphere/without_depth_tri.vs",
							prefix + "/glsl/particles/imposter/square/flat_shading.fs");
				}
			}
		}
		else if(shadingMode == NormalShading) {
			if(shape == SphericalShape) {
				if(renderingQuality == LowQuality) {
					if(renderer->useGeometryShaders()) {
						_shader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_shaded_nodepth",
								prefix + "/glsl/particles/imposter/sphere/without_depth.vs",
								prefix + "/glsl/particles/imposter/sphere/without_depth.fs",
								prefix + "/glsl/particles/imposter/sphere/without_depth.gs");
					}
					else {
						_shader = renderer->loadShaderProgram("particle_imposter_spherical_shaded_nodepth",
								prefix + "/glsl/particles/imposter/sphere/without_depth_tri.vs",
								prefix + "/glsl/particles/imposter/sphere/without_depth.fs");
					}
				}
				else if(renderingQuality == MediumQuality) {
					if(renderer->useGeometryShaders()) {
						_shader = renderer->loadShaderProgram("particle_geomshader_imposter_spherical_shaded_depth",
								prefix + "/glsl/particles/imposter/sphere/with_depth.vs",
								prefix + "/glsl/particles/imposter/sphere/with_depth.fs",
								prefix + "/glsl/particles/imposter/sphere/with_depth.gs");
					}
					else {
						_shader = renderer->loadShaderProgram("particle_imposter_spherical_shaded_depth",
								prefix + "/glsl/particles/imposter/sphere/with_depth_tri.vs",
								prefix + "/glsl/particles/imposter/sphere/with_depth.fs");
					}
				}
			}
		}
	}
	else if(_renderingTechnique == BOX_GEOMETRY) {
		if(shadingMode == NormalShading) {
			if(renderer->useGeometryShaders()) {
				if(shape == SphericalShape && renderingQuality == HighQuality) {
					_shader = renderer->loadShaderProgram("particle_geomshader_sphere",
							prefix + "/glsl/particles/geometry/sphere/sphere.vs",
							prefix + "/glsl/particles/geometry/sphere/sphere.fs",
							prefix + "/glsl/particles/geometry/sphere/sphere.gs");
				}
				else if(shape == SquareCubicShape) {
					_shader = renderer->loadShaderProgram("particle_geomshader_cube",
							prefix + "/glsl/particles/geometry/cube/cube.vs",
							prefix + "/glsl/particles/geometry/cube/cube.fs",
							prefix + "/glsl/particles/geometry/cube/cube.gs");
				}
				else if(shape == BoxShape) {
					_shader = renderer->loadShaderProgram("particle_geomshader_box",
							prefix + "/glsl/particles/geometry/box/box.vs",
							prefix + "/glsl/particles/geometry/cube/cube.fs",
							prefix + "/glsl/particles/geometry/box/box.gs");
				}
				else if(shape == EllipsoidShape) {
					_shader = renderer->loadShaderProgram("particle_geomshader_ellipsoid",
							prefix + "/glsl/particles/geometry/ellipsoid/ellipsoid.vs",
							prefix + "/glsl/particles/geometry/ellipsoid/ellipsoid.fs",
							prefix + "/glsl/particles/geometry/ellipsoid/ellipsoid.gs");
				}
			}
			else {
				if(shape == SphericalShape && renderingQuality == HighQuality) {
					_shader = renderer->loadShaderProgram("particle_tristrip_sphere",
							prefix + "/glsl/particles/geometry/sphere/sphere_tristrip.vs",
							prefix + "/glsl/particles/geometry/sphere/sphere.fs");
				}
				else if(shape == SquareCubicShape) {
					_shader = renderer->loadShaderProgram("particle_tristrip_cube",
							prefix + "/glsl/particles/geometry/cube/cube_tristrip.vs",
							prefix + "/glsl/particles/geometry/cube/cube.fs");
				}
				else if(shape == BoxShape) {
					_shader = renderer->loadShaderProgram("particle_tristrip_box",
							prefix + "/glsl/particles/geometry/box/box_tristrip.vs",
							prefix + "/glsl/particles/geometry/cube/cube.fs");
				}
				else if(shape == EllipsoidShape) {
					_shader = renderer->loadShaderProgram("particle_tristrip_ellipsoid",
							prefix + "/glsl/particles/geometry/ellipsoid/ellipsoid_tristrip.vs",
							prefix + "/glsl/particles/geometry/ellipsoid/ellipsoid.fs");
				}
			}
		}
	}
	OVITO_ASSERT(_shader != nullptr);
}

/******************************************************************************
* Allocates a particle buffer with the given number of particles.
******************************************************************************/
void OpenGLParticlePrimitive::setSize(int particleCount)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	_particleCount = particleCount;

	// Allocate VBOs.
	_positionsBuffer.create(QOpenGLBuffer::StaticDraw, particleCount, _verticesPerParticle);
	_radiiBuffer.create(QOpenGLBuffer::StaticDraw, particleCount, _verticesPerParticle);
	_colorsBuffer.create(QOpenGLBuffer::StaticDraw, particleCount, _verticesPerParticle);
	if(particleShape() == BoxShape || particleShape() == EllipsoidShape) {
		_shapeBuffer.create(QOpenGLBuffer::StaticDraw, particleCount, _verticesPerParticle);
		_shapeBuffer.fillConstant(Vector_3<float>::Zero());
		_orientationBuffer.create(QOpenGLBuffer::StaticDraw, particleCount, _verticesPerParticle);
		_orientationBuffer.fillConstant(QuaternionT<float>(0,0,0,1));
	}
}

/******************************************************************************
* Sets the coordinates of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticlePositions(const Point3* coordinates)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	// Make a copy of the particle coordinates. They will be needed when rendering
	// semi-transparent particles in the correct order from back to front.
	if(translucentParticles()) {
		_particleCoordinates.resize(particleCount());
		std::copy(coordinates, coordinates + particleCount(), _particleCoordinates.begin());
	}

	_positionsBuffer.fill(coordinates);
}

/******************************************************************************
* Sets the radii of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleRadii(const FloatType* radii)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_radiiBuffer.fill(radii);
}

/******************************************************************************
* Sets the radius of all particles to the given value.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleRadius(FloatType radius)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_radiiBuffer.fillConstant(radius);
}

/******************************************************************************
* Sets the colors of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleColors(const ColorA* colors)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorsBuffer.fill(colors);
}

/******************************************************************************
* Sets the colors of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleColors(const Color* colors)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);

	// Need to convert array from Color to ColorA.
	ColorAT<float>* dest = _colorsBuffer.map();
	const Color* end_colors = colors + _colorsBuffer.elementCount();
	for(; colors != end_colors; ++colors) {
		for(int i = 0; i < _colorsBuffer.verticesPerElement(); i++, ++dest) {
			dest->r() = (float)colors->r();
			dest->g() = (float)colors->g();
			dest->b() = (float)colors->b();
			dest->a() = 1;
		}
	}
	_colorsBuffer.unmap();
}

/******************************************************************************
* Sets the color of all particles to the given value.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleColor(const ColorA color)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	_colorsBuffer.fillConstant(color);
}

/******************************************************************************
* Sets the aspherical shapes of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleShapes(const Vector3* shapes)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	if(_shapeBuffer.isCreated())
		_shapeBuffer.fill(shapes);
}

/******************************************************************************
* Sets the orientations of aspherical particles.
******************************************************************************/
void OpenGLParticlePrimitive::setParticleOrientations(const Quaternion* orientations)
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	if(_orientationBuffer.isCreated())
		_orientationBuffer.fill(orientations);
}

/******************************************************************************
* Resets the aspherical shape of the particles.
******************************************************************************/
void OpenGLParticlePrimitive::clearParticleShapes()
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	if(_shapeBuffer.isCreated())
		_shapeBuffer.fillConstant(Vector_3<float>::Zero());
}

/******************************************************************************
* Resets the orientation of particles.
******************************************************************************/
void OpenGLParticlePrimitive::clearParticleOrientations()
{
	OVITO_ASSERT(QOpenGLContextGroup::currentContextGroup() == _contextGroup);
	if(_orientationBuffer.isCreated())
		_orientationBuffer.fillConstant(QuaternionT<float>(0,0,0,1));
}

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLParticlePrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);
	if(!vpRenderer) return false;
	return (_particleCount >= 0) && (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void OpenGLParticlePrimitive::render(SceneRenderer* sceneRenderer)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());

	OpenGLSceneRenderer* renderer = dynamic_object_cast<OpenGLSceneRenderer>(sceneRenderer);
    OVITO_REPORT_OPENGL_ERRORS(renderer);

	if(particleCount() <= 0 || !renderer)
		return;

	// If object is translucent, don't render it during the first rendering pass.
	// Queue primitive so that it gets rendered during the second pass.
	if(!renderer->isPicking() && translucentParticles() && renderer->translucentPass() == false) {
		renderer->registerTranslucentPrimitive(shared_from_this());
		return;
	}

	renderer->rebindVAO();

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
	if(!renderer->isPicking() && translucentParticles()) {
		renderer->glEnable(GL_BLEND);
		renderer->glBlendEquation(GL_FUNC_ADD);
		renderer->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR, GL_ONE);
	}

	// Make vertex IDs available to the shader.
	if(renderer->isPicking()) {
		GLint pickingBaseID = renderer->registerSubObjectIDs(particleCount());
		shader->setUniformValue("picking_base_id", pickingBaseID);
	}
	renderer->activateVertexIDs(shader, particleCount() * _verticesPerParticle);

	// Bind VBOs.
	_positionsBuffer.bindPositions(renderer, shader);
	if(_shapeBuffer.isCreated())
		_shapeBuffer.bind(renderer, shader, "shape", GL_FLOAT, 0, 3);
	if(_orientationBuffer.isCreated())
		_orientationBuffer.bind(renderer, shader, "orientation", GL_FLOAT, 0, 4);
	_radiiBuffer.bind(renderer, shader, "particle_radius", GL_FLOAT, 0, 1);
	if(!renderer->isPicking())
		_colorsBuffer.bindColors(renderer, shader, 4);

	if(renderer->useGeometryShaders()) {
		// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
		if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
			// Create OpenGL index buffer which can be used with glDrawElements.
			OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
			primitiveIndices.create(QOpenGLBuffer::StaticDraw, particleCount());
			primitiveIndices.fill(determineRenderingOrder(renderer).data());
			primitiveIndices.oglBuffer().bind();
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_POINTS, particleCount(), GL_UNSIGNED_INT, nullptr));
			primitiveIndices.oglBuffer().release();
		}
		else {
			// Fully opaque particles can be rendered in unsorted storage order.
			OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_POINTS, 0, particleCount()));
		}
	}
	else {
		if(!QOpenGLContext::currentContext()->isOpenGLES()) {
			if(_renderingTechnique == BOX_GEOMETRY) {
				// Prepare arrays required for glMultiDrawArrays().
				// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
				if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
					auto indices = determineRenderingOrder(renderer);
					_primitiveStartIndices.clear();
					_primitiveStartIndices.resize(particleCount());
					std::transform(indices.begin(), indices.end(), _primitiveStartIndices.begin(), [this](GLuint i) { return i * _verticesPerParticle; });
					if(_primitiveVertexCounts.size() != particleCount()) {
						_primitiveVertexCounts.clear();
						_primitiveVertexCounts.resize(particleCount());
						std::fill(_primitiveVertexCounts.begin(), _primitiveVertexCounts.end(), _verticesPerParticle);
					}
				}
				else if(_primitiveStartIndices.size() < particleCount()) {
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

				OVITO_CHECK_OPENGL(renderer, 
					renderer->glMultiDrawArrays(GL_TRIANGLE_STRIP,
						_primitiveStartIndices.data(),
						_primitiveVertexCounts.data(),
						particleCount()));
			}
			else if(_renderingTechnique == IMPOSTER_QUADS) {
				// Are we rendering translucent particles? If yes, render them in back to front order to avoid visual artifacts at overlapping particles.
				if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
					auto indices = determineRenderingOrder(renderer);
					// Create OpenGL index buffer which can be used with glDrawElements.
					OpenGLBuffer<GLuint> primitiveIndices(QOpenGLBuffer::IndexBuffer);
					primitiveIndices.create(QOpenGLBuffer::StaticDraw, _verticesPerParticle * particleCount());
					GLuint* p = primitiveIndices.map();
					for(size_t i = 0; i < indices.size(); i++, p += _verticesPerParticle)
						std::iota(p, p + _verticesPerParticle, indices[i] * _verticesPerParticle);
					primitiveIndices.unmap();
					primitiveIndices.oglBuffer().bind();
					OVITO_CHECK_OPENGL(renderer, renderer->glDrawElements(GL_TRIANGLES, particleCount() * _verticesPerParticle, GL_UNSIGNED_INT, nullptr));
					primitiveIndices.oglBuffer().release();
				}
				else {
					// Fully opaque particles can be rendered in unsorted storage order.
					OVITO_CHECK_OPENGL(renderer, renderer->glDrawArrays(GL_TRIANGLES, 0, particleCount() * _verticesPerParticle));
				}
			}
		}
		else {
			// glMultiDrawArrays() is not available in OpenGL ES. Use glDrawElements() instead.
			int indicesPerElement = 3 * 12; // (3 vertices per triangle) * (12 triangles per cube).
			if(!renderer->isPicking() && translucentParticles() && !_particleCoordinates.empty()) {
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
		}
	}

	// Detach VBOs.
	_positionsBuffer.detachPositions(renderer, shader);
	if(!renderer->isPicking())
		_colorsBuffer.detachColors(renderer, shader);
	if(_shapeBuffer.isCreated())
		_shapeBuffer.detach(renderer, shader, "shape");
	if(_orientationBuffer.isCreated())
		_orientationBuffer.detach(renderer, shader, "orientation");
	_radiiBuffer.detach(renderer, shader, "particle_radius");

	// Reset state.
	renderer->deactivateVertexIDs(shader);
	shader->release();
	renderer->glDisable(GL_CULL_FACE);
	renderer->glDisable(GL_BLEND);
}

/******************************************************************************
* Returns an array of particle indices, sorted back-to-front, which is used
* to render translucent particles.
******************************************************************************/
std::vector<GLuint> OpenGLParticlePrimitive::determineRenderingOrder(OpenGLSceneRenderer* renderer)
{
	// Create array of particle indices.
	std::vector<GLuint> indices(particleCount());
	std::iota(indices.begin(), indices.end(), 0);
	if(!_particleCoordinates.empty()) {
		// Viewing direction in object space:
		Vector3 direction = renderer->modelViewTM().inverse().column(2);

		OVITO_ASSERT(_particleCoordinates.size() == particleCount());
		// First compute distance of each particle from the camera along viewing direction (=camera z-axis).
		std::vector<FloatType> distances(particleCount());
		std::transform(_particleCoordinates.begin(), _particleCoordinates.end(), distances.begin(), [direction](const Point3& p) {
			return direction.dot(p - Point3::Origin());
		});
		// Now sort particle indices with respect to distance (back-to-front order).
		std::sort(indices.begin(), indices.end(), [&distances](GLuint a, GLuint b) {
			return distances[a] < distances[b];
		});
	}
	return indices;
}

}	// End of namespace
