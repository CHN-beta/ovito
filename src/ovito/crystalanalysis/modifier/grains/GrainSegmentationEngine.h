////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
//  Copyright 2020 Peter Mahler Larsen
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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/modifier/analysis/ptm/PTMAlgorithm.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>
#include "DisjointSet.h"
#include "GrainSegmentationModifier.h"

namespace Ovito { namespace CrystalAnalysis {

/*
 * Computation engine of the GrainSegmentationModifier, which decomposes a polycrystalline microstructure into individual grains.
 */
class GrainSegmentationEngine1 : public AsynchronousModifier::Engine
{
public:
	class Graph
	{
	public:
		size_t next = 0;
		std::map<size_t, FloatType> wnode;
		std::map<size_t, std::map<size_t, FloatType>> adj;

		size_t num_nodes() const {
			return adj.size();
		}

		size_t next_node() const {
			return adj.begin()->first;
		}

		std::tuple<FloatType, size_t> nearest_neighbor(size_t a) const {
			FloatType dmin = std::numeric_limits<FloatType>::max();
			size_t vmin = std::numeric_limits<size_t>::max();

			OVITO_ASSERT(adj.find(a) != adj.end());
			for (const auto& x : adj.find(a)->second) {
				size_t v = x.first;
				FloatType weight = x.second;

				OVITO_ASSERT(v != a); // Graph has self loops.
				if(v == a)
					throw Exception("Graph has self loops");

				OVITO_ASSERT(wnode.find(v) != wnode.end());
				FloatType d = wnode.find(v)->second / weight;
				OVITO_ASSERT(!std::isnan(d));

				if (d < dmin) {
					dmin = d;
					vmin = v;
				}
				else if (d == dmin) {
					vmin = std::min(vmin, v);
				}
			}

			OVITO_ASSERT(wnode.find(a) != wnode.end());
			FloatType check = dmin * wnode.find(a)->second;
			OVITO_ASSERT(!std::isnan(check));

			return std::make_tuple(dmin * wnode.find(a)->second, vmin);
		}

		void add_node(size_t u) {
			next = u + 1;
			wnode[u] = 0;
		}

		void add_edge(size_t u, size_t v, FloatType w) {

			auto it_u = adj.find(u);
			if (it_u == adj.end()) {
				add_node(u);
				it_u = adj.emplace(u, std::map<size_t, FloatType>{{{v,w}}}).first;
			}
			else it_u->second[v] = w;

			auto it_v = adj.find(v);
			if (it_v == adj.end()) {
				add_node(v);
				it_v = adj.emplace(v, std::map<size_t, FloatType>{{{u,w}}}).first;
			}
			else it_v->second[u] = w;

			wnode[u] += w;
			wnode[v] += w;
		}

		void remove_node(size_t u) {

			for (auto const& x: adj[u]) {
				size_t v = x.first;
				adj[v].erase(u);
			}

			adj.erase(u);
			//wnode.erase(u);
		}

		size_t contract_edge(size_t a, size_t b) {

			if (adj[b].size() > adj[a].size()) {
				std::swap(a, b);
			}

			for (auto const& x: adj[b]) {
				size_t v = x.first;
				FloatType w = x.second;
				if (v == a) continue;

				(adj[a])[v] += w;
				(adj[v])[a] += w;
			}

			adj[a].erase(b);
			wnode[a] += wnode[b];
			remove_node(b);
			return a;
		}
	};

#ifndef Q_CC_MSVC
	/// The maximum number of neighbor atoms taken into account for orphan atom adoption.
	static constexpr int MAX_DISORDERED_NEIGHBORS = 8;
#else
	enum { MAX_DISORDERED_NEIGHBORS = 8 };
#endif

	/// Represents a single bond connecting two neighboring lattice atoms.
	struct NeighborBond {
		size_t a;
		size_t b;
		FloatType disorientation;
		FloatType length;
	};

	struct DendrogramNode {

		DendrogramNode() = default;
		DendrogramNode(size_t _a, size_t _b, FloatType _distance, FloatType _disorientation, size_t _size, Quaternion _orientation)
			: a(_a), b(_b), distance(_distance), disorientation(_disorientation), size(_size), orientation(_orientation) {}

		size_t a = 0;
		size_t b = 0;
		FloatType distance = std::numeric_limits<FloatType>::lowest();
		FloatType disorientation = std::numeric_limits<FloatType>::lowest();
		size_t size = 0;
        FloatType merge_size = 0;
		Quaternion orientation;
	};

