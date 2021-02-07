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

#include <ovito/particles/Particles.h>
#include <ovito/particles/modifier/modify/LoadTrajectoryModifier.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "ParticleImporter.h"

#include <boost/range/numeric.hpp>

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticleImporter);
DEFINE_PROPERTY_FIELD(ParticleImporter, sortParticles);
DEFINE_PROPERTY_FIELD(ParticleImporter, generateBonds);
DEFINE_PROPERTY_FIELD(ParticleImporter, recenterCell);
SET_PROPERTY_FIELD_LABEL(ParticleImporter, sortParticles, "Sort particles by ID");
SET_PROPERTY_FIELD_LABEL(ParticleImporter, generateBonds, "Generate bonds");
SET_PROPERTY_FIELD_LABEL(ParticleImporter, recenterCell, "Center simulation box on coordinate origin");

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ParticleImporter::propertyChanged(const PropertyFieldDescriptor& field)
{
	FileSourceImporter::propertyChanged(field);

	if(field == PROPERTY_FIELD(sortParticles) || field == PROPERTY_FIELD(generateBonds) || field == PROPERTY_FIELD(recenterCell)) {
		// Reload input file(s) when these options are changed by the user.
		// But there is no need to refetch the data file(s) from the remote location. Reparsing the cached files is sufficient.
		requestReload();
	}
}

/******************************************************************************
* Returns the particles container object, newly creating it first if necessary.
******************************************************************************/
ParticlesObject* ParticleImporter::FrameLoader::particles()
{
	if(!_particles) {
		_particles = state().getMutableObject<ParticlesObject>();
		if(!_particles) {
			_particles = state().createObject<ParticlesObject>(dataSource(), executionContext());
			if(_particleRadiusScalingFactor != 1.0) {
				// Set up the vis element for the particles.
				if(ParticlesVis* particlesVis = dynamic_object_cast<ParticlesVis>(_particles->visElement())) {
					particlesVis->setRadiusScaleFactor(_particleRadiusScalingFactor);
				}
			}
		}
	}
	return _particles;
}

/******************************************************************************
* Returns the bonds container object, newly creating it first if necessary.
******************************************************************************/
BondsObject* ParticleImporter::FrameLoader::bonds()
{
	if(!_bonds) {
		if(particles()->bonds()) {
			_bonds = particles()->makeBondsMutable();
		}
		else {
			particles()->setBonds(DataOORef<BondsObject>::create(dataset(), executionContext()));
			_bonds = particles()->makeBondsMutable();
			_bonds->setDataSource(dataSource());
		}
	}
	return _bonds;
}

/******************************************************************************
* Returns the angles container object, newly creating it first if necessary.
******************************************************************************/
AnglesObject* ParticleImporter::FrameLoader::angles()
{
	if(!_angles) {
		if(particles()->angles()) {
			_angles = particles()->makeAnglesMutable();
		}
		else {
			particles()->setAngles(DataOORef<AnglesObject>::create(dataset(), executionContext()));
			_angles = particles()->makeAnglesMutable();
			_angles->setDataSource(dataSource());
		}
	}
	return _angles;
}

/******************************************************************************
* Returns the dihedrals container object, newly creating it first if necessary.
******************************************************************************/
DihedralsObject* ParticleImporter::FrameLoader::dihedrals()
{
	if(!_dihedrals) {
		if(particles()->dihedrals()) {
			_dihedrals = particles()->makeDihedralsMutable();
		}
		else {
			particles()->setDihedrals(DataOORef<DihedralsObject>::create(dataset(), executionContext()));
			_dihedrals = particles()->makeDihedralsMutable();
			_dihedrals->setDataSource(dataSource());
		}
	}
	return _dihedrals;
}

/******************************************************************************
* Returns the impropers container object, newly creating it first if necessary.
******************************************************************************/
ImpropersObject* ParticleImporter::FrameLoader::impropers()
{
	if(!_impropers) {
		if(particles()->impropers()) {
			_impropers = particles()->makeImpropersMutable();
		}
		else {
			particles()->setImpropers(DataOORef<ImpropersObject>::create(dataset(), executionContext()));
			_impropers = particles()->makeImpropersMutable();
			_impropers->setDataSource(dataSource());
		}
	}
	return _impropers;
}

