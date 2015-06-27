///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "ElasticMapping.h"
#include "CrystalPathFinder.h"
#include "DislocationTracer.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

// List of vertices that bound the six edges of a tetrahedron.
static const int edgeVertices[6][2] = {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};

/******************************************************************************
* Builds the list of edges in the tetrahedral tessellation.
******************************************************************************/
bool ElasticMapping::generateTessellationEdges(FutureInterfaceBase& progress)
{
	progress.setProgressRange(tessellation().number_of_primary_tetrahedra());

	// Generate list of tessellation edges.
	for(DelaunayTessellation::CellIterator cell = tessellation().begin_cells(); cell != tessellation().end_cells(); ++cell) {

		// Skip invalid cells (those not connecting four physical atoms) and ghost cells.
		if(cell->info().isGhost) continue;

		// Update progress indicator.
		if(!progress.setProgressValueIntermittent(cell->info().index))
			return false;

		// Create edge data structure for each of the six edges of the cell.
		for(int edgeIndex = 0; edgeIndex < 6; edgeIndex++) {
			int vertex1 = cell->vertex(edgeVertices[edgeIndex][0])->point().index();
			int vertex2 = cell->vertex(edgeVertices[edgeIndex][1])->point().index();
			if(vertex1 == vertex2)
				continue;
			Vector3 v = Point3(cell->vertex(edgeVertices[edgeIndex][0])->point()) - Point3(cell->vertex(edgeVertices[edgeIndex][1])->point());
			if(structureAnalysis().cell().isWrappedVector(v))
				continue;
			OVITO_ASSERT(vertex1 >= 0 && vertex2 >= 0);
			TessellationEdge* edge = findEdge(vertex1, vertex2);
			if(edge == nullptr) {
				// Create a new pair of edges.
				TessellationEdge* edge12 = _edgePool.construct(vertex1, vertex2);
				TessellationEdge* edge21 = _edgePool.construct(vertex2, vertex1);
				edge12->next = _vertexEdges[vertex1];
				_vertexEdges[vertex1] = edge12;
				edge21->next = _vertexEdges[vertex2];
				_vertexEdges[vertex2] = edge21;
				edge12->reverse = edge21;
				edge21->reverse = edge12;
				_edgeCount++;
			}
		}
	}

	qDebug() << "Number of tessellation edges:" << _edgeCount;

	return true;
}

/******************************************************************************
* Assigns each tessellation vertex to a cluster.
******************************************************************************/
bool ElasticMapping::assignVerticesToClusters(FutureInterfaceBase& progress)
{
	// Unknown runtime length.
	progress.setProgressRange(0);

	// Assign a cluster to each vertex of the tessellation, which will be used to express
	// reference vectors assigned to the edges leaving the vertex.

	// If an atoms is part of an atomic cluster, then the cluster is also assigned to corresponding tessellation vertex.
	for(size_t atomIndex = 0; atomIndex < _vertexClusters.size(); atomIndex++) {
		_vertexClusters[atomIndex] = structureAnalysis().atomCluster(atomIndex);
	}

	// Now try to assign a cluster to those vertices of the tessellation whose corresponding atom
	// is not part of a cluster. This is performed by repeatedly copying the cluster assignment
	// from an already assigned vertex to all its unassigned neighbors.
	bool notDone;
	do {
		if(progress.isCanceled())
			return false;

		notDone = false;
		for(TessellationEdge* firstEdge : _vertexEdges) {
			if(!firstEdge) continue;
			if(clusterOfVertex(firstEdge->vertex1)->id == 0) {
				for(TessellationEdge* e = firstEdge; e != nullptr; e = e->next) {
					OVITO_ASSERT(e->vertex1 == firstEdge->vertex1);
					if(clusterOfVertex(e->vertex2)->id != 0) {
						_vertexClusters[e->vertex1] = _vertexClusters[e->vertex2];
						notDone = true;
						break;
					}
				}
			}
		}
	}
	while(notDone);

	return true;
}

/******************************************************************************
* Determines the ideal vector corresponding to each edge of the tessellation.
******************************************************************************/
bool ElasticMapping::assignIdealVectorsToEdges(int crystalPathSteps, FutureInterfaceBase& progress)
{
	CrystalPathFinder pathFinder(_structureAnalysis, crystalPathSteps);

	// Try to assign a reference vector to the tessellation edges.
	progress.setProgressRange(_vertexEdges.size());
	int progressCounter = 0;
	for(TessellationEdge* firstEdge : _vertexEdges) {

		if(!progress.setProgressValueIntermittent(progressCounter++))
			return false;

		for(TessellationEdge* edge = firstEdge; edge != nullptr; edge = edge->next) {
			// Check if the reference vector of this edge has already been determined.
			if(edge->hasClusterVector()) continue;

			Cluster* cluster1 = clusterOfVertex(edge->vertex1);
			Cluster* cluster2 = clusterOfVertex(edge->vertex2);
			OVITO_ASSERT(cluster1 && cluster2);
			if(cluster1->id == 0 || cluster2->id == 0) continue;

			// Determine the ideal vector connecting the two atoms.
			boost::optional<ClusterVector> idealVector = pathFinder.findPath(edge->vertex1, edge->vertex2);
			if(!idealVector)
				continue;

			// Translate vector to the frame of the vertex cluster.
			Vector3 localVec;
			if(idealVector->cluster() == cluster1)
				localVec = idealVector->localVec();
			else {
				ClusterTransition* transition = clusterGraph().determineClusterTransition(idealVector->cluster(), cluster1);
				if(!transition)
					continue;
				localVec = transition->transform(idealVector->localVec());
			}

			// Assign the cluster transition to the edge.
			ClusterTransition* transition = clusterGraph().determineClusterTransition(cluster1, cluster2);
			// The two clusters may be part of two disconnected components of the cluster graph.
			if(!transition)
				continue;

			// Assign cluster vector to edge and its reverse edge.
			edge->assignClusterVector(localVec, transition);
		}
	}

	if(!reconstructIdealEdgeVectors(progress))
		return false;

#if 0
	_unassignedEdges = new BondsStorage();
	for(TessellationEdge* firstEdge : _vertexEdges) {
		for(TessellationEdge* edge = firstEdge; edge != nullptr; edge = edge->next) {
			if(edge->hasClusterVector()) continue;
			_unassignedEdges->push_back({ Vector_3<int8_t>::Zero(), (unsigned int)edge->vertex1, (unsigned int)edge->vertex2 });
		}
	}
#endif

	return true;
}

