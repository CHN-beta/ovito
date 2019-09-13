///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "CoordinationPolyhedraModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(CoordinationPolyhedraModifier);
DEFINE_REFERENCE_FIELD(CoordinationPolyhedraModifier, surfaceMeshVis);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationPolyhedraModifier::CoordinationPolyhedraModifier(DataSet* dataset) : AsynchronousModifier(dataset)
{
	// Create the vis element for rendering the polyhedra generated by the modifier.
	setSurfaceMeshVis(new SurfaceMeshVis(dataset));
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
Future<AsynchronousModifier::ComputeEnginePtr> CoordinationPolyhedraModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	const PropertyObject* selectionProperty = particles->getProperty(ParticlesObject::SelectionProperty);
	const PropertyObject* topologyProperty = particles->expectBondsTopology();
	const PropertyObject* bondPeriodicImagesProperty = particles->bonds()->getProperty(BondsObject::PeriodicImageProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

	if(!selectionProperty)
		throwException(tr("Please select particles first for which coordination polyhedra should be generated."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputePolyhedraEngine>(
			posProperty->storage(),
			selectionProperty->storage(),
			typeProperty ? typeProperty->storage() : nullptr,
			identifierProperty ? identifierProperty->storage() : nullptr,
			topologyProperty->storage(),
			bondPeriodicImagesProperty ? bondPeriodicImagesProperty->storage() : nullptr,
			simCell->data());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::perform()
{
	task()->setProgressText(tr("Generating coordination polyhedra"));

	// Create the exterior spatial region.
	mesh().createRegion();
	OVITO_ASSERT(mesh().regionCount() == 1);

	// Create the "Region" face property.
	mesh().createFaceProperty(SurfaceMeshFaces::RegionProperty);

	// Determine number of selected particles.
	size_t npoly = std::count_if(_selection->constDataInt(), _selection->constDataInt() + _selection->size(), [](int s) { return s != 0; });
	task()->setProgressMaximum(npoly);

	ParticleBondMap bondMap(_bondTopology, _bondPeriodicImages);

	for(size_t i = 0; i < _positions->size(); i++) {
		if(_selection->getInt(i) == 0) continue;

		// Collect the bonds that are part of the coordination polyhedron.
		std::vector<Point3> bondVectors;
		const Point3& p1 = _positions->getPoint3(i);
		for(Bond bond : bondMap.bondsOfParticle(i)) {
			if(bond.index2 < _positions->size()) {
				Vector3 delta = _positions->getPoint3(bond.index2) - p1;
				if(bond.pbcShift.x()) delta += cell().matrix().column(0) * (FloatType)bond.pbcShift.x();
				if(bond.pbcShift.y()) delta += cell().matrix().column(1) * (FloatType)bond.pbcShift.y();
				if(bond.pbcShift.z()) delta += cell().matrix().column(2) * (FloatType)bond.pbcShift.z();
				bondVectors.push_back(p1 + delta);
			}
		}

		// Also include the central particle in the vertex list.
		bondVectors.push_back(p1);

		// Construct the polyhedron (i.e. convex hull) from the bond vectors.
		mesh().constructConvexHull(std::move(bondVectors));
	
		if(!task()->incrementProgressValue())
			return;
	}

	// Create the "Center particle" region property, which indicates the ID of the particle that is at the center of each coordination polyhedron.
	PropertyPtr centerProperty = std::make_shared<PropertyStorage>(mesh().regionCount(), PropertyStorage::Int64, 1, 0, QStringLiteral("Center particle"), false);
	auto centerParticle = centerProperty->dataInt64();
	*centerParticle++ = 0;
	for(size_t i = 0; i < _positions->size(); i++) {
		if(_selection->getInt(i) == 0) continue;
		if(_particleIdentifiers)
			*centerParticle++ = _particleIdentifiers->getInt64(i);
		else
			*centerParticle++ = i;
	}
	mesh().addRegionProperty(std::move(centerProperty));
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	CoordinationPolyhedraModifier* modifier = static_object_cast<CoordinationPolyhedraModifier>(modApp->modifier());

	// Create the output data object.
	SurfaceMesh* meshObj = state.createObject<SurfaceMesh>(QStringLiteral("coord-polyhedra"), modApp, tr("Coordination polyhedra"));
	mesh().transferTo(meshObj);
	meshObj->setDomain(state.getObject<SimulationCellObject>());
	meshObj->setVisElement(modifier->surfaceMeshVis());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