/******************************************************************************
* Creates a particle object (if the particle count is non-zero) and adjusts the 
* number of elements of the property container.
******************************************************************************/
void ParticleImporter::FrameLoader::setParticleCount(size_t count)
{
	if(count != 0) {
		particles()->setElementCount(count);
	}
	else {
		if(const ParticlesObject* particles = state().getObject<ParticlesObject>())
			state().removeObject(particles);
		_particles = nullptr;
	}
}

/******************************************************************************
* Creates a bonds container object (if the bond count is non-zero) and adjusts the 
* number of elements of the property container.
******************************************************************************/
void ParticleImporter::FrameLoader::setBondCount(size_t count)
{
	if(count != 0) {
		bonds()->setElementCount(count);
	}
	else {
		if(const ParticlesObject* particles = state().getObject<ParticlesObject>())
			if(particles->bonds())
				state().makeMutable(particles)->setBonds(nullptr);
		_bonds = nullptr;
	}
}

/******************************************************************************
* Creates an angles container object (if the bond count is non-zero) and adjusts the 
* number of elements of the property container.
******************************************************************************/
void ParticleImporter::FrameLoader::setAngleCount(size_t count)
{
	if(count != 0) {
		angles()->setElementCount(count);
	}
	else {
		if(const ParticlesObject* particles = state().getObject<ParticlesObject>())
			if(particles->angles())
				state().makeMutable(particles)->setAngles(nullptr);
		_angles = nullptr;
	}
}

/******************************************************************************
* Creates a dihedrals container object (if the bond count is non-zero) and adjusts the 
* number of elements of the property container.
******************************************************************************/
void ParticleImporter::FrameLoader::setDihedralCount(size_t count)
{
	if(count != 0) {
		dihedrals()->setElementCount(count);
	}
	else {
		if(const ParticlesObject* particles = state().getObject<ParticlesObject>())
			if(particles->dihedrals())
				state().makeMutable(particles)->setDihedrals(nullptr);
		_dihedrals = nullptr;
	}
}

/******************************************************************************
* Creates an impropers containerobject (if the bond count is non-zero) and adjusts the 
* number of elements of the property container.
******************************************************************************/
void ParticleImporter::FrameLoader::setImproperCount(size_t count)
{
	if(count != 0) {
		impropers()->setElementCount(count);
	}
	else {
		if(const ParticlesObject* particles = state().getObject<ParticlesObject>())
			if(particles->impropers())
				state().makeMutable(particles)->setImpropers(nullptr);
		_impropers = nullptr;
	}
}

/******************************************************************************
* Determines the PBC shift vectors for bonds using the minimum image convention.
******************************************************************************/
void ParticleImporter::FrameLoader::generateBondPeriodicImageProperty()
{
	ConstPropertyAccess<Point3> posProperty = particles()->getProperty(ParticlesObject::PositionProperty);
	if(!posProperty) return;

	ConstPropertyAccess<ParticleIndexPair> bondTopologyProperty = bonds()->getProperty(BondsObject::TopologyProperty);
	if(!bondTopologyProperty) return;

	PropertyAccess<Vector3I> bondPeriodicImageProperty = bonds()->createProperty(BondsObject::PeriodicImageProperty, false, executionContext());

	if(!hasSimulationCell() || !simulationCell()->hasPbc()) {
		bondPeriodicImageProperty.take()->fill(Vector3I::Zero());
	}
	else {
		const AffineTransformation inverseCellMatrix = simulationCell()->inverseMatrix();
		const std::array<bool,3> pbcFlags = simulationCell()->pbcFlags();
		for(size_t bondIndex = 0; bondIndex < bondTopologyProperty.size(); bondIndex++) {
			size_t index1 = bondTopologyProperty[bondIndex][0];
			size_t index2 = bondTopologyProperty[bondIndex][1];
			OVITO_ASSERT(index1 < posProperty.size() && index2 < posProperty.size());
			Vector3 delta = posProperty[index1] - posProperty[index2];
			for(size_t dim = 0; dim < 3; dim++) {
				if(pbcFlags[dim])
					bondPeriodicImageProperty[bondIndex][dim] = std::lround(inverseCellMatrix.prodrow(delta, dim));
			}
		}
	}
}