/******************************************************************************
* Tries to determine the ideal vectors of tessellation edges, which haven't
* been assigned one during the first phase.
******************************************************************************/
bool ElasticMapping::reconstructIdealEdgeVectors(FutureInterfaceBase& progress)
{
	auto vertexEdgeList = _vertexEdges.cbegin();
	for(size_t vertexIndex = 0; vertexIndex < _vertexEdges.size(); vertexIndex++, ++vertexEdgeList) {
		if(progress.isCanceled())
			return false;
		Cluster* cluster1 = clusterOfVertex(vertexIndex);
		if(cluster1->id == 0) continue;
		for(TessellationEdge* edge = *vertexEdgeList; edge != nullptr; edge = edge->next) {
			if(edge->hasClusterVector()) continue;

			Cluster* cluster2 = clusterOfVertex(edge->vertex2);
			if(cluster2->id == 0) continue;

			for(TessellationEdge* e1 = *vertexEdgeList; e1 != nullptr && !edge->hasClusterVector(); e1 = e1->next) {
				if(e1->hasClusterVector() == false) continue;
				OVITO_ASSERT(e1 != edge);
				for(TessellationEdge* e2 = _vertexEdges[e1->vertex2]; e2 != nullptr; e2 = e2->next) {
					if(e2->hasClusterVector() == false) continue;
					if(e2->vertex2 == edge->vertex2) {
						OVITO_ASSERT(e1->clusterTransition->cluster2 == e2->clusterTransition->cluster1);
						ClusterTransition* transition = clusterGraph().concatenateClusterTransitions(e1->clusterTransition, e2->clusterTransition);
						OVITO_ASSERT(transition != nullptr);
						Vector3 clusterVector = e1->clusterVector + e1->clusterTransition->reverseTransform(e2->clusterVector);
						edge->assignClusterVector(clusterVector, transition);
						OVITO_ASSERT(edge->clusterTransition->cluster1 == clusterOfVertex(e1->vertex1));
						OVITO_ASSERT(edge->clusterTransition->cluster2 == clusterOfVertex(e2->vertex2));
						break;
					}
				}
			}
		}
	}

	return true;
}


/******************************************************************************
* Determines whether the elastic mapping from the physical configuration
* of the crystal to the imaginary, stress-free configuration is compatible
* within the given tessellation cell. Returns false if the mapping is incompatible
* or cannot be determined at all.
******************************************************************************/
bool ElasticMapping::isElasticMappingCompatible(DelaunayTessellation::CellHandle cell) const
{
	// Must be a valid tessellation cell to determine the mapping.
	if(!tessellation().isValidCell(cell))
		return false;

	// Retrieve the six edges of the tetrahedron.
	// All must have a cluster vector assigned to them.
	TessellationEdge* edges[6];
	for(int edgeIndex = 0; edgeIndex < 6; edgeIndex++) {
		int vertex1 = cell->vertex(edgeVertices[edgeIndex][0])->point().index();
		int vertex2 = cell->vertex(edgeVertices[edgeIndex][1])->point().index();
		OVITO_ASSERT(vertex1 != -1 && vertex2 != -1);
		edges[edgeIndex] = findEdge(vertex1, vertex2);
		if(edges[edgeIndex] == nullptr || edges[edgeIndex]->hasClusterVector() == false) {
			return false;
		}
	}

	static const int circuits[4][3] = { {0,4,2}, {1,5,2}, {0,3,1}, {3,5,4} };

	// Perform the Burgers circuit test on each of the four faces of the tetrahedron.
	for(int face = 0; face < 4; face++) {
		Vector3 burgersVector = edges[circuits[face][0]]->clusterVector;
		burgersVector += edges[circuits[face][0]]->clusterTransition->reverseTransform(edges[circuits[face][1]]->clusterVector);
		burgersVector -= edges[circuits[face][2]]->clusterVector;
		if(!burgersVector.isZero(CA_LATTICE_VECTOR_EPSILON)) {
			return false;
		}
	}

	// Perform disclination test on each of the four faces.
	for(int face = 0; face < 4; face++) {
		ClusterTransition* t1 = edges[circuits[face][0]]->clusterTransition;
		ClusterTransition* t2 = edges[circuits[face][1]]->clusterTransition;
		ClusterTransition* t3 = edges[circuits[face][2]]->clusterTransition;
		if(!t1->isSelfTransition() || !t2->isSelfTransition() || !t3->isSelfTransition()) {
			Matrix3 frankRotation = t3->reverse->tm * t2->tm * t1->tm;
			if(!frankRotation.equals(Matrix3::Identity()))
				return false;
		}
	}

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
