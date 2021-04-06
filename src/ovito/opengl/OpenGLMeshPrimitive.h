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
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/data/DataBuffer.h>
#include <ovito/core/utilities/mesh/TriMesh.h>

#include <QOpenGLBuffer>

namespace Ovito {

class OpenGLSceneRenderer; // defined in OpenGLSceneRenderer.h
class OpenGLShaderHelper;  // defined in OpenGLShaderHelper.h

/**
 * \brief Buffer object that stores a triangle mesh to be rendered in the viewports.
 */
class OpenGLMeshPrimitive : public MeshPrimitive, public std::enable_shared_from_this<OpenGLMeshPrimitive>
{
public:

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, DepthSortingMode depthSortingMode) override {
		MeshPrimitive::setMesh(mesh, depthSortingMode);
		_depthSortingMode = depthSortingMode;
		_wireframeLines.reset();
	}

	/// \brief Renders the geometry.
	void render(OpenGLSceneRenderer* renderer);

private:

	/// Renders the mesh wireframe edges.
	void renderWireframe(OpenGLSceneRenderer* renderer);

	/// Generates the list of wireframe line elements.
	const ConstDataBufferPtr& wireframeLines(SceneRenderer* renderer);

	/// Prepares the OpenGL buffer with the per-instance transformation matrices.
	QOpenGLBuffer getInstanceTMBuffer(OpenGLSceneRenderer* renderer, OpenGLShaderHelper& shader);

	/// Stores data of a single vertex passed to the shader.
	struct ColoredVertexWithNormal {
		Point_3<float> position;
		Vector_3<float> normal;
		ColorAT<float> color;
	};

	/// The list of wireframe line elements.
	ConstDataBufferPtr _wireframeLines;	// Array of Point3, two per line element.

	/// Controls how the OpenGL renderer performs depth-correct rendering of semi-transparent meshes.
	DepthSortingMode _depthSortingMode = AnyShapeMode;
};

}	// End of namespace
