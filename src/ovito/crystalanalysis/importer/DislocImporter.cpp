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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/Microstructure.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/crystalanalysis/objects/SlipSurfaceVis.h>
#include <ovito/crystalanalysis/modifier/microstructure/SimplifyMicrostructureModifier.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "DislocImporter.h"

#include <3rdparty/netcdf_integration/NetCDFIntegration.h>
#include <netcdf.h>
#include <boost/functional/hash.hpp>

namespace Ovito::CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool DislocImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	QString filename = QDir::toNativeSeparators(file.localFilePath());
	if(filename.isEmpty())
		return false;

	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker;

	// Check if we can open the input file for reading.
	int ncid;
	int err = nc_open(qUtf8Printable(filename), NC_NOWRITE, &ncid);
	if(err == NC_NOERR) {

		// Make sure we have the right file conventions.
		size_t len;
		if(nc_inq_attlen(ncid, NC_GLOBAL, "Conventions", &len) == NC_NOERR) {
			std::unique_ptr<char[]> conventions_str(new char[len+1]);
			if(nc_get_att_text(ncid, NC_GLOBAL, "Conventions", conventions_str.get()) == NC_NOERR) {
				conventions_str[len] = '\0';
				if(strcmp(conventions_str.get(), "FixDisloc") == 0) {
					nc_close(ncid);
					return true;
				}
			}
		}

		nc_close(ncid);
	}

	return false;
}

