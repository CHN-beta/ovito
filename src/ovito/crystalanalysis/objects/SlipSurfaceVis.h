////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/mesh/surface/RenderableSurfaceMesh.h>
#include <ovito/particles/objects/ParticleType.h>

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief A visualization element for rendering the slip facets of a Microstructure.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SlipSurfaceVis : public SurfaceMeshVis
{
	Q_OBJECT
	OVITO_CLASS(SlipSurfaceVis)

	Q_CLASSINFO("DisplayName", "Slip surfaces");

public:

	/// Constructor.
	Q_INVOKABLE SlipSurfaceVis(DataSet* dataset);

protected:

	/// Computation engine that builds the render mesh.
	class PrepareMeshEngine : public SurfaceMeshVis::PrepareSurfaceEngine
	{
	public:

		/// Constructor.
		PrepareMeshEngine(const SurfaceMesh* microstructure, QVector<Plane3> cuttingPlanes, bool smoothShading);

	protected:

		/// Determines the set of visible mesh faces,
		virtual void determineVisibleFaces() override;

		/// Assigns colors to invidual mesh faces.
		virtual void determineFaceColors() override;

	private:

		/// The input microstructure data.
		MicrostructureAccess _microstructure;

		/// Mapping of microstructure phases to standard crystal types.
		std::map<int,ParticleType::PredefinedStructureType> _phaseStructureTypes;
	};

	/// Creates the asynchronous task that builds the non-peridic representation of the input surface mesh.
	virtual std::shared_ptr<PrepareSurfaceEngine> createSurfaceEngine(const SurfaceMesh* mesh) const override {
		return std::make_shared<PrepareMeshEngine>(mesh, mesh->cuttingPlanes(), smoothShading());
	}

	/// Create the viewport picking record for the surface mesh object.
	virtual OORef<ObjectPickInfo> createPickInfo(const SurfaceMesh* mesh, const RenderableSurfaceMesh* renderableMesh) const override;
};

/**
 * \brief This data structure is attached to the slip surface mesh by the SlipSurfaceVis when rendering
 * it in the viewports. It facilitates the picking of slip surface facets with the mouse.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SlipSurfacePickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(SlipSurfacePickInfo)

public:

	/// Constructor.
	SlipSurfacePickInfo(const SlipSurfaceVis* visElement, const SurfaceMesh* surfaceMesh, const RenderableSurfaceMesh* renderableMesh) :
		_visElement(visElement), _surfaceMesh(surfaceMesh), _renderableMesh(renderableMesh) {}

	/// The data object containing the slip surfaces.
	const SurfaceMesh* surfaceMesh() const { return _surfaceMesh; }

	/// The renderable surface mesh for the slip surfaces.
	const RenderableSurfaceMesh* renderableMesh() const { OVITO_ASSERT(_renderableMesh); return _renderableMesh; }

	/// Returns the vis element that rendered the slip surfaces.
	const SlipSurfaceVis* visElement() const { return _visElement; }

	/// \brief Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding slip surface facet.
	int slipFacetIndexFromSubObjectID(quint32 subobjID) const {
		if(subobjID < renderableMesh()->originalFaceMap().size())
			return renderableMesh()->originalFaceMap()[subobjID];
		else
			return -1;
	}

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

private:

	/// The data object containing the slip surfaces.
	OORef<SurfaceMesh> _surfaceMesh;

	/// The renderable surface mesh for the slip surfaces.
	OORef<RenderableSurfaceMesh> _renderableMesh;

	/// The vis element that rendered the slip surfaces.
	OORef<SlipSurfaceVis> _visElement;
};

}	// End of namespace
}	// End of namespace
