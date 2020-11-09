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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/delaunay/DelaunayTessellation.h>
#include <ovito/delaunay/ManifoldConstructionHelper.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/grid/modifier/MarchingCubes.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "ConstructSurfaceModifier.h"

#include <boost/range/numeric.hpp>

namespace Ovito { namespace Particles {

using namespace Ovito::Delaunay;

IMPLEMENT_OVITO_CLASS(ConstructSurfaceModifier);
DEFINE_REFERENCE_FIELD(ConstructSurfaceModifier, surfaceMeshVis);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, smoothingLevel);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, probeSphereRadius);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, selectSurfaceParticles);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, transferParticleProperties);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, identifyRegions);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, method);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, gridResolution);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, radiusFactor);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, isoValue);
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, onlySelectedParticles, "Use only selected input particles");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, selectSurfaceParticles, "Select particles at the surface");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, transferParticleProperties, "Transfer particle properties to surface");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, identifyRegions, "Identify volumetric regions (filled/void)");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, method, "Construction method");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, gridResolution, "Resolution");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, radiusFactor, "Radius scaling");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, isoValue, "Iso value");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, smoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(ConstructSurfaceModifier, gridResolution, IntegerParameterUnit, 2, 600);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, radiusFactor, PercentParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ConstructSurfaceModifier::ConstructSurfaceModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_smoothingLevel(8),
	_probeSphereRadius(4),
	_onlySelectedParticles(false),
	_selectSurfaceParticles(false),
	_transferParticleProperties(false),
	_method(AlphaShape),
	_gridResolution(50),
	_radiusFactor(1.0),
	_isoValue(0.6),
	_identifyRegions(false)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void ConstructSurfaceModifier::loadUserDefaults(Application::ExecutionContext executionContext)
{
	// Create the vis element for rendering the surface generated by the modifier.
	setSurfaceMeshVis(OORef<SurfaceMeshVis>::create(dataset(), executionContext));

	AsynchronousModifier::loadUserDefaults(executionContext);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool ConstructSurfaceModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> ConstructSurfaceModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, Application::ExecutionContext executionContext)
{
	// Get input particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get particle selection flags if requested.
	const PropertyObject* selProperty = onlySelectedParticles() ? particles->expectProperty(ParticlesObject::SelectionProperty) : nullptr;

	// Get particle cluster property.
	const PropertyObject* clusterProperty = particles->getProperty(ParticlesObject::ClusterProperty);

	// Get simulation cell.
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The construct surface mesh modifier does not support 2d simulation cells."));

	// Collect the set of particle properties that should be transferred over to the surface mesh vertices.
	std::vector<ConstPropertyPtr> particleProperties;
	if(transferParticleProperties()) {
		for(const PropertyObject* property : particles->properties()) {
			// Certain properties should not be transferred to the mesh vertices.
			if(property->type() == ParticlesObject::SelectionProperty) continue;
			if(property->type() == ParticlesObject::PositionProperty) continue;
			if(property->type() == ParticlesObject::IdentifierProperty) continue;
			particleProperties.push_back(property);
		}
	}
	
	// Create an empty surface mesh.
	DataOORef<SurfaceMesh> mesh = DataOORef<SurfaceMesh>::create(dataset(), executionContext, tr("Surface"));
	mesh->setIdentifier(input.generateUniqueIdentifier<SurfaceMesh>(QStringLiteral("surface")));
	mesh->setDataSource(modApp);
	mesh->setDomain(simCell);
	mesh->setVisElement(surfaceMeshVis());

	if(method() == AlphaShape) {
		// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
		return std::make_shared<AlphaShapeEngine>(executionContext, dataset(), 
				posProperty,
				selProperty,
				clusterProperty,
				std::move(mesh),
				probeSphereRadius(),
				smoothingLevel(),
				selectSurfaceParticles(),
				identifyRegions(),
				std::move(particleProperties));
	}
	else {
		// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
		return std::make_shared<GaussianDensityEngine>(executionContext, dataset(), 
				posProperty,
				selProperty,
				std::move(mesh),
				radiusFactor(),
				isoValue(),
				gridResolution(),
				particles->inputParticleRadii(),
				std::move(particleProperties));
	}
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void ConstructSurfaceModifier::AlphaShapeEngine::perform()
{
	setProgressText(tr("Constructing surface mesh"));
	OVITO_ASSERT(mesh()->domain());

	if(probeSphereRadius() <= 0)
		throw Exception(tr("Radius parameter must be positive."));

	if(mesh()->domain()->volume3D() <= FLOATTYPE_EPSILON*FLOATTYPE_EPSILON*FLOATTYPE_EPSILON)
		throw Exception(tr("Simulation cell is degenerate."));

	double alpha = probeSphereRadius() * probeSphereRadius();
	FloatType ghostLayerSize = probeSphereRadius() * FloatType(3.5);

	// Check if combination of radius parameter and simulation cell size is valid.
	for(size_t dim = 0; dim < 3; dim++) {
		if(mesh()->domain()->hasPbc(dim)) {
			int stencilCount = (int)ceil(ghostLayerSize / mesh()->domain()->matrix().column(dim).dot(mesh()->domain()->cellNormalVector(dim)));
			if(stencilCount > 1)
				throw Exception(tr("Cannot generate Delaunay tessellation. Simulation cell is too small, or radius parameter is too large."));
		}
	}

	// Algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	beginProgressSubStepsWithWeights({ 10, 30, 2, 2, 2 });

	// Generate Delaunay tessellation.
	DelaunayTessellation tessellation;
	bool coverDomainWithFiniteTets = _identifyRegions;

	if(!tessellation.generateTessellation(
			mesh()->domain(), 
			ConstPropertyAccess<Point3>(positions()).cbegin(), 
			positions()->size(), 
			ghostLayerSize,
			coverDomainWithFiniteTets, 
			selection() ? ConstPropertyAccess<int>(selection()).cbegin() : nullptr, 
			*this))
		return;

	nextProgressSubStep();

	SurfaceMeshData mesh(this->mesh());

	// Predefine the filled spatial regions if there is already a particle cluster assignement. 
	if(_identifyRegions && particleClusters()) {
		
		// Determine the maximum cluster ID.
		qlonglong maxClusterId = 0;
		if(particleClusters()->size() != 0) {
			maxClusterId = qBound<qlonglong>(0, 
				*boost::max_element(ConstPropertyAccess<qlonglong>(particleClusters())), 
				std::numeric_limits<SurfaceMeshData::region_index>::max() - 1);
		}

		// Create one region in the output mesh for each particle cluster.
		mesh.createRegions(maxClusterId + 1);
	}

	// Helper function that determines which spatial region a filled Delaunay cell belongs to.
	auto tetrahedronRegion = [&,clusters = ConstPropertyAccess<qlonglong>(_identifyRegions ? particleClusters() : nullptr)](DelaunayTessellation::CellHandle cell) -> SurfaceMeshData::region_index {
		if(clusters) {
			// Decide which particle cluster the Delaunay cell belongs to.
			// We need a tie-breaker in case the four vertex atoms belong to different clusters.
			qlonglong result = 0;
			for(int v = 0; v < 4; v++) {
				size_t particleIndex = tessellation.vertexIndex(tessellation.cellVertex(cell, v));
				qlonglong clusterId = clusters[particleIndex];
				if(clusterId > result)
					result = clusterId;
			}
			return result;
		}
		return 0;
	};

	// This callback function is called for every surface facet created by the manifold construction helper.
	PropertyAccess<int> surfaceParticleSelectionArray(surfaceParticleSelection());
	auto prepareMeshFace = [&](SurfaceMeshData::face_index face, const std::array<size_t,3>& vertexIndices, const std::array<DelaunayTessellation::VertexHandle,3>& vertexHandles, DelaunayTessellation::CellHandle cell) {
		// Mark the face's corner particles as belonging to the surface.
		if(surfaceParticleSelectionArray) {
			for(size_t vi : vertexIndices) {
				OVITO_ASSERT(vi < surfaceParticleSelectionArray.size());
				surfaceParticleSelectionArray[vi] = 1;
			}
		}
	};

	// This callback function is called for every surface vertex created by the manifold construction helper.
	std::vector<size_t> vertexToParticleMap;
	auto prepareMeshVertex = [&](SurfaceMeshData::vertex_index vertex, size_t particleIndex) {
		OVITO_ASSERT(vertex == vertexToParticleMap.size());
		vertexToParticleMap.push_back(particleIndex);
	};

	if(!_identifyRegions) {
		// Predefine the filled spatial region. 
		// An empty region is not defined, because we are creating only a one-sided surface mesh.
		mesh.createRegion();
		OVITO_ASSERT(mesh.regionCount() == 1);

		// Just construct a one-sided surface mesh without caring about spatial regions.
		ManifoldConstructionHelper manifoldConstructor(executionContext(), tessellation, mesh, alpha, false, positions());
		if(!manifoldConstructor.construct(tetrahedronRegion, *this, std::move(prepareMeshFace), std::move(prepareMeshVertex)))
			return;
	}
	else {
		beginProgressSubStepsWithWeights({ 2, 1 });

		// Construct a two-sided surface mesh with mesh faces associated with spatial regions (filled or solid).
		ManifoldConstructionHelper manifoldConstructor(executionContext(), tessellation, mesh, alpha, true, positions());
		if(!manifoldConstructor.construct(tetrahedronRegion, *this, std::move(prepareMeshFace), std::move(prepareMeshVertex)))
			return;

		nextProgressSubStep();

		// After construct() above has identified the filled regions, now identify the empty regions.
		if(!manifoldConstructor.formEmptyRegions(*this))
			return;

		_filledRegionCount = manifoldConstructor.filledRegionCount();
		_emptyRegionCount = manifoldConstructor.emptyRegionCount();

		// Output auxiliary per-region information.
		PropertyAccess<int> filledProperty(mesh.createRegionProperty(SurfaceMeshRegions::IsFilledProperty, false, executionContext()));
		std::fill(filledProperty.begin(), filledProperty.begin() + _filledRegionCount, 1);
		std::fill(filledProperty.begin() + _filledRegionCount, filledProperty.end(), 0);

		endProgressSubSteps();
	}

	// Create mesh vertex properties.
	for(const ConstPropertyPtr& particleProperty : particleProperties()) {
		PropertyPtr vertexProperty;
		if(SurfaceMeshVertices::OOClass().isValidStandardPropertyId(particleProperty->type())) {
			// Input property is also a standard property for mesh vertices.
			vertexProperty = mesh.createVertexProperty(static_cast<SurfaceMeshVertices::Type>(particleProperty->type()), false, executionContext());
			OVITO_ASSERT(vertexProperty->dataType() == particleProperty->dataType());
			OVITO_ASSERT(vertexProperty->stride() == particleProperty->stride());
		}
		else if(SurfaceMeshVertices::OOClass().standardPropertyTypeId(particleProperty->name()) != 0) {
			// Input property name is that of a standard property for mesh vertices.
			// Must rename the property to avoid conflict, because user properties may not have a standard property name.
			QString newPropertyName = particleProperty->name() + tr("_particles");
			vertexProperty = mesh.createVertexProperty(newPropertyName, particleProperty->dataType(), particleProperty->componentCount(), particleProperty->stride(), false, particleProperty->componentNames());
		}
		else {
			// Input property is a user property for mesh vertices.
			vertexProperty = mesh.createVertexProperty(particleProperty->name(), particleProperty->dataType(), particleProperty->componentCount(), particleProperty->stride(), false, particleProperty->componentNames());
		}
		// Copy particle property values to mesh vertices using precomputed index mapping.
		particleProperty->mappedCopyTo(*vertexProperty, vertexToParticleMap);
	}

	nextProgressSubStep();

	// Make sure every mesh vertex is only part of one surface manifold.
	SurfaceMeshData::size_type duplicatedVertices = mesh.makeManifold();

	nextProgressSubStep();
	if(!mesh.smoothMesh(_smoothingLevel, *this))
		return;

	nextProgressSubStep();

	if(_identifyRegions) {
		// Create the 'Surface area' region property.
		PropertyAccess<FloatType> surfaceAreaProperty = mesh.createRegionProperty(SurfaceMeshRegions::SurfaceAreaProperty, true, executionContext());

		// Compute surface area (total and per region) by summing up the triangle face areas.
		setProgressMaximum(mesh.faceCount());
		for(SurfaceMeshData::edge_index edge : mesh.firstFaceEdges()) {
			if(!incrementProgressValue()) return;
			const Vector3& e1 = mesh.edgeVector(edge);
			const Vector3& e2 = mesh.edgeVector(mesh.nextFaceEdge(edge));
			FloatType faceArea = e1.cross(e2).length() / 2;
			SurfaceMeshData::region_index region = mesh.faceRegion(mesh.adjacentFace(edge));
			surfaceAreaProperty[region] += faceArea;

			// Only count surface area of outer surface, which is bordering an empty region.
			// Don't count area of internal interfaces.
			if(region >= _filledRegionCount)
				addSurfaceArea(faceArea);
		}

		// Compute total volumes.
		for(SurfaceMeshData::region_index region = 0; region < _filledRegionCount; region++)
			_totalFilledVolume += mesh.regionVolume(region);
		for(SurfaceMeshData::region_index region = _filledRegionCount; region < mesh.regionCount(); region++)
			_totalEmptyVolume += mesh.regionVolume(region);
	}
	else {
		// Compute total surface area by summing up the triangle face areas.
		setProgressMaximum(mesh.faceCount());
		for(SurfaceMeshData::edge_index edge : mesh.firstFaceEdges()) {
			if(!incrementProgressValue()) return;
			const Vector3& e1 = mesh.edgeVector(edge);
			const Vector3& e2 = mesh.edgeVector(mesh.nextFaceEdge(edge));
			FloatType faceArea = e1.cross(e2).length() / 2;
			addSurfaceArea(faceArea);
		}
	}

	endProgressSubSteps();

	// Release data that is no longer needed.
	releaseWorkingData();
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void ConstructSurfaceModifier::GaussianDensityEngine::perform()
{
	setProgressText(tr("Constructing surface mesh"));
	OVITO_ASSERT(mesh()->domain());

	// Check input data.
	if(mesh()->domain()->volume3D() <= FLOATTYPE_EPSILON*FLOATTYPE_EPSILON*FLOATTYPE_EPSILON)
		throw Exception(tr("Simulation cell is degenerate."));

	if(positions()->size() == 0) {
		// Release data that is no longer needed.
		releaseWorkingData();
		return;
	}

	// Algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	beginProgressSubStepsWithWeights({ 1, 30, 1600, 1500, 30, 500, 100, 300 });

	// Scale the atomic radii.
	for(FloatType& r : _particleRadii) r *= _radiusFactor;

	// Determine the cutoff range of atomic Gaussians.
	FloatType cutoffSize = FloatType(3) * *std::max_element(_particleRadii.cbegin(), _particleRadii.cend());

	// Determine the extents of the density grid.
	AffineTransformation gridBoundaries = mesh()->domain()->matrix();
	ConstPropertyAccess<Point3> positionsArray(positions());
	for(size_t dim = 0; dim < 3; dim++) {
		// Use bounding box of particles in directions that are non-periodic.
		if(!mesh()->domain()->hasPbc(dim)) {
			// Compute range of relative atomic coordinates in the current direction.
			FloatType xmin =  FLOATTYPE_MAX;
			FloatType xmax = -FLOATTYPE_MAX;
			const AffineTransformation inverseCellMatrix = mesh()->domain()->inverseMatrix();
			for(const Point3& p : positionsArray) {
				FloatType rp = inverseCellMatrix.prodrow(p, dim);
				if(rp < xmin) xmin = rp;
				if(rp > xmax) xmax = rp;
			}

			// Need to add extra margin along non-periodic dimensions, because Gaussian functions reach beyond atomic radii.
			FloatType rcutoff = cutoffSize / gridBoundaries.column(dim).length();
			xmin -= rcutoff;
			xmax += rcutoff;

			gridBoundaries.column(3) += xmin * gridBoundaries.column(dim);
			gridBoundaries.column(dim) *= (xmax - xmin);
		}
	}

	// Determine the number of voxels in each direction of the density grid.
	size_t gridDims[3];
	FloatType voxelSizeX = gridBoundaries.column(0).length() / _gridResolution;
	FloatType voxelSizeY = gridBoundaries.column(1).length() / _gridResolution;
	FloatType voxelSizeZ = gridBoundaries.column(2).length() / _gridResolution;
	FloatType voxelSize = std::max(voxelSizeX, std::max(voxelSizeY, voxelSizeZ));
	gridDims[0] = std::max((size_t)2, (size_t)(gridBoundaries.column(0).length() / voxelSize));
	gridDims[1] = std::max((size_t)2, (size_t)(gridBoundaries.column(1).length() / voxelSize));
	gridDims[2] = std::max((size_t)2, (size_t)(gridBoundaries.column(2).length() / voxelSize));

	nextProgressSubStep();

	// Allocate storage for the density grid values.
	std::vector<FloatType> densityData(gridDims[0] * gridDims[1] * gridDims[2], FloatType(0));

	// Set up a particle neighbor finder to speed up density field computation.
	CutoffNeighborFinder neighFinder;
	if(!neighFinder.prepare(cutoffSize, positions(), mesh()->domain(), selection(), this))
		return;

	nextProgressSubStep();

	// Set up a matrix that converts grid coordinates to spatial coordinates.
	AffineTransformation gridToCartesian = gridBoundaries;
	gridToCartesian.column(0) /= gridDims[0] - (mesh()->domain()->hasPbc(0)?0:1);
	gridToCartesian.column(1) /= gridDims[1] - (mesh()->domain()->hasPbc(1)?0:1);
	gridToCartesian.column(2) /= gridDims[2] - (mesh()->domain()->hasPbc(2)?0:1);

	// Compute the accumulated density at each grid point.
	parallelFor(densityData.size(), *this, [&](size_t voxelIndex) {

		// Determine the center coordinates of the current grid cell.
		size_t ix = voxelIndex % gridDims[0];
		size_t iy = (voxelIndex / gridDims[0]) % gridDims[1];
		size_t iz = voxelIndex / (gridDims[0] * gridDims[1]);
		Point3 voxelCenter = gridToCartesian * Point3(ix, iy, iz);
		FloatType& density = densityData[voxelIndex];

		// Visit all particles in the vicinity of the center point.
		for(CutoffNeighborFinder::Query neighQuery(neighFinder, voxelCenter); !neighQuery.atEnd(); neighQuery.next()) {
			FloatType alpha = _particleRadii[neighQuery.current()];
			density += std::exp(-neighQuery.distanceSquared() / (FloatType(2) * alpha * alpha));
		}
	});
	if(isCanceled())
		return;

	nextProgressSubStep();

	// Set up callback function returning the field value, which will be passed to the marching cubes algorithm.
    auto getFieldValue = [
			_data = densityData.data(),
			_pbcFlags = mesh()->domain()->pbcFlags(),
			_gridShape = gridDims
			](int i, int j, int k) -> FloatType {
        if(_pbcFlags[0]) {
            if(i == _gridShape[0]) i = 0;
        }
        else {
            if(i == 0 || i == _gridShape[0] + 1) return std::numeric_limits<FloatType>::lowest();
            i--;
        }
        if(_pbcFlags[1]) {
            if(j == _gridShape[1]) j = 0;
        }
        else {
            if(j == 0 || j == _gridShape[1] + 1) return std::numeric_limits<FloatType>::lowest();
            j--;
        }
        if(_pbcFlags[2]) {
            if(k == _gridShape[2]) k = 0;
        }
        else {
            if(k == 0 || k == _gridShape[2] + 1) return std::numeric_limits<FloatType>::lowest();
            k--;
        }
        OVITO_ASSERT(i >= 0 && i < _gridShape[0]);
        OVITO_ASSERT(j >= 0 && j < _gridShape[1]);
        OVITO_ASSERT(k >= 0 && k < _gridShape[2]);
        return _data[(i + j*_gridShape[0] + k*_gridShape[0]*_gridShape[1])];
    };

	// Set the domain of the output mesh.
	if(mesh()->domain()->cellMatrix() != gridBoundaries) {
		auto newCell = DataOORef<SimulationCellObject>::makeCopy(mesh()->domain());
		newCell->setCellMatrix(gridBoundaries);
		mesh()->setDomain(std::move(newCell));
	}

	// Construct isosurface of the density field.
	SurfaceMeshData mesh(this->mesh());
	MarchingCubes mc(mesh, gridDims[0], gridDims[1], gridDims[2], false, std::move(getFieldValue));
	if(!mc.generateIsosurface(_isoLevel, *this))
		return;

	nextProgressSubStep();

	// Transform mesh vertices from orthogonal grid space to world space.
	mesh.transformVertices(gridToCartesian);
	if(isCanceled())
		return;

	nextProgressSubStep();

	// Create mesh vertex properties for transferring particle property values to the surface.
	std::vector<std::pair<ConstPropertyAccess<FloatType,true>, PropertyAccess<FloatType,true>>> propertyMapping;
	for(const ConstPropertyPtr& particleProperty : particleProperties()) {
		// Can only transfer floating-point properties, because we'll need to blend values of several particles.
		if(particleProperty->dataType() == PropertyObject::Float) {
			PropertyPtr vertexProperty;
			if(SurfaceMeshVertices::OOClass().isValidStandardPropertyId(particleProperty->type())) {
				// Input property is also a standard property for mesh vertices.
				vertexProperty = mesh.createVertexProperty(static_cast<SurfaceMeshVertices::Type>(particleProperty->type()), true, executionContext());
				OVITO_ASSERT(vertexProperty->dataType() == particleProperty->dataType());
				OVITO_ASSERT(vertexProperty->stride() == particleProperty->stride());
			}
			else if(SurfaceMeshVertices::OOClass().standardPropertyTypeId(particleProperty->name()) != 0) {
				// Input property name is that of a standard property for mesh vertices.
				// Must rename the property to avoid conflict, because user properties may not have a standard property name.
				QString newPropertyName = particleProperty->name() + tr("_particles");
				vertexProperty = mesh.createVertexProperty(newPropertyName, particleProperty->dataType(), particleProperty->componentCount(), particleProperty->stride(), true, particleProperty->componentNames());
			}
			else {
				// Input property is a user property for mesh vertices.
				vertexProperty = mesh.createVertexProperty(particleProperty->name(), particleProperty->dataType(), particleProperty->componentCount(), particleProperty->stride(), true, particleProperty->componentNames());
			}
			propertyMapping.emplace_back(particleProperty, std::move(vertexProperty));
		}
	}

	// Transfer property values from particles to the mesh vertices.
	if(!propertyMapping.empty()) {
		// Compute the accumulated density at each grid point.
		parallelFor(mesh.vertexCount(), *this, [&](size_t vertexIndex) {
			// Visit all particles in the vicinity of the vertex.
			FloatType weightSum = 0;
			for(CutoffNeighborFinder::Query neighQuery(neighFinder, mesh.vertexPosition(vertexIndex)); !neighQuery.atEnd(); neighQuery.next()) {
				FloatType alpha = _particleRadii[neighQuery.current()];
				FloatType weight = std::exp(-neighQuery.distanceSquared() / (FloatType(2) * alpha * alpha));
				// Perform summation of particle contributions to the property values at the current mesh vertex.
				for(auto& p : propertyMapping) {
					for(size_t component = 0; component < p.first.componentCount(); component++) {
						p.second.value(vertexIndex, component) += weight * p.first.get(neighQuery.current(), component);
					}
				}
				weightSum += weight;
			}
			if(weightSum != 0) {
				// Normalize property values.
				for(auto& p : propertyMapping) {
					for(size_t component = 0; component < p.second.componentCount(); component++) {
						p.second.value(vertexIndex, component) /= weightSum;
					}
				}
			}
		});
		if(isCanceled())
			return;
	}

	// Flip surface orientation if cell is mirrored.
	if(gridToCartesian.determinant() < 0)
		mesh.flipFaces();

	nextProgressSubStep();

	if(!mesh.connectOppositeHalfedges())
		throw Exception(tr("Something went wrong. Isosurface mesh is not closed."));
	if(isCanceled())
		return;

	nextProgressSubStep();

	// Compute surface area (only total) by summing up the triangle face areas.
	for(SurfaceMeshData::edge_index edge : mesh.firstFaceEdges()) {
		if(isCanceled()) return;
		const Vector3& e1 = mesh.edgeVector(edge);
		const Vector3& e2 = mesh.edgeVector(mesh.nextFaceEdge(edge));
		FloatType area = e1.cross(e2).length() / 2;
		addSurfaceArea(area);
	}

	endProgressSubSteps();

	// Release data that is no longer needed.
	releaseWorkingData();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ConstructSurfaceModifier::AlphaShapeEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ConstructSurfaceModifier* modifier = static_object_cast<ConstructSurfaceModifier>(modApp->modifier());

	// Output the constructed surface mesh to the pipeline.
	state.addObjectWithUniqueId<SurfaceMesh>(mesh());

	// Output selection of surface particles.
	if(surfaceParticleSelection()) {
		ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();
		particles->verifyIntegrity();
		particles->createProperty(surfaceParticleSelection());
	}

	// Output total surface area.
	state.addAttribute(QStringLiteral("ConstructSurfaceMesh.surface_area"), QVariant::fromValue(surfaceArea()), modApp);

	if(_identifyRegions) {

		// Output more global attributes.
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.cell_volume"), QVariant::fromValue(_totalCellVolume), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.specific_surface_area"), QVariant::fromValue(_totalCellVolume ? (surfaceArea() / _totalCellVolume) : 0), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.filled_volume"), QVariant::fromValue(_totalFilledVolume), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.filled_fraction"), QVariant::fromValue(_totalCellVolume ? (_totalFilledVolume / _totalCellVolume) : 0), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.filled_region_count"), QVariant::fromValue(_filledRegionCount), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.empty_volume"), QVariant::fromValue(_totalEmptyVolume), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.empty_fraction"), QVariant::fromValue(_totalCellVolume ? (_totalEmptyVolume / _totalCellVolume) : 0), modApp);
		state.addAttribute(QStringLiteral("ConstructSurfaceMesh.empty_region_count"), QVariant::fromValue(_emptyRegionCount), modApp);

		QString statusString = tr("Surface area: %1\n# filled regions (volume): %2 (%3)\n# empty regions (volume): %4 (%5)")
				.arg(surfaceArea())
				.arg(_filledRegionCount)
				.arg(_totalFilledVolume)
				.arg(_emptyRegionCount)
				.arg(_totalEmptyVolume);

		state.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusString)));
	}
	else {
		state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Surface area: %1").arg(surfaceArea())));
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void ConstructSurfaceModifier::GaussianDensityEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ConstructSurfaceModifier* modifier = static_object_cast<ConstructSurfaceModifier>(modApp->modifier());

	// Output the constructed surface mesh to the pipeline.
	state.addObjectWithUniqueId<SurfaceMesh>(mesh());

	// Output total surface area.
	state.addAttribute(QStringLiteral("ConstructSurfaceMesh.surface_area"), QVariant::fromValue(surfaceArea()), modApp);

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Surface area: %1").arg(surfaceArea())));
}

}	// End of namespace
}	// End of namespace