	class InterfaceHandler
	{
	public:
		InterfaceHandler(ConstPropertyAccess<PTMAlgorithm::StructureType> structuresArray) {

			// Count structure types
			int structureCounts[PTMAlgorithm::NUM_STRUCTURE_TYPES] = {0};
			for (auto structureType: structuresArray) {
				structureCounts[(int)structureType]++;
			}

			parent_fcc = structureCounts[(size_t)PTMAlgorithm::FCC] >= structureCounts[(size_t)PTMAlgorithm::HCP];
			parent_dcub = structureCounts[(size_t)PTMAlgorithm::CUBIC_DIAMOND] >= structureCounts[(size_t)PTMAlgorithm::HEX_DIAMOND];

			// Set structure targets (i.e. which way a structure will flip)
			if (parent_fcc) {
				target[(size_t)PTMAlgorithm::HCP] = PTMAlgorithm::FCC;
			}
			else {
				target[(size_t)PTMAlgorithm::FCC] = PTMAlgorithm::HCP;
			}

			if (parent_dcub) {
				target[(size_t)PTMAlgorithm::HEX_DIAMOND] = PTMAlgorithm::CUBIC_DIAMOND;
			}
			else {
				target[(size_t)PTMAlgorithm::CUBIC_DIAMOND] = PTMAlgorithm::HEX_DIAMOND;
			}
		}

		PTMAlgorithm::StructureType parent_phase(PTMAlgorithm::StructureType defectStructureType) {
			return target[(size_t)defectStructureType];
		}

		bool reorder_bond(NeighborBond& bond, std::vector<PTMAlgorithm::StructureType>& _adjustedStructureTypes) {
			auto a = bond.a;
			auto b = bond.b;
			auto sa = _adjustedStructureTypes[a];
			auto sb = _adjustedStructureTypes[b];

			// We want ordering of (a, b) to be (parent phase, defect phase)
			bool flipped = false;
			if (sa == PTMAlgorithm::FCC && sb == PTMAlgorithm::HCP) {
				flipped |= !parent_fcc;
			}
			else if (sa == PTMAlgorithm::HCP && sb == PTMAlgorithm::FCC) {
				flipped |= parent_fcc;
			}
			else if (sa == PTMAlgorithm::CUBIC_DIAMOND && sb == PTMAlgorithm::HEX_DIAMOND) {
				flipped |= !parent_dcub;
			}
			else if (sa == PTMAlgorithm::HEX_DIAMOND && sb == PTMAlgorithm::CUBIC_DIAMOND) {
				flipped |= parent_dcub;
			}
			else {
				return false;
			}

			if (flipped) {
				std::swap(a, b);
			}

			bond.a = a;
			bond.b = b;
			return true;
		}

	private:
		bool parent_fcc;
		bool parent_dcub;
		PTMAlgorithm::StructureType target[PTMAlgorithm::NUM_STRUCTURE_TYPES];
	};

	/// Constructor.
	GrainSegmentationEngine1(
			const PipelineObject* dataSource, 
			ExecutionContext executionContext, 
			DataSet* dataset, 
			ParticleOrderingFingerprint fingerprint, 
			ConstPropertyPtr positions,
			ConstPropertyPtr structureProperty,
			ConstPropertyPtr orientationProperty,
			ConstPropertyPtr correspondenceProperty,
			const SimulationCellObject* simCell,
			GrainSegmentationModifier::MergeAlgorithm algorithmType,
			bool handleCoherentInterfaces,
			bool outputBonds);

	/// Performs the computation.
	virtual void perform() override;

	/// Injects the computed results into the data pipeline.
	virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// This method is called by the system whenever a parameter of the modifier changes.
	/// The method can be overriden by subclasses to indicate to the caller whether the engine object should be 
	/// discarded (false) or may be kept in the cache, because the computation results are not affected by the changing parameter (true). 
	virtual bool modifierChanged(const PropertyFieldEvent& event) override {

		// Avoid a recomputation if a parameters changes that does not affect this algorithm stage.
		if(event.field() == &PROPERTY_FIELD(GrainSegmentationModifier::colorParticlesByGrain)
				|| event.field() == &PROPERTY_FIELD(GrainSegmentationModifier::mergingThreshold) 
				|| event.field() == &PROPERTY_FIELD(GrainSegmentationModifier::minGrainAtomCount)
				|| event.field() == &PROPERTY_FIELD(GrainSegmentationModifier::orphanAdoption))
			return true;

		return AsynchronousModifier::Engine::modifierChanged(event);
	}

	/// Creates another engine that performs the next stage of the computation. 
	virtual std::shared_ptr<Engine> createContinuationEngine(ModifierApplication* modApp, const PipelineFlowState& input) override;

