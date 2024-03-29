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

#include <ovito/particles/Particles.h>
#include <ovito/particles/util/NearestNeighborFinder.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "CentroSymmetryModifier.h"
#include <mwm_csp/mwm_csp.h>


namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(CentroSymmetryModifier);
DEFINE_PROPERTY_FIELD(CentroSymmetryModifier, numNeighbors);
DEFINE_PROPERTY_FIELD(CentroSymmetryModifier, mode);
DEFINE_PROPERTY_FIELD(CentroSymmetryModifier, onlySelectedParticles);
SET_PROPERTY_FIELD_LABEL(CentroSymmetryModifier, numNeighbors, "Number of neighbors");
SET_PROPERTY_FIELD_LABEL(CentroSymmetryModifier, mode, "Mode");
SET_PROPERTY_FIELD_LABEL(CentroSymmetryModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CentroSymmetryModifier, numNeighbors, IntegerParameterUnit, 2, CentroSymmetryModifier::MAX_CSP_NEIGHBORS);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CentroSymmetryModifier::CentroSymmetryModifier(ObjectCreationParams params) : AsynchronousModifier(params),
	_numNeighbors(12),
	_mode(ConventionalMode),
	_onlySelectedParticles(false)
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
Future<AsynchronousModifier::EnginePtr> CentroSymmetryModifier::createEngine(const ModifierEvaluationRequest& request, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

	if(numNeighbors() < 2)
		throwException(tr("The number of neighbors to take into account in the centrosymmetry calculation is invalid. It must be at least 2."));

	if(numNeighbors() > MAX_CSP_NEIGHBORS)
		throwException(tr("The number of neighbors to take into account in the centrosymmetry calculation is too large. Maximum number of neighbors is %1.").arg(MAX_CSP_NEIGHBORS));

	if(numNeighbors() % 2)
		throwException(tr("The number of neighbors to take into account in the centrosymmetry calculation must be a positive and even integer."));

	// Get particle selection.
	const PropertyObject* selectionProperty = onlySelectedParticles() ? particles->expectProperty(ParticlesObject::SelectionProperty) : nullptr;

	// Create an empty data table for the CSP value histogram to be computed.
	DataOORef<DataTable> histogram = DataOORef<DataTable>::create(dataset(), DataTable::Line, tr("CSP distribution"));
	histogram->setIdentifier(input.generateUniqueIdentifier<DataTable>(QStringLiteral("csp-centrosymmetry")));
	histogram->setDataSource(request.modApp());
	histogram->setAxisLabelX(tr("CSP"));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CentroSymmetryEngine>(request, particles, posProperty, selectionProperty, simCell, numNeighbors(), mode(), std::move(histogram));
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CentroSymmetryModifier::CentroSymmetryEngine::perform()
{
	setProgressText(tr("Computing centrosymmetry parameters"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(_nneighbors);
	if(!neighFinder.prepare(positions(), cell(), selection(), this))
		return;

	// Access output array.
	PropertyAccess<FloatType> cspArray(csp());

	// Perform analysis on each particle.
	ConstPropertyAccess<int> selectionData(selection());
	parallelFor(positions()->size(), *this, [&](size_t index) {
		if(!selectionData || selectionData[index])
			cspArray[index] = computeCSP(neighFinder, index, _mode);
		else
			cspArray[index] = 0.0;
	});
	if(isCanceled())
		return;

	// Determine histogram bin size based on maximum CSP value.
	const size_t numHistogramBins = 100;
	FloatType cspHistogramBinSize = (cspArray.size() != 0) ? (FloatType(1.01) * *boost::max_element(cspArray) / numHistogramBins) : 0;
	if(cspHistogramBinSize <= 0) cspHistogramBinSize = 1;
	
	// Perform binning of CSP values.
	PropertyAccessAndRef<qlonglong> histogramCounts = DataTable::OOClass().createUserProperty(dataset(), numHistogramBins, PropertyObject::Int64, 1, tr("Count"), DataBuffer::InitializeMemory);
	const int* sel = selectionData ? selectionData.begin() : nullptr;
	for(FloatType cspValue : cspArray) {
		OVITO_ASSERT(cspValue >= 0);
		if(!sel || *sel++) {
			int binIndex = cspValue / cspHistogramBinSize;
			if(binIndex < numHistogramBins)
				histogramCounts[binIndex]++;
		}
	}
	_histogram->setY(histogramCounts.take());
	_histogram->setIntervalStart(0);
	_histogram->setIntervalEnd(cspHistogramBinSize * numHistogramBins);

	// Release data that is no longer needed.
	selectionData.reset();
	_positions.reset();
	_selection.reset();
	_simCell.reset();
}

/******************************************************************************
* Computes the centrosymmetry parameter of a single particle.
******************************************************************************/
FloatType CentroSymmetryModifier::computeCSP(NearestNeighborFinder& neighFinder, size_t particleIndex, CSPMode mode)
{
	// Find k nearest neighbor of current atom.
	NearestNeighborFinder::Query<MAX_CSP_NEIGHBORS> neighQuery(neighFinder);
	neighQuery.findNeighbors(particleIndex);

	int numNN = neighQuery.results().size();

	FloatType csp = 0;
	if(mode == CentroSymmetryModifier::ConventionalMode) {
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
		csp = std::accumulate(pairs, pairs + (numNN/2), FloatType(0), std::plus<FloatType>());
	}
	else {
		// Make sure our own neighbor count limit is consistent with the limit defined in the mwm-csp module.
		OVITO_STATIC_ASSERT(MAX_CSP_NEIGHBORS <= MWM_CSP_MAX_POINTS);

		double P[MAX_CSP_NEIGHBORS][3];
		for(size_t i = 0; i < numNN; i++) {
			auto v = neighQuery.results()[i].delta;
			P[i][0] = (double)v.x();
			P[i][1] = (double)v.y();
			P[i][2] = (double)v.z();
		}

		csp = (FloatType)calculate_mwm_csp(numNN, P);
	}
	OVITO_ASSERT(std::isfinite(csp));

	return csp;
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CentroSymmetryModifier::CentroSymmetryEngine::applyResults(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
	if(_inputFingerprint.hasChanged(particles))
		request.modApp()->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	// Output per-particle CSP values.
	particles->createProperty(csp());

	// Output CSP distribution histogram.
	state.addObjectWithUniqueId<DataTable>(_histogram);
}

}	// End of namespace