/******************************************************************************
* Generates ad-hoc bonds between atoms based on their van der Waals radii.
******************************************************************************/
void ParticleImporter::FrameLoader::generateBonds()
{
	if(isCanceled()) return;
	if(!_particles) return;

	// Get the type particle property.
	const PropertyObject* typeProperty = _particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* positionProperty = _particles->getProperty(ParticlesObject::PositionProperty);
	if(!typeProperty || !positionProperty) return;

	// Get the list of van der Waals radii.
	std::vector<FloatType> typeVdWRadiusMap;
	FloatType maxRadius = 0;
	for(const ElementType* type : typeProperty->elementTypes()) {
		if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(type)) {
			if(ptype->vdwRadius() > 0.0 && ptype->numericId() >= 0) {
				if(ptype->vdwRadius() > maxRadius)
					maxRadius = ptype->vdwRadius();
				if(type->numericId() >= typeVdWRadiusMap.size())
					typeVdWRadiusMap.resize(type->numericId() + 1, 0.0);
				typeVdWRadiusMap[type->numericId()] = ptype->vdwRadius();
			}
		}
	}

	// Determine maximum bond distance cutoff.
	FloatType vdwPrefactor = 0.6; // Note: Value 0.6 has been adopted from VMD source code.
	FloatType maxCutoff = vdwPrefactor * 2.0 * maxRadius;
	if(maxCutoff == 0.0)
		return;
	FloatType minCutoffSquared = 1e-10 * maxCutoff * maxCutoff;
	setProgressText(tr("Generating bonds"));
	
	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(maxCutoff, positionProperty, state().getObject<SimulationCellObject>(), {}, this))
		return;	

	ConstPropertyAccess<int> particleTypesArray(typeProperty);

	// Multi-threaded loop over all particles, each thread producing a partial bonds list.
	size_t particleCount = positionProperty->size();
	auto partialBondsLists = parallelForCollect<std::vector<Bond>>(particleCount, *this, [&](size_t particleIndex, std::vector<Bond>& bondList) {
		// Kernel called for each particle: Iterate over the particle's neighbors withing the cutoff range.
		for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
			int type1 = particleTypesArray[particleIndex];
			int type2 = particleTypesArray[neighborQuery.current()];
			if(type1 >= 0 && type2 >= 0 && type1 < (int)typeVdWRadiusMap.size() && type2 < (int)typeVdWRadiusMap.size()) {
				FloatType cutoff = vdwPrefactor * (typeVdWRadiusMap[type1] + typeVdWRadiusMap[type2]);
				if(neighborQuery.distanceSquared() <= cutoff*cutoff && neighborQuery.distanceSquared() >= minCutoffSquared) {
					Bond bond = { particleIndex, neighborQuery.current(), neighborQuery.unwrappedPbcShift() };
					// Skip every other bond to create only one bond per particle pair.
					if(!bond.isOdd())
						bondList.push_back(bond);
				}
			}
		}
	});
	if(isCanceled())
		return;

	// Create BondsObject.
	setBondCount(boost::accumulate(partialBondsLists, (size_t)0, [](size_t n, const std::vector<Bond>& bonds) { return n + bonds.size(); }));
	PropertyAccess<ParticleIndexPair> bondTopologyProperty = this->bonds()->createProperty(BondsObject::TopologyProperty, false, executionContext());
	PropertyAccess<int> bondTypeProperty = this->bonds()->createProperty(BondsObject::TypeProperty, false, executionContext());
	PropertyAccess<Vector3I> bondPeriodicImageProperty = this->bonds()->createProperty(BondsObject::PeriodicImageProperty, false, executionContext());

	// Create bond type.
	addNumericType(BondsObject::OOClass(), bondTypeProperty.buffer(), 1, {});

	// Transfer bonds lists to BondsObject.
	boost::fill(bondTypeProperty, 1);
	auto bondTopologyIter = bondTopologyProperty.begin();
	auto bondPBCImageIter = bondPeriodicImageProperty.begin();
	for(const std::vector<Bond>& bondsList : partialBondsLists) {
		for(const Bond& bond : bondsList) {
			*bondTopologyIter++ = ParticleIndexPair{{(qlonglong)bond.index1, (qlonglong)bond.index2}};
			*bondPBCImageIter++ = bond.pbcShift;
		}
	}
	OVITO_ASSERT(bondTopologyIter == bondTopologyProperty.end());
}