	/// Returns the global dataset object.
	DataSet* dataset() const { return _dataset; }

	/// Returns the property storage that contains the input particle positions.
	const ConstPropertyPtr& positions() const { return _positions; }

	/// Returns the simulation cell data.
	const DataOORef<const SimulationCellObject>& cell() const { return _simCell; }

	// Returns the log merge distances for the scatter plot
	const PropertyPtr& logMergeDistance() const { return _logMergeDistance; }

	// Returns the log merge sizes for the scatter plot
	const PropertyPtr& logMergeSize() const { return _logMergeSize; }

	/// Returns the per-particle structure types.
	const ConstPropertyPtr& structureTypes() const { return _structureTypes; }

	/// Returns the per-particle lattice orientations.
	const ConstPropertyPtr& orientations() const { return _orientations; }

	/// Returns the per-particle template correspondences.
	const ConstPropertyPtr& correspondences() const { return _correspondences; }

	/// Returns the adaptively determined merge threshold.
	FloatType suggestedMergingThreshold() const { return _suggestedMergingThreshold; }

private:

	/// Returns the list of bonds connecting neighboring lattice atoms.
	std::vector<NeighborBond>& neighborBonds() { return _neighborBonds; }

	/// Creates neighbor bonds from stored PTM data.
	bool createNeighborBonds();

	/// Rotates hexagonal atoms (HCP and hex-diamond) to an equivalent cubic orientation.
	bool rotateInterfaceAtoms();

	/// Calculates the disorientation angle for each graph edge (i.e. bond).
	bool computeDisorientationAngles();

	/// Builds grains by iterative region merging.
	bool determineMergeSequence();

	/// Computes the disorientation angle between two crystal clusters of the given lattice type. 
	/// Furthermore, the function computes the weighted average of the two cluster orientations. 
	/// The norm of the two input quaternions and the output quaternion represents the size of the clusters.
	static FloatType calculate_disorientation(int structureType, Quaternion& qa, const Quaternion& qb);

	// Algorithm types:
	bool minimum_spanning_tree_clustering(std::vector<Quaternion>& qsum, DisjointSet& uf);
	bool node_pair_sampling_clustering(Graph& graph, std::vector<Quaternion>& qsum);

	// Selects a threshold for Node Pair Sampling algorithm
	FloatType calculate_threshold_suggestion();

	// Determines if a bond is crystalline
	bool isCrystallineBond(const NeighborBond& bond)
	{
		auto a = _adjustedStructureTypes[bond.a];
		auto b = _adjustedStructureTypes[bond.b];

		if (a == PTMAlgorithm::OTHER) return false;
		if (b == PTMAlgorithm::OTHER) return false;
		if (a == b) return true;
		if (!_handleBoundaries) return false;

		if (a == PTMAlgorithm::FCC && b == PTMAlgorithm::HCP) return true;
		if (a == PTMAlgorithm::HCP && b == PTMAlgorithm::FCC) return true;
		if (a == PTMAlgorithm::CUBIC_DIAMOND && b == PTMAlgorithm::HEX_DIAMOND) return true;
		if (a == PTMAlgorithm::HEX_DIAMOND && b == PTMAlgorithm::CUBIC_DIAMOND) return true;
		return false;
	}

	bool interface_cubic_hex(NeighborBond& bond, InterfaceHandler& interfaceHandler,
						     Quaternion& output);

	// Converts a disorientation to an edge weight for Node Pair Sampling algorithm
	static FloatType calculateGraphWeight(FloatType disorientation) {
		// This is fairly arbitrary but it works well.
		return std::exp(-FloatType(1)/3 * disorientation * disorientation);
	}

	// TODO: remove this and replace with a lambda function if possible
	struct PriorityQueueCompare
	{
		bool operator()(const NeighborBond &a, const NeighborBond &b) const {return a.disorientation > b.disorientation;}
	};

private:

	const size_t _minPlotSize = 20;

	// A hardcoded cutoff, in degrees, used for skipping low-weight edges in Node Pair Sampling mode
	static constexpr FloatType _misorientationThreshold = 4.0;

	/// The global dataset object.
	DataSet* _dataset;

	// The linkage criterion used in the merge algorithm
	GrainSegmentationModifier::MergeAlgorithm _algorithmType;

	// The type of stacking fault handling
	bool _handleBoundaries;

	/// Controls the output of neighbor bonds to the data pipeline for visualization purposes.
	bool _outputBondsToPipeline;

	/// The number of input particles.
	size_t _numParticles;

	/// The coordinates of the input particles.
	ConstPropertyPtr _positions;

	/// The simulation cell geometry.
	DataOORef<const SimulationCellObject> _simCell;

