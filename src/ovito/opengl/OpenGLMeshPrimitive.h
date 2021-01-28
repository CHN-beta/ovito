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
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/data/DataBuffer.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief Buffer object that stores a triangle mesh to be rendered in the viewports.
 */
class OpenGLMeshPrimitive : public MeshPrimitive
{
public:

	/// Constructor.
	OpenGLMeshPrimitive(OpenGLSceneRenderer* renderer);

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, DepthSortingMode depthSortingMode) override {
		MeshPrimitive::setMesh(mesh, depthSortingMode);
		_depthSortingMode = depthSortingMode;
		discardBuffers();
	}

	/// Sets array of materials referenced by the materialIndex() field of the mesh faces.
	virtual void setMaterialColors(std::vector<ColorA> colors) override { 
		MeshPrimitive::setMaterialColors(std::move(colors));
		discardBuffers();
	}

	/// Sets the rendering color to be used if the mesh doesn't have per-vertex colors.
	virtual void setUniformColor(const ColorA& color) override { 
		MeshPrimitive::setUniformColor(color); 
		discardBuffers();
	}

	/// Sets whether mesh edges are rendered as wireframe.
	virtual void setEmphasizeEdges(bool emphasizeEdges) override { 
		MeshPrimitive::setEmphasizeEdges(emphasizeEdges); 
		discardBuffers();
	}

	/// \brief Renders the geometry.
	void render(OpenGLSceneRenderer* renderer);

private:

	/// Throws away the OpenGL vertex buffers whenever the mesh changes.
	void discardBuffers();

	/// Fills the OpenGL vertex buffers with the mesh data.
	void setupBuffers();

private:

	/// Stores data of a single vertex passed to the OpenGL shader.
	struct ColoredVertexWithNormal {
		Point_3<float> pos;
		Vector_3<float> normal;
		ColorAT<float> color;
	};

	/// The internal OpenGL vertex buffer that stores the per-vertex data.
	OpenGLBuffer<ColoredVertexWithNormal> _vertexBuffer;

	/// The OpenGL shader program for renderint the triangles.
	QOpenGLShaderProgram* _faceShader;

	/// The OpenGL shader program for rendering the wireframe edges.
	QOpenGLShaderProgram* _edgeShader;

	/// Controls how the OpenGL renderer performs depth-correct rendering of semi-transparent meshes.
	DepthSortingMode _depthSortingMode = AnyShapeMode;

	/// Stores the center coordinates of the triangles, which are used to render semi-transparent faces in the 
	/// correct order from back to front.
	std::vector<Vector_3<float>> _triangleDepthSortData;

	/// The internal OpenGL vertex buffer that stores the vertex data for rendering polygon edges.
	OpenGLBuffer<Point_3<float>> _edgeLinesBuffer;
};

}	// End of namespace
