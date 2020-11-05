////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "AcklandJonesModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(AcklandJonesModifier);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AcklandJonesModifier::AcklandJonesModifier(DataSet* dataset) : StructureIdentificationModifier(dataset)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void AcklandJonesModifier::loadUserDefaults(Application::ExecutionContext executionContext)
{
	// Create the structure types.
	createStructureType(OTHER, ParticleType::PredefinedStructureType::OTHER);
	createStructureType(FCC, ParticleType::PredefinedStructureType::FCC);
	createStructureType(HCP, ParticleType::PredefinedStructureType::HCP);
	createStructureType(BCC, ParticleType::PredefinedStructureType::BCC);
	createStructureType(ICO, ParticleType::PredefinedStructureType::ICO);
	
	StructureIdentificationModifier::loadUserDefaults(executionContext);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> AcklandJonesModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The Ackland-Jones ananlysis modifier does not support 2d simulation cells."));

	// Get particle selection.
	const PropertyObject* selectionProperty = onlySelectedParticles() ? particles->expectProperty(ParticlesObject::SelectionProperty) : nullptr;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<AcklandJonesAnalysisEngine>(dataset(), particles, posProperty, simCell, structureTypes(), selectionProperty);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void AcklandJonesModifier::AcklandJonesAnalysisEngine::perform()
{
	setProgressText(tr("Performing Ackland-Jones analysis"));

	// Prepare the neighbor finder.
	NearestNeighborFinder neighborFinder(14);
	if(!neighborFinder.prepare(positions(), cell(), selection(), this))
		return;

	PropertyAccess<int> output(structures());

	// Perform analysis on each particle.
	if(!selection()) {
		parallelFor(positions()->size(), *this, [&](size_t index) {
			output[index] = determineStructure(neighborFinder, index);
		});
	}
	else {
		ConstPropertyAccess<int> selectionData(selection());
		parallelFor(positions()->size(), *this, [&](size_t index) {
			// Skip particles that are not included in the analysis.
			if(selectionData[index])
				output[index] = determineStructure(neighborFinder, index);
			else
				output[index] = OTHER;
		});
	}

	// Release data that is no longer needed.
	releaseWorkingData();
}

/******************************************************************************
* Determines the coordination structure of a single particle using the
* bond-angle analysis method.
******************************************************************************/
AcklandJonesModifier::StructureType AcklandJonesModifier::AcklandJonesAnalysisEngine::determineStructure(NearestNeighborFinder& neighFinder, size_t particleIndex)
{
	// Find 14 nearest neighbors of current particle.
	NearestNeighborFinder::Query<14> neighborQuery(neighFinder);
	neighborQuery.findNeighbors(particleIndex);

	// Reject under-coordinated particles.
	if(neighborQuery.results().size() < 6)
		return OTHER;

	// Mean squared distance of 6 nearest neighbors.
	FloatType r0_sq = 0;
	for(int j = 0; j < 6; j++)
		r0_sq += neighborQuery.results()[j].distanceSq;
	r0_sq /= 6;

	// n0 near neighbors with: distsq<1.45*r0_sq
	// n1 near neighbors with: distsq<1.55*r0_sq
	FloatType n0_dist_sq = FloatType(1.45) * r0_sq;
	FloatType n1_dist_sq = FloatType(1.55) * r0_sq;
	int n0 = 0;
	for(auto n = neighborQuery.results().begin(); n != neighborQuery.results().end(); ++n, ++n0) {
		if(n->distanceSq > n0_dist_sq) break;
	}
	auto n0end = neighborQuery.results().begin() + n0;
	int n1 = n0;
	for(auto n = n0end; n != neighborQuery.results().end(); ++n, ++n1) {
		if(n->distanceSq >= n1_dist_sq) break;
	}

	// Evaluate all angles <(r_ij,rik) for all n0 particles with: distsq<1.45*r0_sq
	int chi[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	for(auto j = neighborQuery.results().begin(); j != n0end; ++j) {
		FloatType norm_j = sqrt(j->distanceSq);
		for(auto k = j + 1; k != n0end; ++k) {
			FloatType norm_k = sqrt(k->distanceSq);
			FloatType bond_angle = j->delta.dot(k->delta) / (norm_j*norm_k);

			// Build histogram for identifying the relevant peaks.
			if(bond_angle < FloatType(-0.945)) { chi[0]++; }
			else if(FloatType(-0.945) <= bond_angle && bond_angle < FloatType(-0.915)) { chi[1]++; }
			else if(FloatType(-0.915) <= bond_angle && bond_angle < FloatType(-0.755)) { chi[2]++; }
			else if(FloatType(-0.755) <= bond_angle && bond_angle < FloatType(-0.195)) { chi[3]++; }
			else if(FloatType(-0.195) <= bond_angle && bond_angle < FloatType(0.195)) { chi[4]++; }
			else if(FloatType(0.195) <= bond_angle && bond_angle < FloatType(0.245)) { chi[5]++; }
			else if(FloatType(0.245) <= bond_angle && bond_angle < FloatType(0.795)) { chi[6]++; }
			else if(FloatType(0.795) <= bond_angle) { chi[7]++; }
		}
	}

	// Calculate deviations from the different lattice structures.
	FloatType delta_bcc = FloatType(0.35) * chi[4] / (FloatType)(chi[5] + chi[6] - chi[4]);
	FloatType delta_cp = std::abs(FloatType(1) - (FloatType)chi[6] / 24);
	FloatType delta_fcc = FloatType(0.61) * (FloatType)(std::abs(chi[0] + chi[1] - 6) + chi[2]) / 6;
	FloatType delta_hcp = (FloatType)(std::abs(chi[0] - 3) + std::abs(chi[0] + chi[1] + chi[2] + chi[3] - 9)) / 12;

	// Identification of the local structure according to the reference.
	if(chi[0] == 7)       { delta_bcc = 0; }
	else if(chi[0] == 6)  { delta_fcc = 0; }
	else if(chi[0] <= 3)  { delta_hcp = 0; }

	if(chi[7] > 0) return OTHER;
	else if(chi[4] < 3) {
		if(!typeIdentificationEnabled(ICO) || n1 > 13 || n1 < 11) return OTHER;
		else return ICO;
	}
	else if(delta_bcc <= delta_cp) {
		if(!typeIdentificationEnabled(BCC) || n1 < 11) return OTHER;
		else return BCC;
	}
	else if(n1 > 12 || n1 < 11) return OTHER;
	else if(delta_fcc < delta_hcp) {
		if(typeIdentificationEnabled(FCC)) return FCC;
		else return OTHER;
	}
	else if(typeIdentificationEnabled(HCP)) return HCP;
	else return OTHER;
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void AcklandJonesModifier::AcklandJonesAnalysisEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	StructureIdentificationEngine::applyResults(time, modApp, state);

	// Also output structure type counts, which have been computed by the base class.
	state.addAttribute(QStringLiteral("AcklandJones.counts.OTHER"), QVariant::fromValue(getTypeCount(OTHER)), modApp);
	state.addAttribute(QStringLiteral("AcklandJones.counts.FCC"), QVariant::fromValue(getTypeCount(FCC)), modApp);
	state.addAttribute(QStringLiteral("AcklandJones.counts.HCP"), QVariant::fromValue(getTypeCount(HCP)), modApp);
	state.addAttribute(QStringLiteral("AcklandJones.counts.BCC"), QVariant::fromValue(getTypeCount(BCC)), modApp);
	state.addAttribute(QStringLiteral("AcklandJones.counts.ICO"), QVariant::fromValue(getTypeCount(ICO)), modApp);
}

}	// End of namespace
}	// End of namespace
