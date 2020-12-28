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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/rendering/ArrowPrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief Buffer object that stores a set of arrows to be rendered in the viewports.
 */
class OpenGLArrowPrimitive : public ArrowPrimitive, public std::enable_shared_from_this<OpenGLArrowPrimitive>
{
public:

	/// Constructor.
	OpenGLArrowPrimitive(OpenGLSceneRenderer* renderer, ArrowPrimitive::Shape shape, ShadingMode shadingMode, RenderingQuality renderingQuality, bool translucentElements);

	/// \brief Allocates a geometry buffer with the given number of elements.
	virtual void startSetElements(int elementCount) override;

	/// \brief Returns the number of elements stored in the buffer.
	virtual int elementCount() const override { return _elementCount; }

	/// \brief Sets the properties of a single line element.
	virtual void setElement(int index, const Point3& pos, const Vector3& dir, const ColorA& color, FloatType width) override;

	/// \brief Finalizes the geometry buffer after all elements have been set.
	virtual void endSetElements() override;

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

private:

	/// \brief Creates the geometry for a single cylinder element.
	void createCylinderElement(int index, const Point_3<float>& pos, const Vector_3<float>& dir, const ColorAT<float>& color, float width);

	/// \brief Creates the geometry for a single arrow element.
	void createArrowElement(int index, const Point_3<float>& pos, const Vector_3<float>& dir, const ColorAT<float>& color, float width);

	/// Renders the geometry as triangle mesh with normals.
	void renderWithNormals(OpenGLSceneRenderer* renderer);

	/// Renders the geometry as with extra information passed to the vertex shader.
	void renderWithElementInfo(OpenGLSceneRenderer* renderer);

private:

	/// Per-vertex data stored in VBOs when rendering triangle geometry.
	struct VertexWithNormal {
		Point_3<float> pos;
		Vector_3<float> normal;
		ColorAT<float> color;
	};

	/// Per-vertex data stored in VBOs when rendering raytraced cylinders.
	struct VertexWithElementInfo {
		Point_3<float> pos;
		Point_3<float> base;
		Vector_3<float> dir;
		ColorAT<float> color;
		float radius;
	};

	/// The GL context group under which the GL vertex buffers have been created.
	QPointer<QOpenGLContextGroup> _contextGroup;

	/// The number of elements stored in the buffer.
	int _elementCount = -1;

	/// The number of cylinder segments to generate.
	int _cylinderSegments = 16;

	/// The number of mesh vertices generated per primitive.
	int _verticesPerElement = 0;

	/// Indicates whether OpenGL geometry shaders are supported.
	bool _usingGeometryShader;

	/// The OpenGL vertex buffer objects that store the vertices with normal vectors for polygon rendering.
	OpenGLBuffer<VertexWithNormal> _verticesWithNormals;

	/// The OpenGL vertex buffer objects that store the vertices with full element info for raytraced shader rendering.
	OpenGLBuffer<VertexWithElementInfo> _verticesWithElementInfo;

	/// Pointer to the memory-mapped VBO buffer.
	VertexWithNormal* _mappedVerticesWithNormals = nullptr;

	/// Pointer to the memory-mapped VBO buffer.
	VertexWithElementInfo* _mappedVerticesWithElementInfo = nullptr;

	/// The OpenGL shader program that is used for rendering.
	QOpenGLShaderProgram* _shader = nullptr;

	/// Lookup table for fast cylinder geometry generation.
	std::vector<float> _cosTable;

	/// Lookup table for fast cylinder geometry generation.
	std::vector<float> _sinTable;

	/// Primitive start indices passed to glMultiDrawArrays() using GL_TRIANGLE_STRIP primitives.
	std::vector<GLint> _stripPrimitiveVertexStarts;

	/// Primitive vertex counts passed to glMultiDrawArrays() using GL_TRIANGLE_STRIP primitives.
	std::vector<GLsizei> _stripPrimitiveVertexCounts;

	/// Primitive start indices passed to glMultiDrawArrays() using GL_TRIANGLE_FAN primitives.
	std::vector<GLint> _fanPrimitiveVertexStarts;

	/// Primitive vertex counts passed to glMultiDrawArrays() using GL_TRIANGLE_FAN primitives.
	std::vector<GLsizei> _fanPrimitiveVertexCounts;

	// OpenGL ES only:

	/// The number of vertex indices need per element.
	int _indicesPerElement = 0;

	/// Vertex indices passed to glDrawElements() using GL_TRIANGLES primitives.
	std::vector<GLuint> _trianglePrimitiveVertexIndices;
};

}	// End of namespace