/******************************************************************************
* This method is called when the pipeline node for the FileSource is created.
******************************************************************************/
void DislocImporter::setupPipeline(PipelineSceneNode* pipeline, FileSource* importObj)
{
	ParticleImporter::setupPipeline(pipeline, importObj);

	// Insert a SimplyMicrostructureModifier into the data pipeline by default.
//	OORef<SimplifyMicrostructureModifier> modifier = new SimplifyMicrostructureModifier(pipeline->dataset());
//	pipeline->applyModifier(modifier);
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void DislocImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading disloc file %1").arg(fileHandle().toString()));

	QString filename = QDir::toNativeSeparators(fileHandle().localFilePath());
	if(filename.isEmpty())
		throw Exception(tr("The disloc file reader supports reading only from physical files. Cannot read data from an in-memory buffer."));

	// Fix disloc specific data.
	std::vector<Vector3> latticeVectors;
	std::vector<Vector3> transformedLatticeVectors;
	size_t segmentCount = 0;

	// Create data object.
	Microstructure* microstructureObj;
	if(const Microstructure* existingMicrostructureObj = state().getObject<Microstructure>()) {
		microstructureObj = state().makeMutable(existingMicrostructureObj);
	}
	else {
		microstructureObj = state().createObject<Microstructure>(dataSource());

		// Create a visual element for the dislocation lines.
		microstructureObj->setVisElement(OORef<DislocationVis>::create(dataset()));

		// Create a visual element for the slip surfaces.
		microstructureObj->addVisElement(OORef<SlipSurfaceVis>::create(dataset()));
	}

	/// The loaded microstructure.
	MicrostructureAccess microstructure(microstructureObj);
	microstructure.clearMesh();

	// Temporary data structure.
	std::vector<std::pair<qlonglong,qlonglong>> slipSurfaceMap;

	// Only serial access to NetCDF functions is allowed, because they are not thread-safe.
	NetCDFExclusiveAccess locker(this);
	if(!locker.isLocked()) return;

	int root_ncid = 0;
	try {
		// Open the input file for reading.
		NCERR(nc_open(qPrintable(filename), NC_NOWRITE, &root_ncid));

		// Make sure we have the right file convention.
		size_t len;
		NCERR(nc_inq_attlen(root_ncid, NC_GLOBAL, "Conventions", &len));
		auto conventions_str = std::make_unique<char[]>(len+1);
		NCERR(nc_get_att_text(root_ncid, NC_GLOBAL, "Conventions", conventions_str.get()));
		conventions_str[len] = '\0';
		if(strcmp(conventions_str.get(), "FixDisloc"))
			throw Exception(tr("NetCDF file follows '%1' conventions; expected 'FixDisloc' convention.").arg(conventions_str.get()));

		// Read precise version of file convention.
		enum DislocFileConvention {
			CONVENTION_1_1,
			CONVENTION_1_2
		};
		NCERR(nc_inq_attlen(root_ncid, NC_GLOBAL, "ConventionVersion", &len));
		auto convention_version_str = std::make_unique<char[]>(len+1);
		NCERR(nc_get_att_text(root_ncid, NC_GLOBAL, "ConventionVersion", convention_version_str.get()));
		convention_version_str[len] = '\0';
		DislocFileConvention fileConvention;
		if(strcmp(convention_version_str.get(), "1.1") == 0)
			fileConvention = CONVENTION_1_1;
		else if(strcmp(convention_version_str.get(), "1.2") == 0)
			fileConvention = CONVENTION_1_2;
		else
			throw Exception(tr("NetCDF file follows convention version %1. This version of OVITO only supports convention versions 1.1/1.2.").arg(convention_version_str.get()));

		// Read lattice structure.
		NCERR(nc_inq_attlen(root_ncid, NC_GLOBAL, "LatticeStructure", &len));
		auto lattice_structure_str = std::make_unique<char[]>(len+1);
		NCERR(nc_get_att_text(root_ncid, NC_GLOBAL, "LatticeStructure", lattice_structure_str.get()));
		lattice_structure_str[len] = '\0';

		// Get NetCDF dimensions.
		int spatial_dim, nodes_dim, dislocation_segments_dim, pair_dim, line_segment_dim, node_id_dim;
		NCERR(nc_inq_dimid(root_ncid, "spatial", &spatial_dim));
		NCERR(nc_inq_dimid(root_ncid, "nodes", &nodes_dim));
		NCERR(nc_inq_dimid(root_ncid, "dislocations", &dislocation_segments_dim));
		if(fileConvention == CONVENTION_1_1)
			NCERR(nc_inq_dimid(root_ncid, "pair", &pair_dim));
		else
			NCERR(nc_inq_dimid(root_ncid, "line_segment", &line_segment_dim));
		NCERR(nc_inq_dimid(root_ncid, "node_id", &node_id_dim));

		// Get NetCDF variables.
		int cell_vectors_var, cell_origin_var, cell_pbc_var;
		NCERR(nc_inq_varid(root_ncid, "cell_vectors", &cell_vectors_var));
		NCERR(nc_inq_varid(root_ncid, "cell_origin", &cell_origin_var));
		NCERR(nc_inq_varid(root_ncid, "cell_pbc", &cell_pbc_var));

		// Read simulation cell information.
		AffineTransformation cellMatrix;
		int cellPbc[3];
		size_t startp[2] = { 0, 0 };
		size_t countp[2] = { 3, 3 };
#ifdef FLOATTYPE_FLOAT
		NCERR(nc_get_vara_float(root_ncid, cell_vectors_var, startp, countp, cellMatrix.elements()));
		NCERR(nc_get_vara_float(root_ncid, cell_origin_var, startp, countp, cellMatrix.column(3).data()));
#else
		NCERR(nc_get_vara_double(root_ncid, cell_vectors_var, startp, countp, cellMatrix.elements()));
		NCERR(nc_get_vara_double(root_ncid, cell_origin_var, startp, countp, cellMatrix.column(3).data()));
#endif
		NCERR(nc_get_vara_int(root_ncid, cell_pbc_var, startp, countp, cellPbc));
		simulationCell()->setPbcFlags(cellPbc[0] != 0, cellPbc[1] != 0, cellPbc[2] != 0);
		simulationCell()->setCellMatrix(cellMatrix);
		microstructure.setCell(DataOORef<SimulationCellObject>::create(dataset(), ObjectCreationParams::WithoutVisElement, cellMatrix, cellPbc[0] != 0, cellPbc[1] != 0, cellPbc[2] != 0));

		// Read lattice orientation matrix.
		int lattice_orientation_var;
		NCERR(nc_inq_varid(root_ncid, "lattice_orientation", &lattice_orientation_var));
		Matrix3 latticeOrientation;
#ifdef FLOATTYPE_FLOAT
		NCERR(nc_get_var_float(root_ncid, lattice_orientation_var, latticeOrientation.elements()));
#else
		NCERR(nc_get_var_double(root_ncid, lattice_orientation_var, latticeOrientation.elements()));
#endif
		if(strcmp(lattice_structure_str.get(), "bcc") == 0) setLatticeStructure(ParticleType::PredefinedStructureType::BCC, latticeOrientation);
		else if(strcmp(lattice_structure_str.get(), "fcc") == 0) setLatticeStructure(ParticleType::PredefinedStructureType::FCC, latticeOrientation);
		else if(strcmp(lattice_structure_str.get(), "fcc_perfect") == 0) setLatticeStructure(ParticleType::PredefinedStructureType::FCC, latticeOrientation);
		else throw Exception(tr("File parsing error. Unknown lattice structure type: %1").arg(lattice_structure_str.get()));

		// Create microstructure regions.
		int emptyRegion = microstructure.createRegion(0);
		int crystalRegion = microstructure.createRegion(latticeStructure());

		// Read node list.
		int nodal_ids_var, nodal_positions_var;
		NCERR(nc_inq_varid(root_ncid, "nodal_ids", &nodal_ids_var));
		NCERR(nc_inq_varid(root_ncid, "nodal_positions", &nodal_positions_var));
		size_t numNodeRecords;
		NCERR(nc_inq_dimlen(root_ncid, nodes_dim, &numNodeRecords));
		std::vector<Point_3<float>> nodalPositions(numNodeRecords);
		std::vector<std::array<qlonglong,3>> nodalIds3;
		std::vector<std::array<qlonglong,4>> nodalIds4;
		if(numNodeRecords) {
			NCERR(nc_get_var_float(root_ncid, nodal_positions_var, nodalPositions.front().data()));
			if(fileConvention == CONVENTION_1_1) {
				nodalIds4.resize(numNodeRecords);
				NCERR(nc_get_var_longlong(root_ncid, nodal_ids_var, nodalIds4.front().data()));
			}
			else {
				nodalIds3.resize(numNodeRecords);
				NCERR(nc_get_var_longlong(root_ncid, nodal_ids_var, nodalIds3.front().data()));
			}
		}

		// Build list of unique nodes.
		std::vector<MicrostructureAccess::vertex_index> vertexMap;
		std::unordered_map<std::array<qlonglong,3>, MicrostructureAccess::vertex_index, boost::hash<std::array<qlonglong,3>>> idMap3;
		std::unordered_map<std::array<qlonglong,4>, MicrostructureAccess::vertex_index, boost::hash<std::array<qlonglong,4>>> idMap4;
		auto nodalPositionsIter = nodalPositions.cbegin();
		if(fileConvention == CONVENTION_1_1) {
			vertexMap.resize(numNodeRecords);
			auto vertexMapIter = vertexMap.begin();
			for(const auto& id : nodalIds4) {
				auto iter = idMap4.find(id);
				if(iter == idMap4.end())
					iter = idMap4.emplace(id, microstructure.createVertex(nodalPositionsIter->toDataType<FloatType>())).first;
				*vertexMapIter = iter->second;
				++vertexMapIter;
				++nodalPositionsIter;
			}
		}
		else {
			for(const auto& id : nodalIds3) {
				auto iter = idMap3.find(id);
				if(iter == idMap3.end())
					iter = idMap3.emplace(id, microstructure.createVertex(nodalPositionsIter->toDataType<FloatType>())).first;
				++nodalPositionsIter;
			}
		}

		// Read dislocation segments.
		int burgers_vectors_var, dislocation_segments_var;
		NCERR(nc_inq_varid(root_ncid, "burgers_vectors", &burgers_vectors_var));
		NCERR(nc_inq_varid(root_ncid, "dislocation_segments", &dislocation_segments_var));
		size_t numDislocationSegments;
		NCERR(nc_inq_dimlen(root_ncid, dislocation_segments_dim, &numDislocationSegments));
		std::vector<Vector_3<float>> burgersVectors(numDislocationSegments);
		std::vector<std::array<qlonglong,2>> dislocationSegments2;
		std::vector<std::array<qlonglong,3>> dislocationSegments3;
		if(numDislocationSegments) {
			NCERR(nc_get_var_float(root_ncid, burgers_vectors_var, burgersVectors.front().data()));
			if(fileConvention == CONVENTION_1_1) {
				dislocationSegments2.resize(numDislocationSegments);
				NCERR(nc_get_var_longlong(root_ncid, dislocation_segments_var, dislocationSegments2.front().data()));
			}
			else {
				dislocationSegments3.resize(numDislocationSegments);
				NCERR(nc_get_var_longlong(root_ncid, dislocation_segments_var, dislocationSegments3.front().data()));
			}
		}

		// Create dislocation segments.
		auto burgersVector = burgersVectors.cbegin();
		if(fileConvention == CONVENTION_1_1) {
			for(const auto& seg : dislocationSegments2) {
				OVITO_ASSERT(seg[0] >= 0 && seg[0] < (qlonglong)vertexMap.size());
				OVITO_ASSERT(seg[1] >= 0 && seg[1] < (qlonglong)vertexMap.size());
				MicrostructureAccess::vertex_index vertex1 = vertexMap[seg[0]];
				MicrostructureAccess::vertex_index vertex2 = vertexMap[seg[1]];
				microstructure.createDislocationSegment(vertex1, vertex2, (*burgersVector++).toDataType<FloatType>(), crystalRegion);
			}
			segmentCount = dislocationSegments2.size();
		}
		else {
			for(const auto& seg : dislocationSegments3) {
				std::array<qlonglong,3> nodeId1 = { seg[0], seg[1], 0 };
				std::array<qlonglong,3> nodeId2 = { seg[0], seg[1], seg[2] };
                if(nodeId1[1] < nodeId1[0])
                    std::swap(nodeId1[0], nodeId1[1]);
                std::sort(nodeId2.begin(), nodeId2.end());

				auto iter1 = idMap3.find(nodeId1);
				if(iter1 == idMap3.end())
					throw Exception(tr("Detected inconsistent dislocation segment information in NetCDF file."));

				auto iter2 = idMap3.find(nodeId2);
				if(iter2 == idMap3.end())
					throw Exception(tr("Detected inconsistent dislocation segment information in NetCDF file."));

				MicrostructureAccess::vertex_index vertex1 = iter1->second;
				MicrostructureAccess::vertex_index vertex2 = iter2->second;
				microstructure.createDislocationSegment(vertex1, vertex2, (*burgersVector++).toDataType<FloatType>(), crystalRegion);
			}
			segmentCount = dislocationSegments3.size();
		}

		// Form continuous dislocation lines from the segments.
		microstructure.makeContinuousDislocationLines();

		// Read slip facets.
		int slip_facets_dim = -1, slip_facet_vertices_dim = -1;
		nc_inq_dimid(root_ncid, "slip_facets", &slip_facets_dim);
		nc_inq_dimid(root_ncid, "slip_facet_vertices", &slip_facet_vertices_dim);
		if(slip_facets_dim != -1) {
			int slipped_edges_var;
			int slip_vectors_var;
			int slip_facet_normals_var;
			int slip_facet_edge_counts_var;
			int slip_facet_vertices_var;
			NCERR(nc_inq_varid(root_ncid, "slipped_edges", &slipped_edges_var));
			NCERR(nc_inq_varid(root_ncid, "slip_vectors", &slip_vectors_var));
			if(nc_inq_varid(root_ncid, "slip_facet_normals", &slip_facet_normals_var) != NC_NOERR)
				slip_facet_normals_var = -1;
			NCERR(nc_inq_varid(root_ncid, "slip_facet_edge_counts", &slip_facet_edge_counts_var));
			NCERR(nc_inq_varid(root_ncid, "slip_facet_vertices", &slip_facet_vertices_var));
			size_t numSlipFacets, numSlipFacetVertices;
			NCERR(nc_inq_dimlen(root_ncid, slip_facets_dim, &numSlipFacets));
			NCERR(nc_inq_dimlen(root_ncid, slip_facet_vertices_dim, &numSlipFacetVertices));
			std::vector<Vector_3<float>> slipVectors(numSlipFacets);
			std::vector<Vector_3<float>> slipFacetNormals;
			std::vector<std::array<qlonglong,2>> slippedEdges(numSlipFacets);
			std::vector<int> slipFacetEdgeCounts(numSlipFacets);
			std::vector<qlonglong> slipFacetVertices(numSlipFacetVertices);
			if(numSlipFacets) {
				NCERR(nc_get_var_float(root_ncid, slip_vectors_var, slipVectors.front().data()));
				if(slip_facet_normals_var != -1) {
					slipFacetNormals.resize(numSlipFacets);
					NCERR(nc_get_var_float(root_ncid, slip_facet_normals_var, slipFacetNormals.front().data()));
				}
				NCERR(nc_get_var_longlong(root_ncid, slipped_edges_var, slippedEdges.front().data()));
				NCERR(nc_get_var_int(root_ncid, slip_facet_edge_counts_var, slipFacetEdgeCounts.data()));
			}
			if(numSlipFacetVertices) {
				NCERR(nc_get_var_longlong(root_ncid, slip_facet_vertices_var, slipFacetVertices.data()));
			}

			// Create slip surface facets (two mesh faces per slip facet).
			auto slipVector = slipVectors.cbegin();
			auto slipFacetNormal = slipFacetNormals.cbegin();
			auto slipFacetEdgeCount = slipFacetEdgeCounts.cbegin();
			auto slipFacetVertex = slipFacetVertices.cbegin();
			slipSurfaceMap.resize(microstructure.faceCount());
			slipSurfaceMap.reserve(slipSurfaceMap.size() + numSlipFacets*2);
			for(const auto& slippedEdge : slippedEdges) {

				// Create first mesh face.
				MicrostructureAccess::face_index face = microstructure.createFace({}, crystalRegion, MicrostructureAccess::SLIP_FACET,
					(*slipVector).toDataType<FloatType>(), slipFacetNormals.empty() ? Vector3::Zero() : (*slipFacetNormal).toDataType<FloatType>());
				MicrostructureAccess::vertex_index node0 = vertexMap[*slipFacetVertex++];
				MicrostructureAccess::vertex_index node1 = node0;
				MicrostructureAccess::vertex_index node2;
				for(int i = 1; i < *slipFacetEdgeCount; i++, node1 = node2) {
					node2 = vertexMap[*slipFacetVertex++];
					microstructure.createEdge(node1, node2, face);
				}
				microstructure.createEdge(node1, node0, face);

				// Create the opposite mesh face.
				MicrostructureAccess::face_index oppositeFace = microstructure.createFace({}, crystalRegion, MicrostructureAccess::SLIP_FACET,
					-(*slipVector).toDataType<FloatType>(), slipFacetNormals.empty() ? Vector3::Zero() : -(*slipFacetNormal).toDataType<FloatType>());
				MicrostructureAccess::edge_index edge = microstructure.firstFaceEdge(face);
				do {
					microstructure.createEdge(microstructure.vertex2(edge), microstructure.vertex1(edge), oppositeFace);
					edge = microstructure.prevFaceEdge(edge);
				}
				while(edge != microstructure.firstFaceEdge(face));
				microstructure.linkOppositeFaces(face, oppositeFace);

				slipSurfaceMap.push_back({ slippedEdge[0], slippedEdge[1] });
				slipSurfaceMap.push_back({ slippedEdge[1], slippedEdge[0] });

				++slipVector;
				++slipFacetEdgeCount;
				if(!slipFacetNormals.empty())
					++slipFacetNormal;
			}
			OVITO_ASSERT(slipFacetVertex == slipFacetVertices.cend());
			OVITO_ASSERT(slipSurfaceMap.size() == microstructure.faceCount());
		}

		// Close the input file again.
		NCERR(nc_close(root_ncid));
	}
	catch(...) {
		if(root_ncid)
			nc_close(root_ncid);
		throw;
	}

	// Connect half-edges of slip faces.
	connectSlipFaces(microstructure, slipSurfaceMap);

	state().setStatus(tr("Number of nodes: %1\nNumber of segments: %2")
		.arg(microstructure.vertexCount())
		.arg(segmentCount));

	// Verify dislocation network (Burgers vector conservation at nodes).
	for(MicrostructureAccess::vertex_index vertex = 0; vertex < microstructure.vertexCount(); vertex++) {
		Vector3 sum = Vector3::Zero();
		for(auto e = microstructure.firstVertexEdge(vertex); e != SurfaceMeshAccess::InvalidIndex; e = microstructure.nextVertexEdge(e)) {
			if(microstructure.isPhysicalDislocationEdge(e))
				sum += microstructure.burgersVector(microstructure.adjacentFace(e));
		}
		if(!sum.isZero(1e-6))
			qDebug() << "Detected violation of Burgers vector conservation at location" << microstructure.vertexPosition(vertex) << "(" << microstructure.countDislocationArms(vertex) << "arms; delta_b =" << sum << ")";
	}

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

/*************************************************************************************
* Connects the slip faces to form two-dimensional manifolds.
**************************************************************************************/
void DislocImporter::FrameLoader::connectSlipFaces(MicrostructureAccess& microstructure, const std::vector<std::pair<qlonglong,qlonglong>>& slipSurfaceMap)
{
	// Link slip surface faces with their neighbors, i.e. find the opposite edge for every half-edge of a slip face.
	MicrostructureAccess::size_type edgeCount = microstructure.edgeCount();
	for(MicrostructureAccess::edge_index edge1 = 0; edge1 < edgeCount; edge1++) {
		// Only process edges which haven't been linked to their neighbors yet.
		if(microstructure.nextManifoldEdge(edge1) != SurfaceMeshAccess::InvalidIndex) continue;
		MicrostructureAccess::face_index face1 = microstructure.adjacentFace(edge1);
		if(!microstructure.isSlipSurfaceFace(face1)) continue;

		OVITO_ASSERT(!microstructure.hasOppositeEdge(edge1));
		MicrostructureAccess::vertex_index vertex1 = microstructure.vertex1(edge1);
		MicrostructureAccess::vertex_index vertex2 = microstructure.vertex2(edge1);
		MicrostructureAccess::edge_index oppositeEdge1 = microstructure.findEdge(microstructure.oppositeFace(face1), vertex2, vertex1);
		OVITO_ASSERT(oppositeEdge1 != SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(microstructure.nextManifoldEdge(edge1) == SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge1) == SurfaceMeshAccess::InvalidIndex);

		// At an edge, either 1, 2, or 3 slip surface manifolds can meet.
		// Here, we will link them together in the right order.

		const std::pair<qulonglong,qulonglong>& edgeVertexCodes = slipSurfaceMap[face1];

		// Find the other two manifolds meeting at the current edge (if they exist).
		MicrostructureAccess::edge_index edge2 = SurfaceMeshAccess::InvalidIndex;
		MicrostructureAccess::edge_index edge3 = SurfaceMeshAccess::InvalidIndex;
		MicrostructureAccess::edge_index oppositeEdge2 = SurfaceMeshAccess::InvalidIndex;
		MicrostructureAccess::edge_index oppositeEdge3 = SurfaceMeshAccess::InvalidIndex;
		for(MicrostructureAccess::edge_index e = microstructure.firstVertexEdge(vertex1); e != SurfaceMeshAccess::InvalidIndex; e = microstructure.nextVertexEdge(e)) {
			MicrostructureAccess::face_index face2 = microstructure.adjacentFace(e);
			if(microstructure.vertex2(e) == vertex2 && microstructure.isSlipSurfaceFace(face2) && face2 != face1) {
				const std::pair<qulonglong,qulonglong>& edgeVertexCodes2 = slipSurfaceMap[face2];
				if(edgeVertexCodes.second == edgeVertexCodes2.first) {
					OVITO_ASSERT(edgeVertexCodes.first != edgeVertexCodes2.second);
					OVITO_ASSERT(edge2 == SurfaceMeshAccess::InvalidIndex);
					OVITO_ASSERT(!microstructure.hasOppositeEdge(e));
					OVITO_ASSERT(microstructure.nextManifoldEdge(e) == SurfaceMeshAccess::InvalidIndex);
					edge2 = e;
					oppositeEdge2 = microstructure.findEdge(microstructure.oppositeFace(face2), vertex2, vertex1);
					OVITO_ASSERT(oppositeEdge2 != SurfaceMeshAccess::InvalidIndex);
					OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge2) == SurfaceMeshAccess::InvalidIndex);
				}
				else {
					OVITO_ASSERT(edgeVertexCodes.first == edgeVertexCodes2.second);
					OVITO_ASSERT(edge3 == SurfaceMeshAccess::InvalidIndex);
					OVITO_ASSERT(!microstructure.hasOppositeEdge(e));
					OVITO_ASSERT(microstructure.nextManifoldEdge(e) == SurfaceMeshAccess::InvalidIndex);
					edge3 = e;
					oppositeEdge3 = microstructure.findEdge(microstructure.oppositeFace(face2), vertex2, vertex1);
					OVITO_ASSERT(oppositeEdge3 != SurfaceMeshAccess::InvalidIndex);
					OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge3) == SurfaceMeshAccess::InvalidIndex);
				}
			}
		}

		if(edge2 != SurfaceMeshAccess::InvalidIndex) {
			microstructure.linkOppositeEdges(edge1, oppositeEdge2);
			microstructure.setNextManifoldEdge(edge1, edge2);
			microstructure.setNextManifoldEdge(oppositeEdge2, oppositeEdge1);
			if(edge3 != SurfaceMeshAccess::InvalidIndex) {
				microstructure.linkOppositeEdges(edge2, oppositeEdge3);
				microstructure.linkOppositeEdges(edge3, oppositeEdge1);
				microstructure.setNextManifoldEdge(edge2, edge3);
				microstructure.setNextManifoldEdge(oppositeEdge3, oppositeEdge2);
				microstructure.setNextManifoldEdge(edge3, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge3);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 3);
				OVITO_ASSERT(microstructure.countManifolds(edge2) == 3);
				OVITO_ASSERT(microstructure.countManifolds(edge3) == 3);
			}
			else {
				microstructure.linkOppositeEdges(edge2, oppositeEdge1);
				microstructure.setNextManifoldEdge(edge2, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge2);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(edge2) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge2) == 2);
			}
		}
		else {
			if(edge3 != SurfaceMeshAccess::InvalidIndex) {
				microstructure.linkOppositeEdges(edge1, oppositeEdge3);
				microstructure.linkOppositeEdges(oppositeEdge1, edge3);
				microstructure.setNextManifoldEdge(edge1, edge3);
				microstructure.setNextManifoldEdge(oppositeEdge3, oppositeEdge1);
				microstructure.setNextManifoldEdge(edge3, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge3);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge1) == 2);
				OVITO_ASSERT(microstructure.countManifolds(edge3) == 2);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge3) == 2);
			}
			else {
				microstructure.setNextManifoldEdge(edge1, edge1);
				microstructure.setNextManifoldEdge(oppositeEdge1, oppositeEdge1);
				OVITO_ASSERT(microstructure.countManifolds(edge1) == 1);
				OVITO_ASSERT(microstructure.countManifolds(oppositeEdge1) == 1);
			}
		}

		OVITO_ASSERT(microstructure.nextManifoldEdge(edge1) != SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(microstructure.vertex2(microstructure.nextManifoldEdge(edge1)) == vertex2);
		OVITO_ASSERT(microstructure.vertex1(microstructure.nextManifoldEdge(edge1)) == vertex1);
		OVITO_ASSERT(microstructure.nextManifoldEdge(oppositeEdge1) != SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(edge2 == SurfaceMeshAccess::InvalidIndex || microstructure.nextManifoldEdge(edge2) != SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(oppositeEdge2 == SurfaceMeshAccess::InvalidIndex || microstructure.nextManifoldEdge(oppositeEdge2) != SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(edge3 == SurfaceMeshAccess::InvalidIndex || microstructure.nextManifoldEdge(edge3) != SurfaceMeshAccess::InvalidIndex);
		OVITO_ASSERT(oppositeEdge3 == SurfaceMeshAccess::InvalidIndex || microstructure.nextManifoldEdge(oppositeEdge3) != SurfaceMeshAccess::InvalidIndex);
	}
}

#if 0
/******************************************************************************
* Inserts the data loaded by the FrameLoader into the provided data collection.
* This function is called by the system from the main thread after the
* asynchronous loading operation has finished.
******************************************************************************/
OORef<DataCollection> DislocImporter::DislocFrameData::handOver(const DataCollection* existing, bool isNewFile, CloneHelper& cloneHelper, FileSource* fileSource, const QString& identifierPrefix)
{
	// Insert simulation cell.
	OORef<DataCollection> output = ParticleFrameData::handOver(existing, isNewFile, cloneHelper, fileSource);

	// Insert microstructure.
	OORef<Microstructure> microstructureObj;
	if(const Microstructure* existingMicrostructure = existing ? existing->getObject<Microstructure>() : nullptr) {
		microstructureObj = cloneHelper.cloneObject(existingMicrostructure, false);
		output->addObject(microstructureObj);
	}
	else {
		microstructureObj = output->createObject<Microstructure>(fileSource);

		// Create a visual element for the dislocation lines.
		microstructureObj->setVisElement(OORef<DislocationVis>::create(fileSource->dataset(), Application::instance()->initializationHints()));

		// Create a visual element for the slip surfaces.
		microstructureObj->addVisElement(OORef<SlipSurfaceVis>::create(fileSource->dataset(), Application::instance()->initializationHints()));
	}
	microstructureObj->setDomain(output->getObject<SimulationCellObject>());
	microstructure().transferTo(microstructureObj);

	// Define crystal phase.
	OVITO_ASSERT(latticeStructure() != 0);
	OVITO_ASSERT(!microstructureObj->dataset()->undoStack().isRecording());
	PropertyObject* phaseProperty = microstructureObj->regions()->expectMutableProperty(SurfaceMeshRegions::PhaseProperty);
	OORef<MicrostructurePhase> phase = dynamic_object_cast<MicrostructurePhase>(phaseProperty->elementType(latticeStructure()));
	if(!phase) {
		phase = new MicrostructurePhase(phaseProperty->dataset());
		phase->setNumericId(latticeStructure());
		phase->setName(ParticleType::getPredefinedStructureTypeName(latticeStructure()));
		phaseProperty->addElementType(phase);
	}
	if(latticeStructure() == ParticleType::PredefinedStructureType::BCC) {
		phase->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
		phase->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, phase->name(), ParticleType::PredefinedStructureType::BCC));
		if(phase->burgersVectorFamilies().empty()) {
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset()));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 11, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 12, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 13, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));
		}
	}
	else if(latticeStructure() == ParticleType::PredefinedStructureType::FCC) {
		phase->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
		phase->setColor(ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, phase->name(), ParticleType::PredefinedStructureType::FCC));
		if(phase->burgersVectorFamilies().empty()) {
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset()));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 1, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 2, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 3, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 4, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset(), 5, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));
		}
	}
	else {
		phase->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::NoSymmetry);
		if(phase->burgersVectorFamilies().empty()) {
			phase->addBurgersVectorFamily(new BurgersVectorFamily(phase->dataset()));
		}
	}

	// Store lattice orientation information.
	OVITO_ASSERT(microstructureObj->regions()->elementCount() == 2);
	PropertyAccess<Matrix3> correspondenceProperty = microstructureObj->regions()->createProperty(SurfaceMeshRegions::LatticeCorrespondenceProperty);
	correspondenceProperty[0] = Matrix3::Zero();		// The "empty" region.
	correspondenceProperty[1] = _latticeOrientation;	// The "crystal" region.

	return output;
}
#endif

}	// End of namespace
