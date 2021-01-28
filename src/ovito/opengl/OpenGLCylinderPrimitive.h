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
#include <ovito/core/rendering/CylinderPrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief Buffer object storing a set of cylinders to be rendered in the viewports.
 */
class OpenGLCylinderPrimitive : public CylinderPrimitive
{
public:

	/// Constructor.
	OpenGLCylinderPrimitive(OpenGLSceneRenderer* renderer, CylinderPrimitive::Shape shape, ShadingMode shadingMode, RenderingQuality renderingQuality);

	/// Sets the coordinates of the base and the head points.
	virtual void setPositions(ConstDataBufferPtr baseCoordinates, ConstDataBufferPtr headCoordinates) override {
		CylinderPrimitive::setPositions(std::move(baseCoordinates), std::move(headCoordinates));
		discardBuffers();
	}

	/// Sets the per-primitive colors.
	virtual void setColors(ConstDataBufferPtr colors) override {
		CylinderPrimitive::setColors(std::move(colors));
		discardBuffers();
	}

	/// Sets the transparency values of the primitives.
	virtual void setTransparencies(ConstDataBufferPtr transparencies) override {
		CylinderPrimitive::setTransparencies(std::move(transparencies));
		discardBuffers();
	}

	/// Sets the radii of the primitives.
	virtual void setRadii(ConstDataBufferPtr radii) override {
		CylinderPrimitive::setRadii(std::move(radii));
		discardBuffers();
	}

	/// Sets the cylinder radius of all primitives to the given value.
	virtual void setUniformRadius(FloatType radius) override {
		CylinderPrimitive::setUniformRadius(radius);
		discardBuffers();
	}

	/// Sets the color of all primitives to the given value.
	virtual void setUniformColor(const Color& color) override {
		CylinderPrimitive::setUniformColor(color);
		discardBuffers();
	}

	/// Renders the geometry.
	void render(OpenGLSceneRenderer* renderer);

private:

	/// Creates and fills the OpenGL VBO buffers with data.
	void fillBuffers(OpenGLSceneRenderer* renderer);

	/// Discards the existing OpenGL VBOs so that they get recreated during the next render pass.
	void discardBuffers() {
		_verticesWithNormals.destroy();
		_verticesWithElementInfo.destroy();
	}

	/// Creates the geometry for a single cylinder element.
	void createCylinderElement(OpenGLSceneRenderer* renderer, const Point_3<float>& base, const Point_3<float>& head, const ColorAT<float>& color, float radius);

	/// Creates the geometry for a single arrow element.
	void createArrowElement(OpenGLSceneRenderer* renderer, const Point_3<float>& base, const Point_3<float>& head, const ColorAT<float>& color, float radius);

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
		Point_3<float> head;
		ColorAT<float> color;
		float radius;
	};

	/// The number of cylinder or arrow primitives stored in the buffer.
	int _primitiveCount = 0;

	/// The number of cylinder segments to generate.
	int _cylinderSegments = 16;

	/// The number of mesh vertices generated per primitive.
	int _verticesPerElement = 0;

	/// The OpenGL vertex buffer objects that store the vertices with normal vectors for polygon rendering.
	OpenGLBuffer<VertexWithNormal> _verticesWithNormals;

	/// The OpenGL vertex buffer objects that store the vertices with full element info for raytraced shader rendering.
	OpenGLBuffer<VertexWithElementInfo> _verticesWithElementInfo;

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

	VertexWithNormal* _mappedVerticesWithNormals = nullptr;
	VertexWithElementInfo* _mappedVerticesWithElementInfo = nullptr;

	// OpenGL ES only:

	/// The number of vertex indices need per element.
	int _indicesPerElement = 0;

	/// Vertex indices passed to glDrawElements() using GL_TRIANGLES primitives.
	std::vector<GLuint> _trianglePrimitiveVertexIndices;
};

}	// End of namespace
