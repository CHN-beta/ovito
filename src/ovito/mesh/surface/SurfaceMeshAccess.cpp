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

#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/mesh/TriMesh.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "SurfaceMeshAccess.h"
#include "SurfaceMesh.h"

namespace Ovito::Mesh {

constexpr SurfaceMeshAccess::size_type SurfaceMeshAccess::InvalidIndex;

/******************************************************************************
* Constructor that takes an existing SurfaceMesh object.
******************************************************************************/
SurfaceMeshAccess::SurfaceMeshAccess(const SurfaceMesh* mesh) : 
	_mesh(mesh),
	_topology(mesh ? mesh->topology() : nullptr),
	_vertices(mesh ? mesh->vertices() : nullptr),
	_faces(mesh ? mesh->faces() : nullptr),
	_regions(mesh ? mesh->regions() : nullptr)
{
}

/******************************************************************************
* Releases the current mesh from this accessor and loads a new one.
******************************************************************************/
OORef<const SurfaceMesh> SurfaceMeshAccess::reset(const SurfaceMesh* newMesh) noexcept
{
	OVITO_ASSERT(newMesh == nullptr || newMesh != this->mesh());
	if(_mesh) {

		// Release the topology sub-object and write it back to the parent SurfaceMesh.
		auto topology = _topology.take();
		if(topology != mesh()->topology()) 
			mutableMesh()->setTopology(std::move(topology));

		// Release the sub-object property containers and write them back to the parent SurfaceMesh.
		auto vertices = static_object_cast<const SurfaceMeshVertices>(_vertices.take());
		if(vertices != _mesh->vertices()) 
			mutableMesh()->setVertices(std::move(vertices));

		auto faces = static_object_cast<const SurfaceMeshFaces>(_faces.take());
		if(faces != _mesh->faces()) 
			mutableMesh()->setFaces(std::move(faces));

		auto regions = static_object_cast<const SurfaceMeshRegions>(_regions.take());
		if(regions != _mesh->regions()) 
			mutableMesh()->setRegions(std::move(regions));
	}
	OORef<const SurfaceMesh> oldMesh = _mesh.take();

	_mesh.reset(newMesh);
	_topology.reset(newMesh ? newMesh->topology() : nullptr);
	_vertices.reset(newMesh ? newMesh->vertices() : nullptr);
	_faces.reset(newMesh ? newMesh->faces() : nullptr);
	_regions.reset(newMesh ? newMesh->regions() : nullptr);

	return oldMesh;
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SurfaceMeshAccess::smoothMesh(int numIterations, Task& task, FloatType k_PB, FloatType lambda)
{
	// This is the implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

    // Performs one iteration of the smoothing algorithm.
    auto smoothMeshIteration = [this](FloatType prefactor) {

        // Compute displacement for each vertex.
        std::vector<Vector3> displacements(vertexCount());
        parallelFor(vertexCount(), [this, &displacements, prefactor](vertex_index vertex) {
            Vector3 d = Vector3::Zero();

            // Go in positive direction around vertex, facet by facet.
            edge_index currentEdge = firstVertexEdge(vertex);
            if(currentEdge != InvalidIndex) {
                int numManifoldEdges = 0;
                do {
                    OVITO_ASSERT(currentEdge != InvalidIndex);
                    OVITO_ASSERT(adjacentFace(currentEdge) != InvalidIndex);
                    d += edgeVector(currentEdge);
                    numManifoldEdges++;
                    currentEdge = oppositeEdge(prevFaceEdge(currentEdge));
                }
                while(currentEdge != firstVertexEdge(vertex));
                d *= (prefactor / numManifoldEdges);
            }

            displacements[vertex] = d;
        });

        // Apply computed displacements.
        auto d = displacements.cbegin();
        for(Point3& vertex : mutableVertexPositions())
            vertex += *d++;
    };

	FloatType mu = FloatType(1) / (k_PB - FloatType(1)/lambda);
	task.setProgressMaximum(numIterations);

	for(int iteration = 0; iteration < numIterations; iteration++) {
		if(!task.setProgressValue(iteration))
			return false;
		smoothMeshIteration(lambda);
		smoothMeshIteration(mu);
	}

	return !task.isCanceled();
}

/******************************************************************************
* Determines which spatial region contains the given point in space.
*
* Algorithm:
*
* J. Andreas Baerentzen and Henrik Aanaes:
* Signed Distance Computation Using the Angle Weighted Pseudonormal
* IEEE Transactions on Visualization and Computer Graphics 11 (2005), Page 243
******************************************************************************/
std::optional<std::pair<SurfaceMeshAccess::region_index, FloatType>> SurfaceMeshAccess::locatePoint(const Point3& location, FloatType epsilon, const boost::dynamic_bitset<>& faceSubset) const
{
	// Determine which vertex is closest to the test point.
	FloatType closestDistanceSq = FLOATTYPE_MAX;
	vertex_index closestVertex = InvalidIndex;
	Vector3 closestNormal, closestVector;
	region_index closestRegion = spaceFillingRegion();
    size_type vcount = vertexCount();
	for(vertex_index vindex = 0; vindex < vcount; vindex++) {
		// Determine the first adjacent edge which has an adjacent face.
		edge_index firstEdge = firstVertexEdge(vindex);
		if(!faceSubset.empty()) {
			while(firstEdge != InvalidIndex && !faceSubset[adjacentFace(firstEdge)])
				firstEdge = nextVertexEdge(firstEdge);
		}
		if(firstEdge == InvalidIndex) continue;

		// Compute distance from query point to vertex.
		Vector3 r = wrapVector(vertexPosition(vindex) - location);
		FloatType distSq = r.squaredLength();
		if(distSq < closestDistanceSq) {
			closestDistanceSq = distSq;
			closestVertex = vindex;
			closestVector = r;
		}
	}

	// If the surface is degenerate, any point is inside the space-filling region.
	if(closestVertex == InvalidIndex)
		return std::make_pair(spaceFillingRegion(), closestDistanceSq);

	// Check if any edge is closer to the test point than the closest vertex.
	size_type edgeCount = this->edgeCount();
	for(edge_index edge = 0; edge < edgeCount; edge++) {
		if(!faceSubset.empty() && !faceSubset[adjacentFace(edge)]) continue;
		OVITO_ASSERT_MSG(hasOppositeEdge(edge), "SurfaceMeshAccess::locatePoint()", "Surface mesh is not fully closed. This should not happen.");
		const Point3& p1 = vertexPosition(vertex1(edge));
		const Point3& p2 = vertexPosition(vertex2(edge));
		Vector3 edgeDir = wrapVector(p2 - p1);
		Vector3 r = wrapVector(p1 - location);
		FloatType edgeLength = edgeDir.length();
		if(edgeLength <= FLOATTYPE_EPSILON) continue;
		edgeDir /= edgeLength;
		FloatType d = -edgeDir.dot(r);
		if(d <= 0 || d >= edgeLength) continue;
		Vector3 c = r + edgeDir * d;
		FloatType distSq = c.squaredLength();
		if(distSq < closestDistanceSq) {

			// Compute pseudo normal of edge by averaging the normal vectors of the two adjacent faces.
			const Point3& p1a = vertexPosition(vertex2(nextFaceEdge(edge)));
			const Point3& p1b = vertexPosition(vertex2(nextFaceEdge(oppositeEdge(edge))));
			Vector3 e1 = wrapVector(p1a - p1);
			Vector3 e2 = wrapVector(p1b - p1);
			Vector3 pseudoNormal = edgeDir.cross(e1).safelyNormalized() + e2.cross(edgeDir).safelyNormalized();

			// In case the manifold is two-sided, skip edge if pseudo-normal is facing toward the the query point.
			if(pseudoNormal.dot(c) > -epsilon || !hasOppositeFace(adjacentFace(edge))) {
				closestDistanceSq = distSq;
				closestVertex = InvalidIndex;
				closestVector = c;
				closestNormal = pseudoNormal;
				closestRegion = hasFaceRegions() ? faceRegion(adjacentFace(edge)) : 0;
			}
		}
	}

	// Check if any facet is closer to the test point than the closest vertex and the closest edge.
	size_type faceCount = this->faceCount();
	for(face_index face = 0; face < faceCount; face++) {
		if(!faceSubset.empty() && !faceSubset[face]) continue;
		edge_index edge1 = firstFaceEdge(face);
		edge_index edge2 = nextFaceEdge(edge1);
		const Point3& p1 = vertexPosition(vertex1(edge1));
		const Point3& p2 = vertexPosition(vertex2(edge1));
		const Point3& p3 = vertexPosition(vertex2(edge2));
		Vector3 edgeVectors[3];
		edgeVectors[0] = wrapVector(p2 - p1);
		edgeVectors[1] = wrapVector(p3 - p2);
		Vector3 r = wrapVector(p1 - location);
		edgeVectors[2] = -edgeVectors[1] - edgeVectors[0];

		// Compute face normal.
		Vector3 normal = edgeVectors[0].cross(edgeVectors[1]);

		// Determine whether the projection of the query point is inside the face's boundaries.
		bool isInsideTriangle = true;
		Vector3 vertexVector = r;
		for(size_t v = 0; v < 3; v++) {
			if(vertexVector.dot(normal.cross(edgeVectors[v])) >= 0.0) {
				isInsideTriangle = false;
				break;
			}
			vertexVector += edgeVectors[v];
		}

		if(isInsideTriangle) {
			FloatType normalLengthSq = normal.squaredLength();
			if(std::abs(normalLengthSq) <= FLOATTYPE_EPSILON) continue;
			normal /= sqrt(normalLengthSq);
			FloatType planeDist = normal.dot(r);
			// In case the manifold is two-sided, skip face if it is facing toward the query point.
			if(planeDist > -epsilon || !hasOppositeFace(face)) {
				if(planeDist * planeDist < closestDistanceSq) {
					closestDistanceSq = planeDist * planeDist;
					closestVector = normal * planeDist;
					closestVertex = InvalidIndex;
					closestNormal = normal;
					closestRegion = hasFaceRegions() ? faceRegion(face) : 0;
				}
			}
		}
	}

	// If a vertex is closest, we still have to compute the local pseudo-normal at the vertex.
	if(closestVertex != InvalidIndex) {
		const Point3& closestVertexPos = vertexPosition(closestVertex);

		// A vertex may have multiple pseudo-normals if it is part of multiple manifolds.
		// We need to compute normal belonging to each manifold and take the one that is facing 
		// away from the query point (if any).

		edge_index firstEdge = firstVertexEdge(closestVertex);
		QVarLengthArray<edge_index, 16> visitedEdges;
		for(;;) {
			// Skip edges that are no adjacent to a visible face.
			if(!faceSubset.empty()) {
				while(firstEdge != InvalidIndex && !faceSubset[adjacentFace(firstEdge)])
					firstEdge = nextVertexEdge(firstEdge);
			}
			if(firstEdge == InvalidIndex) break;

			if(std::find(visitedEdges.cbegin(), visitedEdges.cend(), firstEdge) == visitedEdges.cend()) {
				// Compute vertex pseudo-normal by averaging the normal vectors of adjacent faces.
				closestNormal.setZero();
				edge_index edge = firstEdge;
				Vector3 edge1v = wrapVector(vertexPosition(vertex2(edge)) - closestVertexPos);
				edge1v.normalizeSafely();
				do {
					visitedEdges.push_back(edge);
					OVITO_ASSERT(hasOppositeEdge(edge)); // Make sure the mesh is closed.
					edge_index nextEdge = nextFaceEdge(oppositeEdge(edge));
					OVITO_ASSERT(vertex1(nextEdge) == closestVertex);
					Vector3 edge2v = wrapVector(vertexPosition(vertex2(nextEdge)) - closestVertexPos);
					edge2v.normalizeSafely();
					FloatType angle = std::acos(edge1v.dot(edge2v));
					Vector3 normal = edge2v.cross(edge1v);
					if(normal != Vector3::Zero())
						closestNormal += normal.normalized() * angle;
					edge = nextEdge;
					edge1v = edge2v;
				}
				while(edge != firstEdge);
				closestRegion = hasFaceRegions() ? faceRegion(adjacentFace(firstEdge)) : 0;

				// We can stop if pseudo-normal is facing away from query point.
				if(closestNormal.dot(closestVector) > -epsilon) 
					break;
			}

			// Continue with next edge that is adjacent to the vertex.
			firstEdge = nextVertexEdge(firstEdge);
		}
	}

	FloatType dot = closestNormal.dot(closestVector);
	if(dot >= epsilon) return std::make_pair(closestRegion, sqrt(closestDistanceSq));
	if(dot <= -epsilon) return std::make_pair(spaceFillingRegion(), sqrt(closestDistanceSq));
	return {};
}

/******************************************************************************
* Constructs the convex hull from a set of points and adds the resulting
* polyhedron to the mesh.
******************************************************************************/
void SurfaceMeshAccess::constructConvexHull(std::vector<Point3> vecs, FloatType epsilon)
{
	// Create a new spatial region for the polyhedron in the output mesh.
	SurfaceMeshAccess::region_index region = createRegion();

	if(vecs.size() < 4) return;	// Convex hull requires at least 4 input points.

	// Keep track of how many faces and vertices we started with.
	// We won't touch the existing mesh faces and vertices.
	auto originalFaceCount = faceCount();
	auto originalVertexCount = vertexCount();

	// Determine which points are used to build the initial tetrahedron.
	// Make sure they are not co-planar and the tetrahedron is not degenerate.
	size_t tetrahedraCorners[4];
	tetrahedraCorners[0] = 0;
	Matrix3 m;

	// Find optimal second point.
	FloatType maxVal = epsilon;
	for(size_t i = 1; i < vecs.size(); i++) {
		m.column(0) = vecs[i] - vecs[0];
		FloatType distSq = m.column(0).squaredLength();
		if(distSq > maxVal) {
			maxVal = distSq;
			tetrahedraCorners[1] = i;
		}
	}
	// Convex hull is degenerate if all input points are identitical.
	if(maxVal <= epsilon)
		return;
	m.column(0) = vecs[tetrahedraCorners[1]] - vecs[0];

	// Find optimal third point.
	maxVal = epsilon;
	for(size_t i = 1; i < vecs.size(); i++) {
		if(i == tetrahedraCorners[1]) continue;
		m.column(1) = vecs[i] - vecs[0];
		FloatType areaSq = m.column(0).cross(m.column(1)).squaredLength();
		if(areaSq > maxVal) {
			maxVal = areaSq;
			tetrahedraCorners[2] = i;
		}
	}
	// Convex hull is degnerate if all input points are co-linear.
	if(maxVal <= epsilon)
		return;
	m.column(1) = vecs[tetrahedraCorners[2]] - vecs[0];

	// Find optimal fourth point.
	maxVal = epsilon;
	bool flipTet;
	for(size_t i = 1; i < vecs.size(); i++) {
		if(i == tetrahedraCorners[1] || i == tetrahedraCorners[2]) continue;
		m.column(2) = vecs[i] - vecs[0];
		FloatType vol = m.determinant();
		if(vol > maxVal) {
			maxVal = vol;
			flipTet = false;
			tetrahedraCorners[3] = i;
		}
		else if(-vol > maxVal) {
			maxVal = -vol;
			flipTet = true;
			tetrahedraCorners[3] = i;
		}
	}
	// Convex hull is degnerate if all input points are co-planar.
	if(maxVal <= epsilon)
		return;

	// Create the initial tetrahedron.
	vertex_index tetverts[4];
	for(size_t i = 0; i < 4; i++) {
        tetverts[i] = createVertex(vecs[tetrahedraCorners[i]]);
	}
	if(flipTet) 
		std::swap(tetverts[0], tetverts[1]);
	createFace({tetverts[0], tetverts[1], tetverts[3]}, region);
	createFace({tetverts[2], tetverts[0], tetverts[3]}, region);
	createFace({tetverts[0], tetverts[2], tetverts[1]}, region);
	createFace({tetverts[1], tetverts[2], tetverts[3]}, region);
	// Connect opposite half-edges to link the four faces together.
	for(size_t i = 0; i < 4; i++)
		mutableTopology()->connectOppositeHalfedges(tetverts[i]);

	if(vecs.size() == 4)
		return;	// If the input point set consists only of 4 points, then we are done after constructing the initial tetrahedron.

	// Remove 4 points of initial tetrahedron from input list.
	std::sort(std::begin(tetrahedraCorners), std::end(tetrahedraCorners), std::greater<>());
	OVITO_ASSERT(tetrahedraCorners[0] > tetrahedraCorners[1]);
	for(size_t i = 0; i < 4; i++)
		vecs[tetrahedraCorners[i]] = vecs[vecs.size()-i-1];
	vecs.erase(vecs.end() - 4, vecs.end());

	// Simplified Quick-hull algorithm.
	while(!vecs.empty()) {
		// Find the point on the positive side of a face and furthest away from it.
		// Also remove points from list which are on the negative side of all faces.
		auto furthestPoint = vecs.rend();
		FloatType furthestPointDistance = 0;
		size_t remainingPointCount = vecs.size();
		for(auto p = vecs.rbegin(); p != vecs.rend(); ++p) {
			bool insideHull = true;
			for(auto faceIndex = originalFaceCount; faceIndex < faceCount(); faceIndex++) {
				auto v0 = firstFaceVertex(faceIndex);
				auto v1 = secondFaceVertex(faceIndex);
				auto v2 = thirdFaceVertex(faceIndex);
				Plane3 plane(vertexPosition(v0), vertexPosition(v1), vertexPosition(v2), true);
				FloatType signedDistance = plane.pointDistance(*p);
				if(signedDistance > epsilon) {
					insideHull = false;
					if(signedDistance > furthestPointDistance) {
						furthestPointDistance = signedDistance;
						furthestPoint = p;
					}
				}
			}
			// When point is inside the hull, remove it from the input list.
			if(insideHull) {
				if(furthestPoint == vecs.rend() - remainingPointCount) furthestPoint = p;
				remainingPointCount--;
				*p = vecs[remainingPointCount];
			}
		}
		if(!remainingPointCount) break;
		OVITO_ASSERT(furthestPointDistance > 0 && furthestPoint != vecs.rend());

		// Kill all faces of the polyhedron that can be seen from the selected point.
		for(auto face = originalFaceCount; face < faceCount(); face++) {
			auto v0 = firstFaceVertex(face);
			auto v1 = secondFaceVertex(face);
			auto v2 = thirdFaceVertex(face);
			Plane3 plane(vertexPosition(v0), vertexPosition(v1), vertexPosition(v2), true);
			if(plane.pointDistance(*furthestPoint) > epsilon) {
				deleteFace(face);
				face--;
			}
		}

		// Find an edge that borders the newly created hole in the mesh.
		edge_index firstBorderEdge = InvalidIndex;
		for(auto face = originalFaceCount; face < faceCount() && firstBorderEdge == InvalidIndex; face++) {
			edge_index e = firstFaceEdge(face);
			OVITO_ASSERT(e != InvalidIndex);
			do {
				if(!hasOppositeEdge(e)) {
					firstBorderEdge = e;
					break;
				}
				e = nextFaceEdge(e);
			}
			while(e != firstFaceEdge(face));
		}
		OVITO_ASSERT(firstBorderEdge != InvalidIndex); // If this assert fails, then there was no hole in the mesh.

		// Create new faces that connects the edges at the horizon (i.e. the border of the hole) with
		// the selected vertex.
		vertex_index vertex = createVertex(*furthestPoint);
		edge_index borderEdge = firstBorderEdge;
		face_index previousFace = InvalidIndex;
		face_index firstFace = InvalidIndex;
		face_index newFace;
		do {
			newFace = createFace({ vertex2(borderEdge), vertex1(borderEdge), vertex }, region);
			linkOppositeEdges(firstFaceEdge(newFace), borderEdge);
			if(borderEdge == firstBorderEdge)
				firstFace = newFace;
			else
				linkOppositeEdges(secondFaceEdge(newFace), prevFaceEdge(firstFaceEdge(previousFace)));
			previousFace = newFace;
			// Proceed to next edge along the hole's border.
			for(;;) {
				borderEdge = nextFaceEdge(borderEdge);
				if(!hasOppositeEdge(borderEdge) || borderEdge == firstBorderEdge)
					break;
				borderEdge = oppositeEdge(borderEdge);
			}
		}
		while(borderEdge != firstBorderEdge);
		OVITO_ASSERT(firstFace != newFace);
		linkOppositeEdges(secondFaceEdge(firstFace), prevFaceEdge(firstFaceEdge(newFace)));

		// Remove selected point from the input list as well.
		remainingPointCount--;
		*furthestPoint = vecs[remainingPointCount];
		vecs.resize(remainingPointCount);
	}

	// Delete interior vertices from the mesh that are no longer attached to any of the faces.
	for(auto vertex = originalVertexCount; vertex < vertexCount(); vertex++) {
		if(vertexEdgeCount(vertex) == 0) {
			// Delete the vertex from the mesh topology.
			deleteVertex(vertex);
			// Adjust index to point to next vertex in the mesh after loop incrementation.
			vertex--;
		}
	}
}

/******************************************************************************
* Triangulates the polygonal faces of this mesh and outputs the results as a TriMesh object.
******************************************************************************/
void SurfaceMeshAccess::convertToTriMesh(TriMesh& outputMesh, bool smoothShading, const boost::dynamic_bitset<>& faceSubset, std::vector<size_t>* originalFaceMap, bool autoGenerateOppositeFaces) const
{
	size_type faceCount = this->faceCount();
	OVITO_ASSERT(faceSubset.empty() || faceSubset.size() == faceCount);

	// Create output vertices.
	auto baseVertexCount = outputMesh.vertexCount();
	auto baseFaceCount = outputMesh.faceCount();
	outputMesh.setVertexCount(baseVertexCount + vertexCount());
	vertex_index vidx = 0;
	for(auto p = outputMesh.vertices().begin() + baseVertexCount; p != outputMesh.vertices().end(); ++p)
		*p = vertexPosition(vidx++);

	// Transfer faces from surface mesh to output triangle mesh.
	for(face_index face = 0; face < faceCount; face++) {
		if(!faceSubset.empty() && !faceSubset[face]) continue;

		// Determine whether opposite triangles should be created for the current source face. 
		bool createOppositeFace = autoGenerateOppositeFaces && (!hasOppositeFace(face) || (!faceSubset.empty() && !faceSubset[oppositeFace(face)]));

		// Go around the edges of the face to triangulate the general polygon (assuming it is convex).
		edge_index faceEdge = firstFaceEdge(face);
		vertex_index baseVertex = vertex2(faceEdge);
		edge_index edge1 = nextFaceEdge(faceEdge);
		edge_index edge2 = nextFaceEdge(edge1);
		while(edge2 != faceEdge) {
			TriMeshFace& outputFace = outputMesh.addFace();
			outputFace.setVertices(baseVertex + baseVertexCount, vertex2(edge1) + baseVertexCount, vertex2(edge2) + baseVertexCount);
			outputFace.setEdgeVisibility(edge1 == nextFaceEdge(faceEdge), true, false);
			if(originalFaceMap)
				originalFaceMap->push_back(face);
			edge1 = edge2;
			edge2 = nextFaceEdge(edge2);
			if(edge2 == faceEdge)
				outputFace.setEdgeVisible(2);
			if(createOppositeFace) {
				TriMeshFace& oppositeFace = outputMesh.addFace();
				const TriMeshFace& thisFace = outputMesh.face(outputMesh.faceCount()-2);
				oppositeFace.setVertices(thisFace.vertex(2), thisFace.vertex(1), thisFace.vertex(0));
				oppositeFace.setEdgeVisibility(thisFace.edgeVisible(1), thisFace.edgeVisible(0), thisFace.edgeVisible(2));
				if(originalFaceMap)
					originalFaceMap->push_back(face);
			}
		}
	}

	if(smoothShading) {
		// Compute mesh face normals.
		std::vector<Vector3> faceNormals(faceCount);
		auto faceNormal = faceNormals.begin();
		for(face_index face = 0; face < faceCount; face++, ++faceNormal) {
			if(!faceSubset.empty() && !faceSubset[face])
				faceNormal->setZero();
			else
				*faceNormal = computeFaceNormal(face);
		}

		// Smooth normals.
		std::vector<Vector3> newFaceNormals(faceCount);
		auto oldFaceNormal = faceNormals.begin();
		auto newFaceNormal = newFaceNormals.begin();
		for(face_index face = 0; face < faceCount; face++, ++oldFaceNormal, ++newFaceNormal) {
			*newFaceNormal = *oldFaceNormal;
			if(!faceSubset.empty() && !faceSubset[face]) continue;

			edge_index faceEdge = firstFaceEdge(face);
			edge_index edge = faceEdge;
			do {
				edge_index oe = oppositeEdge(edge);
				if(oe != InvalidIndex) {
					*newFaceNormal += faceNormals[adjacentFace(oe)];
				}
				edge = nextFaceEdge(edge);
			}
			while(edge != faceEdge);

			newFaceNormal->normalizeSafely();
		}
		faceNormals = std::move(newFaceNormals);
		newFaceNormals.clear();
		newFaceNormals.shrink_to_fit();

		// Helper method that calculates the mean normal at a surface mesh vertex.
		// The method takes an half-edge incident on the vertex as input (instead of the vertex itself),
		// because the method will only take into account incident faces belonging to one manifold.
		auto calculateNormalAtVertex = [&](edge_index startEdge) {
			Vector3 normal = Vector3::Zero();
			edge_index edge = startEdge;
			do {
				normal += faceNormals[adjacentFace(edge)];
				edge = oppositeEdge(nextFaceEdge(edge));
				if(edge == InvalidIndex) break;
			}
			while(edge != startEdge);
			if(edge == InvalidIndex) {
				edge = oppositeEdge(startEdge);
				while(edge != InvalidIndex) {
					normal += faceNormals[adjacentFace(edge)];
					edge = oppositeEdge(prevFaceEdge(edge));
				}
			}
			return normal;
		};

		// Compute normal at each face vertex of the output mesh.
		outputMesh.setHasNormals(true);
		auto outputNormal = outputMesh.normals().begin() + (baseFaceCount * 3);
		for(face_index face = 0; face < faceCount; face++) {
			if(!faceSubset.empty() && !faceSubset[face]) continue;

			bool createOppositeFace = autoGenerateOppositeFaces && (!hasOppositeFace(face) || (!faceSubset.empty() && !faceSubset[oppositeFace(face)])) ;

			// Go around the edges of the face.
			edge_index faceEdge = firstFaceEdge(face);
			edge_index edge1 = nextFaceEdge(faceEdge);
			edge_index edge2 = nextFaceEdge(edge1);
			Vector3 baseNormal = calculateNormalAtVertex(faceEdge);
			Vector3 normal1 = calculateNormalAtVertex(edge1);
			while(edge2 != faceEdge) {
				Vector3 normal2 = calculateNormalAtVertex(edge2);
				*outputNormal++ = baseNormal;
				*outputNormal++ = normal1;
				*outputNormal++ = normal2;
				if(createOppositeFace) {
					*outputNormal++ = -normal2;
					*outputNormal++ = -normal1;
					*outputNormal++ = -baseNormal;
				}
				normal1 = normal2;
				edge2 = nextFaceEdge(edge2);
			}
		}
		OVITO_ASSERT(outputNormal == outputMesh.normals().end());
	}
}

/******************************************************************************
* Computes the unit normal vector of a mesh face.
******************************************************************************/
Vector3 SurfaceMeshAccess::computeFaceNormal(face_index face) const
{
	Vector3 faceNormal = Vector3::Zero();

	// Go around the edges of the face to triangulate the general polygon.
	edge_index faceEdge = firstFaceEdge(face);
	edge_index edge1 = nextFaceEdge(faceEdge);
	edge_index edge2 = nextFaceEdge(edge1);
	Point3 base = vertexPosition(vertex2(faceEdge));
	Vector3 e1 = wrapVector(vertexPosition(vertex2(edge1)) - base);
	while(edge2 != faceEdge) {
		Vector3 e2 = wrapVector(vertexPosition(vertex2(edge2)) - base);
		faceNormal += e1.cross(e2);
		e1 = e2;
		edge1 = edge2;
		edge2 = nextFaceEdge(edge2);
	}

	return faceNormal.safelyNormalized();
}

/******************************************************************************
* Joins adjacent faces that are coplanar.
******************************************************************************/
void SurfaceMeshAccess::joinCoplanarFaces(FloatType thresholdAngle)
{
	FloatType dotThreshold = std::cos(thresholdAngle);

	// Compute mesh face normals.
	std::vector<Vector3> faceNormals(faceCount());
	for(face_index face = 0; face < faceCount(); face++) {
		faceNormals[face] = computeFaceNormal(face);
	}

	// Visit each face and its adjacent faces.
	for(face_index face = 0; face < faceCount(); ) {
		face_index nextFace = face + 1;
		const Vector3& normal1 = faceNormals[face];
		edge_index faceEdge = firstFaceEdge(face);
		edge_index edge = faceEdge;
		do {
			edge_index opp_edge = oppositeEdge(edge);
			if(opp_edge != InvalidIndex) {
				face_index adj_face = adjacentFace(opp_edge);
				OVITO_ASSERT(adj_face >= 0 && adj_face < faceNormals.size());
				if(adj_face > face) {

					// Check if current face and its current neighbor are coplanar.
					const Vector3& normal2 = faceNormals[adj_face];
					if(normal1.dot(normal2) > dotThreshold) {
						// Eliminate this half-edge pair and join the two faces.
						SurfaceMeshTopology* topo = mutableTopology();
						for(edge_index currentEdge = nextFaceEdge(edge); currentEdge != edge; currentEdge = nextFaceEdge(currentEdge)) {
							OVITO_ASSERT(adjacentFace(currentEdge) == face);
							topo->setAdjacentFace(currentEdge, adj_face);
						}
						topo->setFirstFaceEdge(adj_face, nextFaceEdge(opp_edge));
						topo->setFirstFaceEdge(face, edge);
						topo->setNextFaceEdge(prevFaceEdge(edge), nextFaceEdge(opp_edge));
						topo->setPrevFaceEdge(nextFaceEdge(opp_edge), prevFaceEdge(edge));
						topo->setNextFaceEdge(prevFaceEdge(opp_edge), nextFaceEdge(edge));
						topo->setPrevFaceEdge(nextFaceEdge(edge), prevFaceEdge(opp_edge));
						topo->setNextFaceEdge(edge, opp_edge);
						topo->setNextFaceEdge(opp_edge, edge);
						topo->setPrevFaceEdge(edge, opp_edge);
						topo->setPrevFaceEdge(opp_edge, edge);
						topo->setAdjacentFace(opp_edge, face);
						OVITO_ASSERT(adjacentFace(edge) == face);
						OVITO_ASSERT(topo->countFaceEdges(face) == 2);
						faceNormals[face] = faceNormals[faceCount() - 1];
						deleteFace(face);
						nextFace = face;
						break;
					}
				}
			}
			edge = nextFaceEdge(edge);
		}
		while(edge != faceEdge);
		face = nextFace;
	}
}

/******************************************************************************
* Splits a face along the edge given by two vertices of the face.
******************************************************************************/
SurfaceMeshAccess::edge_index SurfaceMeshAccess::splitFace(edge_index edge1, edge_index edge2)
{
	OVITO_ASSERT(adjacentFace(edge1) == adjacentFace(edge2));
	OVITO_ASSERT(nextFaceEdge(edge1) != edge2);
	OVITO_ASSERT(prevFaceEdge(edge1) != edge2);
	OVITO_ASSERT(!hasOppositeFace(adjacentFace(edge1)));

	face_index old_f = adjacentFace(edge1);
	face_index new_f = createFace({}, hasFaceRegions() ? faceRegion(old_f) : 1);

	vertex_index v1 = vertex2(edge1);
	vertex_index v2 = vertex2(edge2);
	edge_index edge1_successor = nextFaceEdge(edge1);
	edge_index edge2_successor = nextFaceEdge(edge2);

	// Create the new pair of half-edges.
	SurfaceMeshTopology* topo = mutableTopology();
	edge_index new_e = topo->createEdge(v1, v2, old_f, edge1);
	edge_index new_oe = createOppositeEdge(new_e, new_f);

	// Rewire edge sequence of the primary face.
	OVITO_ASSERT(prevFaceEdge(new_e) == edge1);
	OVITO_ASSERT(nextFaceEdge(edge1) == new_e);
	topo->setNextFaceEdge(new_e, edge2_successor);
	topo->setPrevFaceEdge(edge2_successor, new_e);

	// Rewire edge sequence of the secondary face.
	topo->setNextFaceEdge(edge2, new_oe);
	topo->setPrevFaceEdge(new_oe, edge2);
	topo->setNextFaceEdge(new_oe, edge1_successor);
	topo->setPrevFaceEdge(edge1_successor, new_oe);

	// Connect the edges with the newly created secondary face.
	edge_index e = edge1_successor;
	do {
		topo->setAdjacentFace(e, new_f);
		e = nextFaceEdge(e);
	}
	while(e != new_oe);
	OVITO_ASSERT(adjacentFace(edge2) == new_f);
	OVITO_ASSERT(adjacentFace(new_oe) == new_f);

	// Make the newly created edge the leading edge of the original face.
	topo->setFirstFaceEdge(old_f, new_e);

	return new_e;
}

/******************************************************************************
* Joins pairs of triangular faces to form quadrilateral faces.
******************************************************************************/
void SurfaceMeshAccess::makeQuadrilateralFaces()
{
	// Visit each triangular face and its adjacent faces.
	for(face_index face = 0; face < faceCount(); ) {
		
		// Determine the longest edge of the current face and check if it is a triangle.
		// Find the longest edge of the three edges.
		edge_index faceEdge = firstFaceEdge(face);
		edge_index edge = faceEdge;
		int edgeCount = 0;
		edge_index longestEdge;
		FloatType longestEdgeLengthSq = 0;
		do {
			edgeCount++;
			FloatType edgeLengthSq = edgeVector(edge).squaredLength();
			if(edgeLengthSq >= longestEdgeLengthSq) {
				longestEdgeLengthSq = edgeLengthSq;
				longestEdge = edge;
			}
			edge = nextFaceEdge(edge);
		}
		while(edge != faceEdge);

		// Skip face if it is not a triangle.
		if(edgeCount != 3) {
			face++;
			continue;
		}
		face_index nextFace = face + 1;

		// Check if the adjacent face exists and is also a triangle.
		edge = longestEdge;
		edge_index opp_edge = oppositeEdge(edge);
		if(opp_edge != InvalidIndex) {
			face_index adj_face = adjacentFace(opp_edge);
			if(adj_face > face && topology()->countFaceEdges(adj_face) == 3) {

				// Eliminate this half-edge pair and join the two faces.
				SurfaceMeshTopology* topo = mutableTopology();
				for(edge_index currentEdge = nextFaceEdge(edge); currentEdge != edge; currentEdge = nextFaceEdge(currentEdge)) {
					OVITO_ASSERT(topo->adjacentFace(currentEdge) == face);
					topo->setAdjacentFace(currentEdge, adj_face);
				}
				topo->setFirstFaceEdge(adj_face, topo->nextFaceEdge(opp_edge));
				topo->setFirstFaceEdge(face, edge);
				topo->setNextFaceEdge(topo->prevFaceEdge(edge), topo->nextFaceEdge(opp_edge));
				topo->setPrevFaceEdge(topo->nextFaceEdge(opp_edge), topo->prevFaceEdge(edge));
				topo->setNextFaceEdge(topo->prevFaceEdge(opp_edge), topo->nextFaceEdge(edge));
				topo->setPrevFaceEdge(topo->nextFaceEdge(edge), topo->prevFaceEdge(opp_edge));
				topo->setNextFaceEdge(edge, opp_edge);
				topo->setNextFaceEdge(opp_edge, edge);
				topo->setPrevFaceEdge(edge, opp_edge);
				topo->setPrevFaceEdge(opp_edge, edge);
				topo->setAdjacentFace(opp_edge, face);
				OVITO_ASSERT(adjacentFace(edge) == face);
				OVITO_ASSERT(topo->countFaceEdges(face) == 2);
				deleteFace(face);
				nextFace = face;
			}
		}
		face = nextFace;
	}
}

/******************************************************************************
* Deletes all vertices from the mesh which are not connected to any half-edge.
******************************************************************************/
void SurfaceMeshAccess::deleteIsolatedVertices()
{
	for(vertex_index vertex = vertexCount() - 1; vertex >= 0; vertex--) {
		if(firstVertexEdge(vertex) == InvalidIndex) {
			deleteVertex(vertex);
		}
	}
}

}	// End of namespace
