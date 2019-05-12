///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <plugins/crystalanalysis/util/ManifoldConstructionHelper.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/dataset/DataSet.h>
#include <core/utilities/units/UnitsManager.h>
#include "ConstructSurfaceModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(ConstructSurfaceModifier);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, smoothingLevel);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, probeSphereRadius);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, selectSurfaceParticles);
DEFINE_REFERENCE_FIELD(ConstructSurfaceModifier, surfaceMeshVis);
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, onlySelectedParticles, "Use only selected input particles");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, selectSurfaceParticles, "Select particles on the surface");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, smoothingLevel, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ConstructSurfaceModifier::ConstructSurfaceModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_smoothingLevel(8),
	_probeSphereRadius(4),
	_onlySelectedParticles(false),
	_selectSurfaceParticles(false)
{
	// Create the vis element for rendering the surface generated by the modifier.
	setSurfaceMeshVis(new SurfaceMeshVis(dataset));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ConstructSurfaceModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ConstructSurfaceModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier inputs.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyPtr selProperty;
	if(onlySelectedParticles())
		selProperty = particles->expectProperty(ParticlesObject::SelectionProperty)->storage();
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The construct surface mesh modifier does not support 2d simulation cells."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ConstructSurfaceEngine>(posProperty->storage(),
			std::move(selProperty),
			simCell->data(),
			probeSphereRadius(),
			smoothingLevel(),
			selectSurfaceParticles());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void ConstructSurfaceModifier::ConstructSurfaceEngine::perform()
{
	task()->setProgressText(tr("Constructing surface mesh"));

	if(_radius <= 0)
		throw Exception(tr("Radius parameter must be positive."));

	if(mesh().cell().volume3D() <= FLOATTYPE_EPSILON*FLOATTYPE_EPSILON*FLOATTYPE_EPSILON)
		throw Exception(tr("Simulation cell is degenerate."));

	double alpha = _radius * _radius;
	FloatType ghostLayerSize = _radius * FloatType(3);

	// Check if combination of radius parameter and simulation cell size is valid.
	for(size_t dim = 0; dim < 3; dim++) {
		if(mesh().cell().pbcFlags()[dim]) {
			int stencilCount = (int)ceil(ghostLayerSize / mesh().cell().matrix().column(dim).dot(mesh().cell().cellNormalVector(dim)));
			if(stencilCount > 1)
				throw Exception(tr("Cannot generate Delaunay tessellation. Simulation cell is too small, or radius parameter is too large."));
		}
	}

	// If there are too few particles, don't build Delaunay tessellation.
	// It is going to be invalid anyway.
	size_t numInputParticles = positions()->size();
	if(selection()) {
		numInputParticles = positions()->size() - std::count(selection()->constDataInt(), selection()->constDataInt() + selection()->size(), 0);
	}
	if(numInputParticles <= 3) {
		return;
	}

	// Algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	task()->beginProgressSubStepsWithWeights({ 20, 1, 6, 1 });

	// Generate Delaunay tessellation.
	DelaunayTessellation tessellation;
	if(!tessellation.generateTessellation(mesh().cell(), positions()->constDataPoint3(), positions()->size(), ghostLayerSize,
			selection() ? selection()->constDataInt() : nullptr, *task()))
		return;

	task()->nextProgressSubStep();

	// Determines the region a solid Delaunay cell belongs to.
	// We use this callback function to compute the total volume of the solid region.
	auto tetrahedronRegion = [this, &tessellation](DelaunayTessellation::CellHandle cell) {
		if(tessellation.isGhostCell(cell) == false) {
			Point3 p0 = tessellation.vertexPosition(tessellation.cellVertex(cell, 0));
			Vector3 ad = tessellation.vertexPosition(tessellation.cellVertex(cell, 1)) - p0;
			Vector3 bd = tessellation.vertexPosition(tessellation.cellVertex(cell, 2)) - p0;
			Vector3 cd = tessellation.vertexPosition(tessellation.cellVertex(cell, 3)) - p0;
			addSolidVolume(std::abs(ad.dot(cd.cross(bd))) / FloatType(6));
		}
		return 1;
	};

	//  This callback function is called for every surface facet created by the manifold construction helper.
	auto prepareMeshFace = [this](HalfEdgeMesh::face_index face, const std::array<size_t,3>& vertexIndices, const std::array<DelaunayTessellation::VertexHandle,3>& vertexHandles, DelaunayTessellation::CellHandle cell) {
		// Mark vertex atoms as belonging to the surface.
		if(surfaceParticleSelection()) {
			for(size_t vi : vertexIndices) {
				OVITO_ASSERT(vi < surfaceParticleSelection()->size());
				surfaceParticleSelection()->setInt(vi, 1);
			}
		}
	};

	// Create the empty region in the output mesh.
	_mesh.createRegion();
	OVITO_ASSERT(_mesh.regionCount() == 1);

	ManifoldConstructionHelper<false, false, true> manifoldConstructor(tessellation, _mesh, alpha, *positions());
	if(!manifoldConstructor.construct(tetrahedronRegion, *task(), prepareMeshFace))
		return;

	task()->nextProgressSubStep();

	// Make sure every mesh vertex is only part of one surface manifold.
	_mesh.makeManifold();

	task()->nextProgressSubStep();
	if(!_mesh.smoothMesh(_smoothingLevel, *task()))
		return;

	// Create the 'Surface area' region property.
	PropertyPtr surfaceAreaProperty = _mesh.createRegionProperty(SurfaceMeshRegions::SurfaceAreaProperty, true);

	// Compute surface area (total and per-region) by summing up the triangle face areas.
	for(HalfEdgeMesh::edge_index edge : _mesh.firstFaceEdges()) {
		if(task()->isCanceled()) return;
		const Vector3& e1 = mesh().edgeVector(edge);
		const Vector3& e2 = mesh().edgeVector(mesh().nextFaceEdge(edge));
		FloatType area = e1.cross(e2).length() / 2;
		addSurfaceArea(area);
		SurfaceMeshData::region_index region = mesh().faceRegion(mesh().adjacentFace(edge));
		surfaceAreaProperty->setFloat(region, surfaceAreaProperty->getFloat(region) + area);
	}

	task()->endProgressSubSteps();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ConstructSurfaceModifier::ConstructSurfaceEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ConstructSurfaceModifier* modifier = static_object_cast<ConstructSurfaceModifier>(modApp->modifier());

	// Create the output data object.
	SurfaceMesh* meshObj = state.createObject<SurfaceMesh>(QStringLiteral("surface"), modApp, tr("Surface"));
	mesh().transferTo(meshObj);
	meshObj->setDomain(state.getObject<SimulationCellObject>());
	meshObj->setVisElement(modifier->surfaceMeshVis());

	if(surfaceParticleSelection()) {
		ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
		particles->createProperty(surfaceParticleSelection());
	}

	state.addAttribute(QStringLiteral("ConstructSurfaceMesh.surface_area"), QVariant::fromValue(surfaceArea()), modApp);
	state.addAttribute(QStringLiteral("ConstructSurfaceMesh.solid_volume"), QVariant::fromValue(solidVolume()), modApp);

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Surface area: %1\nSolid volume: %2\nSimulation cell volume: %3\nSolid volume fraction: %4\nSurface area per solid volume: %5\nSurface area per total volume: %6")
			.arg(surfaceArea()).arg(solidVolume()).arg(totalVolume())
			.arg(solidVolume() / totalVolume()).arg(surfaceArea() / solidVolume()).arg(surfaceArea() / totalVolume())));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
