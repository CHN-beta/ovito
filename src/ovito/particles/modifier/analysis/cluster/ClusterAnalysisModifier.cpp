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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ClusterAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(ClusterAnalysisModifier);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, neighborMode);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, cutoff);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, sortBySize);
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, neighborMode, "Neighbor mode");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, cutoff, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, sortBySize, "Sort clusters by size");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ClusterAnalysisModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ClusterAnalysisModifier::ClusterAnalysisModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoff(3.2),
	_onlySelectedParticles(false),
	_sortBySize(false),
	_neighborMode(CutoffRange)
{
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ClusterAnalysisModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> ClusterAnalysisModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get the current particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get simulation cell.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();

	// Get particle selection.
	ConstPropertyPtr selectionProperty;
	if(onlySelectedParticles())
		selectionProperty = particles->expectProperty(ParticlesObject::SelectionProperty)->storage();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	if(neighborMode() == CutoffRange) {
		return std::make_shared<CutoffClusterAnalysisEngine>(particles, posProperty->storage(), inputCell->data(), sortBySize(), std::move(selectionProperty), cutoff());
	}
	else if(neighborMode() == Bonding) {
		particles->expectBonds()->verifyIntegrity();
		return std::make_shared<BondClusterAnalysisEngine>(particles, posProperty->storage(), inputCell->data(), sortBySize(), std::move(selectionProperty), particles->expectBondsTopology()->storage());
	}
	else {
		throwException(tr("Invalid cluster neighbor mode"));
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ClusterAnalysisModifier::ClusterAnalysisEngine::perform()
{
	task()->setProgressText(tr("Performing cluster analysis"));

	// Initialize.
	particleClusters()->fill<qlonglong>(-1);

	// Perform the actual clustering.
	doClustering();
	if(task()->isCanceled())
		return;

	// Sort clusters by size.
	if(_sortBySize && numClusters() != 0) {
		PropertyAccess<qlonglong> particleClusters(this->particleClusters());

		// Determine cluster sizes.
		std::vector<size_t> clusterSizes(numClusters() + 1, 0);
		for(auto id : particleClusters) {
			clusterSizes[id]++;
		}

		// Sort clusters by size.
		std::vector<size_t> mapping(numClusters() + 1);
		std::iota(mapping.begin(), mapping.end(), size_t(0));
		std::sort(mapping.begin() + 1, mapping.end(), [&clusterSizes](size_t a, size_t b) {
			return clusterSizes[a] > clusterSizes[b];
		});
		setLargestClusterSize(clusterSizes[mapping[1]]);
		clusterSizes.clear();
		clusterSizes.shrink_to_fit();

		// Remap cluster IDs.
		std::vector<size_t> inverseMapping(numClusters() + 1);
		for(size_t i = 0; i <= numClusters(); i++)
			inverseMapping[mapping[i]] = i;
		for(auto& id : particleClusters)
			id = inverseMapping[id];
	}
}

/******************************************************************************
* Performs the actual clustering algorithm.
******************************************************************************/
void ClusterAnalysisModifier::CutoffClusterAnalysisEngine::doClustering()
{
	// Prepare the neighbor finder.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(cutoff(), positions(), cell(), selection(), task().get()))
		return;

	size_t particleCount = positions()->size();
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);

	PropertyAccess<qlonglong> particleClusters(this->particleClusters());
	ConstPropertyAccess<int> selectionData(selection());

	std::deque<size_t> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selectionData && !selectionData[seedParticleIndex]) {
			particleClusters[seedParticleIndex] = 0;
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(particleClusters[seedParticleIndex] != -1)
			continue;

		// Start a new cluster.
		setNumClusters(numClusters() + 1);
		qlonglong cluster = numClusters();
		particleClusters[seedParticleIndex] = cluster;

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			task()->incrementProgressValue();
			if(task()->isCanceled())
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();
			for(CutoffNeighborFinder::Query neighQuery(neighborFinder, currentParticle); !neighQuery.atEnd(); neighQuery.next()) {
				size_t neighborIndex = neighQuery.current();
				if(particleClusters[neighborIndex] == -1) {
					particleClusters[neighborIndex] = cluster;
					toProcess.push_back(neighborIndex);
				}
			}
		}
		while(toProcess.empty() == false);
	}
}

/******************************************************************************
* Performs the actual clustering algorithm.
******************************************************************************/
void ClusterAnalysisModifier::BondClusterAnalysisEngine::doClustering()
{
	size_t particleCount = positions()->size();
	task()->setProgressValue(0);
	task()->setProgressMaximum(particleCount);

	// Prepare particle bond map.
	ParticleBondMap bondMap(bondTopology());

	PropertyAccess<qlonglong> particleClusters(this->particleClusters());
	ConstPropertyAccess<int> selectionData(this->selection());
	ConstPropertyAccess<ParticleIndexPair> bondTopology(this->bondTopology());

	std::deque<size_t> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selectionData && !selectionData[seedParticleIndex]) {
			particleClusters[seedParticleIndex] = 0;
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(particleClusters[seedParticleIndex] != -1)
			continue;

		// Start a new cluster.
		setNumClusters(numClusters() + 1);
		qlonglong cluster = numClusters();
		particleClusters[seedParticleIndex] = cluster;

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			task()->incrementProgressValue();
			if(task()->isCanceled())
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();

			// Iterate over all bonds of the current particle.
			for(size_t neighborBondIndex : bondMap.bondIndicesOfParticle(currentParticle)) {
				OVITO_ASSERT(bondTopology[neighborBondIndex][0] == currentParticle || bondTopology[neighborBondIndex][1] == currentParticle);
				size_t neighborIndex = bondTopology[neighborBondIndex][0];
				if(neighborIndex == currentParticle)
					neighborIndex = bondTopology[neighborBondIndex][1];
				if(neighborIndex >= particleCount)
					continue;
				if(particleClusters[neighborIndex] != -1)
					continue;
				if(selectionData && !selectionData[neighborIndex])
					continue;

				particleClusters[neighborIndex] = cluster;
				toProcess.push_back(neighborIndex);
			}
		}
		while(toProcess.empty() == false);
	}
}


/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ClusterAnalysisModifier::ClusterAnalysisEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ClusterAnalysisModifier* modifier = static_object_cast<ClusterAnalysisModifier>(modApp->modifier());
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	particles->createProperty(particleClusters());

	state.addAttribute(QStringLiteral("ClusterAnalysis.cluster_count"), QVariant::fromValue(numClusters()), modApp);
	if(modifier->sortBySize())
		state.addAttribute(QStringLiteral("ClusterAnalysis.largest_size"), QVariant::fromValue(largestClusterSize()), modApp);

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Found %n cluster(s).", "", numClusters())));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
