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
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "CentroSymmetryModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CentroSymmetryModifier);
DEFINE_PROPERTY_FIELD(CentroSymmetryModifier, numNeighbors);
SET_PROPERTY_FIELD_LABEL(CentroSymmetryModifier, numNeighbors, "Number of neighbors");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CentroSymmetryModifier, numNeighbors, IntegerParameterUnit, 2, CentroSymmetryModifier::MAX_CSP_NEIGHBORS);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CentroSymmetryModifier::CentroSymmetryModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_numNeighbors(12)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CentroSymmetryModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CentroSymmetryModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

	if(numNeighbors() < 2)
		throwException(tr("The selected number of neighbors to take into account for the centrosymmetry calculation is invalid."));

	if(numNeighbors() % 2)
		throwException(tr("The number of neighbors to take into account for the centrosymmetry calculation must be a positive, even integer."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CentroSymmetryEngine>(particles, posProperty->storage(), simCell->data(), numNeighbors());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CentroSymmetryModifier::CentroSymmetryEngine::perform()
{
	task()->setProgressText(tr("Computing centrosymmetry parameters"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(_nneighbors);
	if(!neighFinder.prepare(*positions(), cell(), nullptr, task().get())) {
		return;
	}

	// Output storage.
	PropertyStorage& output = *csp();

	// Perform analysis on each particle.
	parallelFor(positions()->size(), *task(), [&neighFinder, &output](size_t index) {
		output.setFloat(index, computeCSP(neighFinder, index));
	});
}

/******************************************************************************
* Computes the centrosymmetry parameter of a single particle.
******************************************************************************/
FloatType CentroSymmetryModifier::computeCSP(NearestNeighborFinder& neighFinder, size_t particleIndex)
{
	// Find k nearest neighbor of current atom.
	NearestNeighborFinder::Query<MAX_CSP_NEIGHBORS> neighQuery(neighFinder);
	neighQuery.findNeighbors(particleIndex);

	int numNN = neighQuery.results().size();

    // R = Ri + Rj for each of npairs i,j pairs among numNN neighbors.
	FloatType pairs[MAX_CSP_NEIGHBORS*MAX_CSP_NEIGHBORS/2];
	FloatType* p = pairs;
	for(auto ij = neighQuery.results().begin(); ij != neighQuery.results().end(); ++ij) {
		for(auto ik = ij + 1; ik != neighQuery.results().end(); ++ik) {
			*p++ = (ik->delta + ij->delta).squaredLength();
		}
	}

    // Find NN/2 smallest pair distances from the list.
	std::partial_sort(pairs, pairs + (numNN/2), p);

    // Centrosymmetry = sum of numNN/2 smallest squared values.
    return std::accumulate(pairs, pairs + (numNN/2), FloatType(0), std::plus<FloatType>());
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CentroSymmetryModifier::CentroSymmetryEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	OVITO_ASSERT(csp()->size() == particles->elementCount());
	particles->createProperty(csp());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace