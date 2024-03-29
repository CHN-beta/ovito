////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 OVITO GmbH, Germany
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
#include <ovito/core/utilities/MemoryPool.h>
#include "Cluster.h"

namespace Ovito::CrystalAnalysis {

/**
 * This class stores the graph of clusters.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ClusterGraph
{
public:

	/// Default constructor. Creates an empty cluster graph.
	ClusterGraph();

	/// Copy constructor. Creates copy of an existing cluster graph.
	ClusterGraph(const ClusterGraph& other);

	/// Returns the list of nodes in the graph.
	const std::vector<Cluster*>& clusters() const { return _clusters; }

	/// Returns the list of directed edges in the graph.
	const std::vector<ClusterTransition*>& clusterTransitions() const { return _clusterTransitions; }

	/// Inserts a new node into the graph.
	Cluster* createCluster(int structureType, int id = -1);

	/// Looks up the cluster with the given ID.
	Cluster* findCluster(int id) const;

	/// Creates a new cluster transition between two clusters A and B.
	/// This will create a new pair of directed edges in the cluster graph unless a transition with same transformation matrix already exists.
	/// The reverse transition B->A will also be created automatically.
	ClusterTransition* createClusterTransition(Cluster* clusterA, Cluster* clusterB, const Matrix3& tm, int distance = 1);

	/// Determines the transformation matrix that transforms vectors from cluster A to cluster B.
	/// For this, the cluster graph is searched for the shortest path connecting the two cluster nodes.
	/// If the two clusters are part of different disconnected components of the graph, then NULL is returned.
	/// Once a new transition between A and B has been found, it is cached by creating a new edge in the graph between
	/// the clusters A and B. Future queries for the same pair can then be answered efficiently.
	ClusterTransition* determineClusterTransition(Cluster* clusterA, Cluster* clusterB);

	/// Creates and returns the self-transition for a cluster (or returns the existing one).
	ClusterTransition* createSelfTransition(Cluster* cluster);

	/// Returns the concatenation of two cluster transitions (A->B->C ->  A->C).
	ClusterTransition* concatenateClusterTransitions(ClusterTransition* tAB, ClusterTransition* tBC);

private:

	/// The list of clusters (graph nodes).
	std::vector<Cluster*> _clusters;

	/// Map from cluster IDs to clusters. This is used for fast lookup of clusters by the findCluster() method.
	std::map<int, Cluster*> _clusterMap;

	/// The list of transitions between clusters. This list doesn't contain self-transitions.
	std::vector<ClusterTransition*> _clusterTransitions;

	/// Memory pool for clusters.
	MemoryPool<Cluster> _clusterPool;

	/// Memory pool for cluster transitions.
	MemoryPool<ClusterTransition> _clusterTransitionPool;

	/// Cached list of cluster pairs which are known to be non-connected.
	std::set<std::pair<Cluster*, Cluster*>> _disconnectedClusters;

	/// Limits the maximum number of (original) transitions between two clusters
	/// when creating a direct transition between them.
	int _maximumClusterDistance;
};

/// Typically, cluster graphs are shallow copied. That's why we use a shared_ptr to hold on to them.
using ClusterGraphPtr = std::shared_ptr<ClusterGraph>;

}	// End of namespace
