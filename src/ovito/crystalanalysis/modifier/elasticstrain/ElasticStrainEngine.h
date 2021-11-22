////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito::CrystalAnalysis {

/*
 * Computation engine of the ElasticStrainModifier, which performs the actual strain tensor calculation.
 */
class ElasticStrainEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	ElasticStrainEngine(const PipelineObject* dataSource, ExecutionContext executionContext, DataSet* dataset, ParticleOrderingFingerprint fingerprint,
			ConstPropertyPtr positions, const SimulationCellObject* simCell,
			int inputCrystalStructure, std::vector<Matrix3> preferredCrystalOrientations,
			bool calculateDeformationGradients, bool calculateStrainTensors,
			FloatType latticeConstant, FloatType caRatio, bool pushStrainTensorsForward);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Injects the computed results into the data pipeline.
	virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Returns the array of atom cluster IDs.
	const PropertyPtr& atomClusters() const { return _atomClusters; }

	/// Assigns the array of atom cluster IDs.
	void setAtomClusters(PropertyPtr prop) { _atomClusters = std::move(prop); }

	/// Returns the created cluster graph.
	const std::shared_ptr<ClusterGraph>& clusterGraph() { return _clusterGraph; }

	/// Returns the property storage that contains the computed per-particle volumetric strain values.
	const PropertyPtr& volumetricStrains() const { return _volumetricStrains; }

	/// Returns the property storage that contains the computed per-particle strain tensors.
	const PropertyPtr& strainTensors() const { return _strainTensors; }

	/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
	const PropertyPtr& deformationGradients() const { return _deformationGradients; }

private:

	const int _inputCrystalStructure;
	FloatType _latticeConstant;
	FloatType _axialScaling;
	const bool _pushStrainTensorsForward;
	std::unique_ptr<StructureAnalysis> _structureAnalysis;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	PropertyPtr _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	const std::shared_ptr<ClusterGraph> _clusterGraph;

	/// This stores the cached results of the modifier.
	const PropertyPtr _volumetricStrains;

	/// This stores the cached results of the modifier.
	const PropertyPtr _strainTensors;

	/// This stores the cached results of the modifier.
	const PropertyPtr _deformationGradients;
};

}	// End of namespace
