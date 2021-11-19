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
#include <ovito/core/dataset/data/DataBuffer.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include "PrimitiveBase.h"
#include "PseudoColorMapping.h"

namespace Ovito {

/**
 * \brief A triangle mesh to be rendered by a SceneRenderer implementation.
 */
class OVITO_CORE_EXPORT MeshPrimitive : public PrimitiveBase
{
public:

	enum DepthSortingMode {
		AnyShapeMode,
		ConvexShapeMode,
	};
	Q_ENUMS(DepthSortingMode);

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, DepthSortingMode depthSortingMode = AnyShapeMode) {
		// Create a shallow copy of the mesh and store it in this buffer object.
		_mesh = mesh;
		_isMeshFullyOpaque.reset();
	}

	/// \brief Returns the number of triangle faces stored in the buffer.
	int faceCount() { return _mesh.faceCount(); }

	/// Returns the triangle mesh stored in this geometry buffer.
	const TriMesh& mesh() const { return _mesh; }

	/// \brief Enables or disables the culling of triangles not facing the viewer.
	void setCullFaces(bool enable) { _cullFaces = enable; }

	/// \brief Returns whether the culling of triangles not facing the viewer is enabled.
	bool cullFaces() const { return _cullFaces; }

	/// Indicates whether mesh edges are rendered as wireframe.
	bool emphasizeEdges() const { return _emphasizeEdges; }

	/// Sets whether mesh edges are rendered as wireframe.
	virtual void setEmphasizeEdges(bool emphasizeEdges) { _emphasizeEdges = emphasizeEdges; }

	/// Indicates whether the mesh is fully opaque (no semi-transparent colors).
	bool isFullyOpaque() const;

	/// Sets the rendering color to be used if the mesh doesn't have per-vertex colors.
	virtual void setUniformColor(const ColorA& color) { 
		_uniformColor = color; 
		_isMeshFullyOpaque.reset();
	}

	/// Returns the rendering color to be used if the mesh doesn't have per-vertex colors.
	const ColorA& uniformColor() const { return _uniformColor; }

	/// Returns the array of materials referenced by the materialIndex() field of the mesh faces.
	const std::vector<ColorA>& materialColors() const { return _materialColors; }

	/// Sets array of materials referenced by the materialIndex() field of the mesh faces.
	virtual void setMaterialColors(std::vector<ColorA> colors) { 
		_materialColors = std::move(colors); 
		_isMeshFullyOpaque.reset();
	}

	/// Returns the mapping from pseudo-color values at the mesh vertices to RGB colors.
	const PseudoColorMapping& pseudoColorMapping() const { return _pseudoColorMapping; }

	/// Sets the mapping from pseudo-color values at the mesh vertices to RGB colors.
	void setPseudoColorMapping(const PseudoColorMapping& mapping) { 
		_pseudoColorMapping = mapping; 
	}

	/// Activates rendering of multiple instances of the mesh.
	virtual void setInstancedRendering(ConstDataBufferPtr perInstanceTMs, ConstDataBufferPtr perInstanceColors) {
		OVITO_ASSERT(perInstanceTMs);
		OVITO_ASSERT(!perInstanceColors || perInstanceTMs->size() == perInstanceColors->size());
		OVITO_ASSERT(!perInstanceColors || perInstanceColors->stride() == sizeof(ColorA));
		OVITO_ASSERT(perInstanceTMs->stride() == sizeof(AffineTransformation));

		// Store the data arrays.
		_perInstanceTMs = std::move(perInstanceTMs);
		_perInstanceColors = std::move(perInstanceColors);
		OVITO_ASSERT(useInstancedRendering());
		_isMeshFullyOpaque.reset();
	}

	/// Returns the list of transformation matrices when rendering multiple instances of the mesh is enabled.
	const ConstDataBufferPtr& perInstanceTMs() const { return _perInstanceTMs; }

	/// Returns the list of colors when rendering multiple instances of the mesh is enabled.
	const ConstDataBufferPtr& perInstanceColors() const { return _perInstanceColors; }

	/// Returns whether instanced rendering of the mesh has been activated.
	bool useInstancedRendering() const { return (bool)_perInstanceTMs; }

	/// Returns the color used for rendering all selected faces.
	const Color& faceSelectionColor() const { return _faceSelectionColor; }

	/// \brief Sets the color to be used for rendering the selected mesh faces.
	void setFaceSelectionColor(const Color& color) {
		_faceSelectionColor = color;
	}

private:

	/// Controls the culling of triangles not facing the viewer.
	bool _cullFaces = false;

	/// Indicates whether the mesh's colors are all fully opaque (alpha=1).
	mutable std::optional<bool> _isMeshFullyOpaque;

	/// The array of materials referenced by the materialIndex() field of the mesh faces.
	std::vector<ColorA> _materialColors;

	/// The mesh storing the geometry.
	TriMesh _mesh;

	/// The rendering color to be used if the mesh doesn't have per-vertex colors.
	ColorA _uniformColor{1,1,1,1};

	/// The mapping from pseudo-color values at the mesh vertices to RGB colors.
	PseudoColorMapping _pseudoColorMapping;

	/// Controls the rendering of edge wireframe.
	bool _emphasizeEdges = false;

	/// The list of transformation matrices when rendering multiple instances of the mesh.
	ConstDataBufferPtr _perInstanceTMs;	// Array of AffineTransformation

	/// The list of colors when rendering multiple instances of the mesh.
	ConstDataBufferPtr _perInstanceColors; // Array of ColorA

	/// The color used for rendering all selected faces.
	Color _faceSelectionColor{1,0,0};
};

}	// End of namespace