/******************************************************************************
* If the 'Velocity' vector particle property is present, then this method 
* computes the 'Velocity Magnitude' scalar property.
******************************************************************************/
void ParticleImporter::FrameLoader::computeVelocityMagnitude()
{
	if(!_particles || isCanceled()) 
		return;

	if(ConstPropertyAccess<Vector3> velocityVectors = _particles->getProperty(ParticlesObject::VelocityProperty)) {
		auto v = velocityVectors.cbegin();
		PropertyObject* magnitudeProperty = particles()->createProperty(ParticlesObject::VelocityMagnitudeProperty, false, executionContext());
		for(FloatType& mag : PropertyAccess<FloatType>(magnitudeProperty)) {
			mag = v->length();
			++v;
		}
	}
}

/******************************************************************************
* If the particles are centered on the coordinate origin but the current simulation cell corner is positioned at (0,0,0), 
* the this method centers the cell at (0,0,0), leaving the particle coordinates unchanged.
******************************************************************************/
void ParticleImporter::FrameLoader::correctOffcenterCell()
{
	if(isCanceled()) 
		return;

	// Check if a simulation cell has been defined. It must be periodic in all directions.
	const SimulationCellObject* simulationCell = state().getObject<SimulationCellObject>();
	if(!simulationCell || !simulationCell->hasPbc(0) || !simulationCell->hasPbc(1) || (!simulationCell->hasPbc(2) && !simulationCell->is2D())) 
		return;

	// The cell corner must be located at (0,0,0).
	if(simulationCell->cellOrigin() != Point3::Origin())
		return;

	// The current implementation is for 3D cells only.
	if(simulationCell->is2D() || simulationCell->cellMatrix().determinant() == 0.0)
		return;

	// Get the particle coordinates.
	ConstPropertyAccess<Point3> positions = _particles ? _particles->getProperty(ParticlesObject::PositionProperty) : nullptr;
	if(!positions || positions.size() == 0)
		return;

	// Compute bounding box of particles in reduced coordinates.
	Box3 boundingBox;
	const AffineTransformation reciprocalCellMatrix = simulationCell->reciprocalCellMatrix();
	for(const Point3& p : positions)
		boundingBox.addPoint(reciprocalCellMatrix * p);
	OVITO_ASSERT(!boundingBox.isEmpty());

	// Check if reduced coordinates of particles are all in the [-0.5, 0.5] range (with an added margin).
	if(boundingBox.minc.x() > -0.01 && boundingBox.minc.y() > -0.01 && boundingBox.minc.z() > -0.01)
		return;
	if(boundingBox.minc.x() < -0.51 || boundingBox.minc.y() < -0.51 || boundingBox.minc.z() < -0.51)
		return;
	if(boundingBox.maxc.x() > 0.51 || boundingBox.maxc.y() > 0.51 || boundingBox.maxc.z() > 0.51)
		return;

	// Translate the simulation box.
	SimulationCellObject* newSimulationCell = state().makeMutable(simulationCell);
	AffineTransformation cellMatrix = newSimulationCell->cellMatrix();
	cellMatrix.translation() = cellMatrix * Vector3(-0.5, -0.5, -0.5);
	newSimulationCell->setCellMatrix(cellMatrix);
}

