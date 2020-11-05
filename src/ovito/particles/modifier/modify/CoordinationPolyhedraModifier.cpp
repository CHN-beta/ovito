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

#include <ovito/particles/Particles.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "CoordinationPolyhedraModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(CoordinationPolyhedraModifier);
DEFINE_REFERENCE_FIELD(CoordinationPolyhedraModifier, surfaceMeshVis);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationPolyhedraModifier::CoordinationPolyhedraModifier(DataSet* dataset) : AsynchronousModifier(dataset)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void CoordinationPolyhedraModifier::loadUserDefaults(Application::ExecutionContext executionContext)
{
	AsynchronousModifier::loadUserDefaults(executionContext);

	// Create the vis element for rendering the polyhedra generated by the modifier.
	setSurfaceMeshVis(OORef<SurfaceMeshVis>::create(dataset(), executionContext));
	surfaceMeshVis()->setShowCap(false);
	surfaceMeshVis()->setSmoothShading(false);
	surfaceMeshVis()->setSurfaceTransparency(FloatType(0.25));
	surfaceMeshVis()->setObjectTitle(tr("Polyhedra"));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CoordinationPolyhedraModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
		return particles->bonds() != nullptr;
	}
	return false;
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> CoordinationPolyhedraModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	const PropertyObject* selectionProperty = particles->getProperty(ParticlesObject::SelectionProperty);
	particles->expectBonds()->verifyIntegrity();
	const PropertyObject* topologyProperty = particles->expectBondsTopology();
	const PropertyObject* bondPeriodicImagesProperty = particles->bonds()->getProperty(BondsObject::PeriodicImageProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

	if(!selectionProperty)
		throwException(tr("Please select particles first for which coordination polyhedra should be generated."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputePolyhedraEngine>(
			dataset(),
			posProperty,
			selectionProperty,
			typeProperty,
			identifierProperty,
			topologyProperty,
			bondPeriodicImagesProperty,
			simCell);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::perform()
{
	setProgressText(tr("Generating coordination polyhedra"));

	// Create the "Region" face property.
	mesh().createFaceProperty(SurfaceMeshFaces::RegionProperty);

	// Determine number of selected particles.
	ConstPropertyAccess<int> selectionArray(_selection);
	size_t npoly = boost::count_if(selectionArray, [](int s) { return s != 0; });
	setProgressMaximum(npoly);

	ParticleBondMap bondMap(_bondTopology, _bondPeriodicImages);

	ConstPropertyAccess<Point3> positionsArray(_positions);
	for(size_t i = 0; i < positionsArray.size(); i++) {
		if(selectionArray[i] == 0) continue;

		// Collect the bonds that are part of the coordination polyhedron.
		std::vector<Point3> bondVectors;
		const Point3& p1 = positionsArray[i];
		for(BondWithIndex bond : bondMap.bondsOfParticle(i)) {
			if(bond.index2 < _positions->size()) {
				Vector3 delta = positionsArray[bond.index2] - p1;
				if(bond.pbcShift.x()) delta += cell()->matrix().column(0) * (FloatType)bond.pbcShift.x();
				if(bond.pbcShift.y()) delta += cell()->matrix().column(1) * (FloatType)bond.pbcShift.y();
				if(bond.pbcShift.z()) delta += cell()->matrix().column(2) * (FloatType)bond.pbcShift.z();
				bondVectors.push_back(p1 + delta);
			}
		}

		// Also include the central particle in the vertex list.
		bondVectors.push_back(p1);

		// Construct the polyhedron (i.e. convex hull) from the bond vectors.
		mesh().constructConvexHull(std::move(bondVectors));

		if(!incrementProgressValue())
			return;
	}

	// Create the "Center particle" region property, which indicates the ID of the particle that is at the center of each coordination polyhedron.
	PropertyAccess<qlonglong> centerProperty = mesh().createRegionProperty(PropertyObject::Int64, 1, 0, QStringLiteral("Center Particle"), false);
	ConstPropertyAccess<qlonglong> particleIdentifiersArray(_particleIdentifiers);
	auto centerParticle = centerProperty.begin();
	for(size_t i = 0; i < positionsArray.size(); i++) {
		if(selectionArray[i] == 0) continue;
		if(particleIdentifiersArray)
			*centerParticle++ = particleIdentifiersArray[i];
		else
			*centerParticle++ = i;
	}
	OVITO_ASSERT(centerParticle == centerProperty.end());

	// Release data that is no longer needed.
	_positions.reset();
	_selection.reset();
	_particleTypes.reset();
	_particleIdentifiers.reset();
	_bondTopology.reset();
	_bondPeriodicImages.reset();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	CoordinationPolyhedraModifier* modifier = static_object_cast<CoordinationPolyhedraModifier>(modApp->modifier());

	// Create the output data object.
	SurfaceMesh* meshObj = state.createObject<SurfaceMesh>(QStringLiteral("coord-polyhedra"), modApp, Application::ExecutionContext::Scripting, tr("Coordination polyhedra"));
	mesh().transferTo(meshObj);
	meshObj->setDomain(state.getObject<SimulationCellObject>());
	meshObj->setVisElement(modifier->surfaceMeshVis());
}

}	// End of namespace
}	// End of namespace
