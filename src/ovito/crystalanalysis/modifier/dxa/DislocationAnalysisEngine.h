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

#pragma once


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/util/DelaunayTessellation.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include "StructureAnalysis.h"
#include "ElasticMapping.h"
#include "InterfaceMesh.h"
#include "DislocationTracer.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the DislocationAnalysisModifier, which performs the actual dislocation analysis.
 */
class DislocationAnalysisEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	DislocationAnalysisEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell,
			int inputCrystalStructure, int maxTrialCircuitSize, int maxCircuitElongation,
			ConstPropertyPtr particleSelection,
			ConstPropertyPtr crystalClusters,
			std::vector<Matrix3> preferredCrystalOrientations,
			bool onlyPerfectDislocations, int defectMeshSmoothingLevel,
			int lineSmoothingLevel, FloatType linePointInterval,
			bool doOutputInterfaceMesh);

	/// This method is called by the system after the computation was successfully completed.
	virtual void cleanup() override {
		_structureAnalysis.reset();
		_tessellation.reset();
		_elasticMapping.reset();
		_interfaceMesh.reset();
		_dislocationTracer.reset();
		_crystalClusters.reset();
		StructureIdentificationEngine::cleanup();
	}

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Injects the computed results into the data pipeline.
	virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Returns the generated defect mesh.
	const SurfaceMeshData& defectMesh() const { return _defectMesh; }

	/// Returns the array of atom cluster IDs.
	const PropertyPtr& atomClusters() const { return _atomClusters; }

	/// Assigns the array of atom cluster IDs.
	void setAtomClusters(PropertyPtr prop) { _atomClusters = std::move(prop); }

	/// Returns the created cluster graph.
	const std::shared_ptr<ClusterGraph>& clusterGraph() { return _clusterGraph; }

	/// Sets the created cluster graph.
	void setClusterGraph(std::shared_ptr<ClusterGraph> graph) { _clusterGraph = std::move(graph); }

	/// Returns the defect interface.
	const HalfEdgeMeshPtr& outputInterfaceMesh() const { return _outputInterfaceMesh; }

	/// Returns the extracted dislocations.
	const std::shared_ptr<DislocationNetwork>& dislocationNetwork() const { return _dislocationNetwork; }

	/// Sets the extracted dislocations.
	void setDislocationNetwork(std::shared_ptr<DislocationNetwork> network) { _dislocationNetwork = std::move(network); }

	/// Returns the total volume of the input simulation cell.
	FloatType simCellVolume() const { return _simCellVolume; }

	/// Returns the computed interface mesh.
	const InterfaceMesh& interfaceMesh() const { return *_interfaceMesh; }

	/// Gives access to the elastic mapping computation engine.
	ElasticMapping& elasticMapping() { return *_elasticMapping; }

	/// Returns the input particle property that stores the cluster assignment of atoms.
	const ConstPropertyPtr& crystalClusters() const { return _crystalClusters; }

private:

	int _inputCrystalStructure;
	bool _onlyPerfectDislocations;
	int _defectMeshSmoothingLevel;
	int _lineSmoothingLevel;
	FloatType _linePointInterval;
	std::unique_ptr<StructureAnalysis> _structureAnalysis;
	std::unique_ptr<DelaunayTessellation> _tessellation;
	std::unique_ptr<ElasticMapping> _elasticMapping;
	std::unique_ptr<InterfaceMesh> _interfaceMesh;
	std::unique_ptr<DislocationTracer> _dislocationTracer;
	ConstPropertyPtr _crystalClusters;

	/// The defect mesh produced by the modifier.
	SurfaceMeshData _defectMesh;

	/// Indicates whether the engine should output the generated interface mesh to the pipeline for debugging purposes.
	bool _doOutputInterfaceMesh;

	/// This stores the interface mesh produced by the modifier for visualization purposes.
	HalfEdgeMeshPtr _outputInterfaceMesh;

	/// Stores the vertex coordinates of the interface output mesh.
	PropertyPtr _outputInterfaceMeshVerts;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	PropertyPtr _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	std::shared_ptr<ClusterGraph> _clusterGraph;

	/// This stores the cached dislocations computed by the modifier.
	std::shared_ptr<DislocationNetwork> _dislocationNetwork;

	/// The total volume of the input simulation cell.
	FloatType _simCellVolume;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace