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

#include <ovito/particles/Particles.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleBondMap.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "CoordinationPolyhedraModifier.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(CoordinationPolyhedraModifier);
DEFINE_REFERENCE_FIELD(CoordinationPolyhedraModifier, surfaceMeshVis);
DEFINE_PROPERTY_FIELD(CoordinationPolyhedraModifier, transferParticleProperties);
SET_PROPERTY_FIELD_LABEL(CoordinationPolyhedraModifier, transferParticleProperties, "Transfer particle properties to mesh");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationPolyhedraModifier::CoordinationPolyhedraModifier(ObjectCreationParams params) : AsynchronousModifier(params),
	_transferParticleProperties(true)
{
	// Create the vis element for rendering the polyhedra generated by the modifier.
	if(params.createSubObjects()) {
		setSurfaceMeshVis(OORef<SurfaceMeshVis>::create(params));
		surfaceMeshVis()->setShowCap(false);
		surfaceMeshVis()->setSmoothShading(false);
		surfaceMeshVis()->setSurfaceTransparency(FloatType(0.25));
		surfaceMeshVis()->setObjectTitle(tr("Polyhedra"));
		if(params.loadUserDefaults())
			surfaceMeshVis()->setHighlightEdges(true);
	}
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CoordinationPolyhedraModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
		return particles->bonds() != nullptr;
	}
	return false;
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> CoordinationPolyhedraModifier::createEngine(const ModifierEvaluationRequest& request, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* selectionProperty = particles->getProperty(ParticlesObject::SelectionProperty);

	particles->expectBonds()->verifyIntegrity();
	const PropertyObject* topologyProperty = particles->expectBondsTopology();
	const PropertyObject* bondPeriodicImagesProperty = particles->bonds()->getProperty(BondsObject::PeriodicImageProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

	if(!selectionProperty)
		throwException(tr("Please first select some particles, for which coordination polyhedra should be generated."));

	// Collect the set of particle properties that should be transferred over to the surface mesh vertices and mesh regions.
	std::vector<ConstPropertyPtr> particleProperties;
	if(transferParticleProperties()) {
		for(const PropertyObject* property : particles->properties()) {
			// Certain properties should never be transferred to the mesh vertices.
			if(property->type() == ParticlesObject::SelectionProperty) continue;
			if(property->type() == ParticlesObject::PositionProperty) continue;
			if(property->type() == ParticlesObject::ColorProperty) continue;
			if(property->type() == ParticlesObject::VectorColorProperty) continue;
			if(property->type() == ParticlesObject::PeriodicImageProperty) continue;
			if(property->type() == ParticlesObject::TransparencyProperty) continue;
			particleProperties.push_back(property);
		}
	}

	// Create the output data object.
	DataOORef<SurfaceMesh> mesh = DataOORef<SurfaceMesh>::create(dataset(), ObjectCreationParams::WithoutVisElement, tr("Coordination polyhedra"));
	mesh->setIdentifier(input.generateUniqueIdentifier<SurfaceMesh>(QStringLiteral("coord-polyhedra")));
	mesh->setDataSource(request.modApp());
	mesh->setDomain(simCell);
	mesh->setVisElement(surfaceMeshVis());

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputePolyhedraEngine>(
			request,
			posProperty,
			selectionProperty,
			topologyProperty,
			bondPeriodicImagesProperty,
			std::move(mesh),
			std::move(particleProperties));
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::perform()
{
	setProgressText(tr("Generating coordination polyhedra"));

	// Create the "Region" face property.
	SurfaceMeshAccess mesh(_mesh);
	mesh.createFaceProperty(SurfaceMeshFaces::RegionProperty);

	// Determine number of selected particles.
	ConstPropertyAccess<int> selectionArray(_selection);
	size_t npoly = boost::count_if(selectionArray, [](int s) { return s != 0; });
	setProgressMaximum(npoly);

	ParticleBondMap bondMap(_bondTopology, _bondPeriodicImages);

	ConstPropertyAccess<Point3> positionsArray(_positions);

	// Working variables.
	std::vector<Point3> neighborPositions;
	std::vector<size_t> neighborIndices;
	SurfaceMeshAccess::size_type oldVertexCount = 0;

	// After construction of the mesh, this array will contain for each 
	// mesh vertex the index of the particle it was created from.
	std::vector<size_t> vertexToParticleMap;
	// After construction of the mesh, this array will contain for each 
	// mesh region the index of the particle it was created for.
	std::vector<size_t> regionToParticleMap;
	regionToParticleMap.reserve(npoly);

	// Iterate over all input particles.
	for(size_t i = 0; i < positionsArray.size(); i++) {
		// Construct corodination polyhedron only for selected particles.
		if(selectionArray[i] == 0) continue;

		// Collect the bonds that are part of the coordination polyhedron.
		const Point3& p1 = positionsArray[i];
		for(BondWithIndex bond : bondMap.bondsOfParticle(i)) {
			if(bond.index2 < _positions->size()) {
				Vector3 delta = positionsArray[bond.index2] - p1;
				if(bond.pbcShift.x()) delta += cell()->matrix().column(0) * (FloatType)bond.pbcShift.x();
				if(bond.pbcShift.y()) delta += cell()->matrix().column(1) * (FloatType)bond.pbcShift.y();
				if(bond.pbcShift.z()) delta += cell()->matrix().column(2) * (FloatType)bond.pbcShift.z();
				neighborPositions.push_back(p1 + delta);
				neighborIndices.push_back(bond.index2);
			}
		}

		// Include the central particle in the point list too.
		neighborPositions.push_back(p1);
		neighborIndices.push_back(i);
		regionToParticleMap.push_back(i);

		// Construct the polyhedron (i.e. convex hull) from the point list.
		if(particleProperties().empty()) {
			// Note: We are moving the point list into the convex hull method to avoid an extra copy.
			mesh.constructConvexHull(std::move(neighborPositions));
		}
		else {
			// Note: We keep our own copy of the point list so that we can determine the insertion order afterwards.
			mesh.constructConvexHull(neighborPositions);

			// Find each input point among the newly added vertices of the mesh. 
			// This will help us later to transfer the particle properties to the corresponding mesh vertices. 
			for(const Point3& vpos : mesh.vertexPositions().advance_begin(oldVertexCount)) {
				auto idx = neighborIndices.cbegin();
				for(const Point3& p : neighborPositions) {
					OVITO_ASSERT(idx != neighborIndices.cend());
					if(vpos == p) {
						vertexToParticleMap.push_back(*idx);
						break;
					}
					++idx;
				}
			}
			OVITO_ASSERT(vertexToParticleMap.size() == mesh.vertexCount());
			oldVertexCount = mesh.vertexCount();
		}

		// Clear point list for next loop iteration.
		neighborPositions.clear();
		neighborIndices.clear();

		if(!incrementProgressValue())
			return;
	}
	OVITO_ASSERT(regionToParticleMap.size() == mesh.regionCount());

	// Transfer particle properties to the mesh vertices and mesh regions if requested.
	if(!particleProperties().empty()) {
		OVITO_ASSERT(vertexToParticleMap.size() == mesh.vertexCount());
		for(const ConstPropertyPtr& particleProperty : particleProperties()) {

			// Create the corresponding output mesh vertex property.
			PropertyPtr vertexProperty;
			if(SurfaceMeshVertices::OOClass().isValidStandardPropertyId(particleProperty->type())) {
				// Input property is also a standard property for mesh vertices.
				vertexProperty = mesh.createVertexProperty(static_cast<SurfaceMeshVertices::Type>(particleProperty->type()));
				OVITO_ASSERT(vertexProperty->dataType() == particleProperty->dataType());
				OVITO_ASSERT(vertexProperty->stride() == particleProperty->stride());
			}
			else if(SurfaceMeshVertices::OOClass().standardPropertyTypeId(particleProperty->name()) != 0) {
				// Input property name is that of a standard property for mesh vertices.
				// Must rename the property to avoid conflict, because user properties may not have a standard property name.
				QString newPropertyName = particleProperty->name() + tr("_particles");
				vertexProperty = mesh.createVertexProperty(newPropertyName, particleProperty->dataType(), particleProperty->componentCount(), DataBuffer::NoFlags, particleProperty->componentNames());
			}
			else {
				// Input property is a user property for mesh vertices.
				vertexProperty = mesh.createVertexProperty(particleProperty->name(), particleProperty->dataType(), particleProperty->componentCount(), DataBuffer::NoFlags, particleProperty->componentNames());
			}
			// Copy particle property values to mesh vertices using precomputed index mapping.
			particleProperty->mappedCopyTo(*vertexProperty, vertexToParticleMap);
			// Also adapt element types of the property.
			vertexProperty->setElementTypes(particleProperty->elementTypes());

			// Create the corresponding output mesh region property.
			PropertyPtr regionProperty;
			if(SurfaceMeshRegions::OOClass().isValidStandardPropertyId(particleProperty->type())) {
				// Input property is also a standard property for mesh regions.
				regionProperty = mesh.createRegionProperty(static_cast<SurfaceMeshRegions::Type>(particleProperty->type()));
				OVITO_ASSERT(regionProperty->dataType() == particleProperty->dataType());
				OVITO_ASSERT(regionProperty->stride() == particleProperty->stride());
			}
			else if(SurfaceMeshRegions::OOClass().standardPropertyTypeId(particleProperty->name()) != 0) {
				// Input property name is that of a standard property for mesh regions.
				// Must rename the property to avoid conflict, because user properties may not have a standard property name.
				QString newPropertyName = particleProperty->name() + tr("_particles");
				regionProperty = mesh.createRegionProperty(newPropertyName, particleProperty->dataType(), particleProperty->componentCount(), DataBuffer::NoFlags, particleProperty->componentNames());
			}
			else {
				// Input property is a user property for mesh regions.
				regionProperty = mesh.createRegionProperty(particleProperty->name(), particleProperty->dataType(), particleProperty->componentCount(), DataBuffer::NoFlags, particleProperty->componentNames());
			}
			// Copy particle property values to mesh regions using precomputed index mapping.
			particleProperty->mappedCopyTo(*regionProperty, regionToParticleMap);
			// Also adapt element types of the property.
			regionProperty->setElementTypes(particleProperty->elementTypes());
		}
	}

	// Create the "Particle index" region property, which contains the index of the particle that is at the center of each coordination polyhedron.
	PropertyPtr particleIndexProperty = mesh.createRegionProperty(QStringLiteral("Particle Index"), PropertyObject::Int64);
	std::copy(regionToParticleMap.cbegin(), regionToParticleMap.cend(), PropertyAccess<qlonglong>(particleIndexProperty).begin());

	// Release data that is no longer needed.
	_positions.reset();
	_selection.reset();
	_bondTopology.reset();
	_bondPeriodicImages.reset();
	_particleProperties.clear();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::applyResults(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	// Output the constructed mesh to the pipeline.
	state.addObjectWithUniqueId<SurfaceMesh>(_mesh);
}

}	// End of namespace