/******************************************************************************
* Translates the simulation cell (and the particles) such that it is centered 
* at the coordinate origin.
******************************************************************************/
void ParticleImporter::FrameLoader::recenterSimulationCell()
{
	if(isCanceled()) 
		return;

	SimulationCellObject* simulationCell = state().getMutableObject<SimulationCellObject>();
	if(!simulationCell) return;

	AffineTransformation cellMatrix = simulationCell->cellMatrix();
	Vector3 offset = cellMatrix * Point3(0.5, 0.5, 0.5) - Point3::Origin();
	if(offset == Vector3::Zero()) return;

	cellMatrix.translation() -= offset;
	simulationCell->setCellMatrix(cellMatrix);

	if(_particles) {
		if(PropertyAccess<Point3> positions = _particles->getMutableProperty(ParticlesObject::PositionProperty)) {
			for(Point3& p : positions)
				p -= offset;
		}
	}
}

/******************************************************************************
* Finalizes the particle data loaded by a sub-class.
******************************************************************************/
void ParticleImporter::FrameLoader::loadFile()
{
	if(isCanceled())
		return;

	StandardFrameLoader::loadFile();

	// Automatically generate the 'Velocity Magnitude' property if the 'Velocity' vector property was loaded from the input file.
	computeVelocityMagnitude();

	// Center the simulation cell on the coordinate origin if requested.
	if(_recenterCell)
		recenterSimulationCell();

#ifdef OVITO_DEBUG
	if(_particles) _particles->verifyIntegrity();
	if(_bonds) _bonds->verifyIntegrity();
	if(_angles) _angles->verifyIntegrity();
	if(_dihedrals) _dihedrals->verifyIntegrity();
	if(_impropers) _impropers->verifyIntegrity();
#endif
}

/******************************************************************************
* Is called when importing multiple files of different formats.
******************************************************************************/
bool ParticleImporter::importFurtherFiles(std::vector<std::pair<QUrl, OORef<FileImporter>>> sourceUrlsAndImporters, ImportMode importMode, bool autodetectFileSequences, PipelineSceneNode* pipeline)
{
	OVITO_ASSERT(!sourceUrlsAndImporters.empty());
	OORef<ParticleImporter> nextImporter = dynamic_object_cast<ParticleImporter>(sourceUrlsAndImporters.front().second);
	if(this->isTrajectoryFormat() == false && nextImporter && nextImporter->isTrajectoryFormat() == true) {

		// Create a new file source for loading the trajectory.
		OORef<FileSource> fileSource = OORef<FileSource>::create(dataset(), Application::instance()->executionContext());

		// Concatenate all files from the input list having the same file format into one sequence,
		// which gets handled by the trajectory importer.
		std::vector<QUrl> sourceUrls;
		sourceUrls.push_back(std::move(sourceUrlsAndImporters.front().first));
		auto iter = std::next(sourceUrlsAndImporters.begin());
		for(; iter != sourceUrlsAndImporters.end(); ++iter) {
			if(iter->second->getOOClass() != nextImporter->getOOClass())
				break;
			sourceUrls.push_back(std::move(iter->first));		
		}
		sourceUrlsAndImporters.erase(sourceUrlsAndImporters.begin(), iter);

		// Set the input file location(s) and importer.
		if(!fileSource->setSource(std::move(sourceUrls), nextImporter, autodetectFileSequences))
			return {};

		// Create a modifier for injecting the trajectory data into the existing pipeline.
		OORef<LoadTrajectoryModifier> loadTrjMod = new LoadTrajectoryModifier(dataset());
		loadTrjMod->setTrajectorySource(std::move(fileSource));
		pipeline->applyModifier(loadTrjMod);

		if(sourceUrlsAndImporters.empty())
			return true;
	}
	return FileSourceImporter::importFurtherFiles(std::move(sourceUrlsAndImporters), importMode, autodetectFileSequences, pipeline);
}

}	// End of namespace
}	// End of namespace
