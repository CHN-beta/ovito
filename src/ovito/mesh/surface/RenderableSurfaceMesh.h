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


#include <ovito/mesh/Mesh.h>
#include <ovito/core/dataset/data/TransformedDataObject.h>
#include <ovito/core/dataset/data/mesh/TriMeshObject.h>

namespace Ovito::Mesh {

/**
 * \brief A non-periodic triangle mesh that is generated from a periodic SurfaceMesh.
 */
class OVITO_MESH_EXPORT RenderableSurfaceMesh : public TransformedDataObject
{
	OVITO_CLASS(RenderableSurfaceMesh)
	Q_CLASSINFO("DisplayName", "Renderable surface mesh");

public:

	/// Constructor.
	Q_INVOKABLE RenderableSurfaceMesh(ObjectCreationParams params, TransformingDataVis* creator = nullptr, const DataObject* sourceData = nullptr, DataOORef<const TriMeshObject> surfaceMesh = {}, DataOORef<const TriMeshObject> capPolygonsMesh = {}, bool backfaceCulling = false) 
		: TransformedDataObject(params, creator, sourceData), 
		_surfaceMesh(std::move(surfaceMesh)),
		_capPolygonsMesh(std::move(capPolygonsMesh)),
		_backfaceCulling(backfaceCulling) 
	{
		// Adopt the ID string from the original data object.
		if(sourceData)
			setIdentifier(sourceData->identifier());
	}

private:

	/// The surface part of the mesh.
	DECLARE_RUNTIME_PROPERTY_FIELD(DataOORef<const TriMeshObject>, surfaceMesh, setSurfaceMesh);

	/// The cap polygon part of the mesh.
	DECLARE_RUNTIME_PROPERTY_FIELD(DataOORef<const TriMeshObject>, capPolygonsMesh, setCapPolygonsMesh);

	/// The material colors assigned to the surface mesh (optional).
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<ColorA>, materialColors, setMaterialColors);

	/// The mapping of triangles of the renderable surface mesh to the original mesh (optional).
	DECLARE_RUNTIME_PROPERTY_FIELD(std::vector<size_t>, originalFaceMap, setOriginalFaceMap);

	/// Indicates whether triangles of the surface mesh should be rendered with active backface culling.
	DECLARE_RUNTIME_PROPERTY_FIELD(bool, backfaceCulling, setBackfaceCulling);
};

}	// End of namespace