	/// Used to detect changes in the input dataset that invalidate cached computation results.
	ParticleOrderingFingerprint _inputFingerprint;

	// The log merge distances
	PropertyPtr _logMergeDistance;

	// The merge sizes
	PropertyPtr _logMergeSize;

	/// The per-particle structure types.
	ConstPropertyPtr _structureTypes;

	/// The per-particle lattice orientations.
	ConstPropertyPtr _orientations;

	/// The per-particle structure types, adjusted for stacking fault handling.
	std::vector<PTMAlgorithm::StructureType> _adjustedStructureTypes;

	/// The per-particle lattice orientations.
	std::vector<Quaternion> _adjustedOrientations;

	/// The per-particle template correspondences.
	ConstPropertyPtr _correspondences;

	/// The bonds connecting neighboring lattice atoms.
	std::vector<NeighborBond> _neighborBonds;

	// Dendrogram as list of cluster merges.
	std::vector<DendrogramNode> _dendrogram;

	/// The adaptively computed merge threshold.
	FloatType _suggestedMergingThreshold = 0;

	// The graph used for the Node Pair Sampling methods
	Graph graph;

	friend class GrainSegmentationEngine2;
};

/*
 * Computation engine of the GrainSegmentationModifier, which decomposes a polycrystalline microstructure into individual grains.
 */
class GrainSegmentationEngine2 : public AsynchronousModifier::Engine
{
public:

	/// Constructor.
	GrainSegmentationEngine2(
			const PipelineObject* dataSource, 
			ExecutionContext executionContext, 
			DataSet* dataset, 
			std::shared_ptr<GrainSegmentationEngine1> engine1,
			FloatType mergingThreshold, 
			bool adoptOrphanAtoms, 
			size_t minGrainAtomCount) :
		Engine(dataSource, executionContext),
		_engine1(std::move(engine1)),
		_numParticles(_engine1->_numParticles),
		_mergingThreshold(mergingThreshold),
		_adoptOrphanAtoms(adoptOrphanAtoms),
		_minGrainAtomCount(minGrainAtomCount),
		_atomClusters(ParticlesObject::OOClass().createUserProperty(dataset, _numParticles, PropertyObject::Int64, 1, 0, QStringLiteral("Grain"), true)) {}
	
	/// Performs the computation.
	virtual void perform() override;

	/// Injects the computed results into the data pipeline.
	virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// This method is called by the system whenever a parameter of the modifier changes.
	/// The method can be overriden by subclasses to indicate to the caller whether the engine object should be 
	/// discarded (false) or may be kept in the cache, because the computation results are not affected by the changing parameter (true). 
	virtual bool modifierChanged(const PropertyFieldEvent& event) override {

		// Avoid a recomputation if a parameters changes that does not affect the algorithm's results.
		if(event.field() == &PROPERTY_FIELD(GrainSegmentationModifier::colorParticlesByGrain))
			return true; // Indicate that the stored results are not affected by the parameter change.

		return Engine::modifierChanged(event);
	}

	/// Returns the global dataset object.
	DataSet* dataset() const { return _engine1->dataset(); }

	/// Returns the array storing the cluster ID of each particle.
	const PropertyPtr& atomClusters() const { return _atomClusters; }


	struct PQNode {
		qlonglong cluster;
		size_t particleIndex;
		FloatType length;
	};

private:

	/// Merges any orphan atoms into the closest cluster.
	bool mergeOrphanAtoms();

	struct PQCompareLength
	{
		bool operator()(const PQNode &a, const PQNode &b) const {return a.length > b.length;}
	};

private:

	/// Pointer to the first algorithm stage.
	std::shared_ptr<GrainSegmentationEngine1> _engine1;

	/// The number of input particles.
	size_t _numParticles;

	/// The particle to cluster assignment.
	PropertyPtr _atomClusters;

	/// Counts the number of clusters
	size_t _numClusters = 1;

	// The output list of grain IDs.
	PropertyPtr _grainIds;

	// The output list of grain sizes.
	PropertyPtr _grainSizes;

	/// The output list of per-grain structure types.
	PropertyPtr _grainStructureTypes;

	/// The output list of colors assigned to grains.
	PropertyPtr _grainColors;

	/// The output list of mean grain orientations.
	PropertyPtr _grainOrientations;

	/// The user-defined merge threshold.
	FloatType _mergingThreshold;

	/// The minimum number of atoms a grain must have.
	size_t _minGrainAtomCount;

	/// Controls the adoption of orphan atoms after the grains have been formed.
	bool _adoptOrphanAtoms;
};

}	// End of namespace
}	// End of namespace
