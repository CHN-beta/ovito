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
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "BondsObject.h"
#include "BondType.h"
#include "ParticlesObject.h"
#include "ParticleBondMap.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsObject);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsObject::BondsObject(DataSet* dataset) : PropertyContainer(dataset)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void BondsObject::initializeObject(ExecutionContext executionContext)
{
	// Assign the default data object identifier.
	setIdentifier(OOClass().pythonName());

	// Create and attach a default visualization element for rendering the bonds.
	if(!visElement())
		setVisElement(OORef<BondsVis>::create(dataset(), executionContext));

	PropertyContainer::initializeObject(executionContext);
}

/******************************************************************************
* Determines the PBC shift vectors for bonds using the minimum image convention.
******************************************************************************/
void BondsObject::generatePeriodicImageProperty(const ParticlesObject* particles, const SimulationCellObject* simulationCellObject)
{
	ConstPropertyAccess<Point3> posProperty = particles->getProperty(ParticlesObject::PositionProperty);
	if(!posProperty) return;

	ConstPropertyAccess<ParticleIndexPair> bondTopologyProperty = getProperty(BondsObject::TopologyProperty);
	if(!bondTopologyProperty) return;

	if(!simulationCellObject)
		return;
	std::array<bool,3> pbcFlags = simulationCellObject->pbcFlags();
	if(!pbcFlags[0] && !pbcFlags[1] && !pbcFlags[2])
		return;
	const AffineTransformation inverseCellMatrix = simulationCellObject->reciprocalCellMatrix();

	auto topoIter = bondTopologyProperty.begin();
	PropertyAccess<Vector3I> bondPeriodicImageProperty = createProperty(BondsObject::PeriodicImageProperty, false, ExecutionContext::Scripting);
	for(Vector3I& pbcVec : bondPeriodicImageProperty) {
		size_t particleIndex1 = (*topoIter)[0];
		size_t particleIndex2 = (*topoIter)[1];
		pbcVec.setZero();
		if(particleIndex1 < posProperty.size() && particleIndex2 < posProperty.size()) {
			const Point3& p1 = posProperty[particleIndex1];
			const Point3& p2 = posProperty[particleIndex2];
			Vector3 delta = p1 - p2;
			for(size_t dim = 0; dim < 3; dim++) {
				if(pbcFlags[dim])
					pbcVec[dim] = std::lround(inverseCellMatrix.prodrow(delta, dim));
			}
		}
		++topoIter;
	}
}

/******************************************************************************
* Creates new bonds making sure bonds are not created twice.
******************************************************************************/
size_t BondsObject::addBonds(const std::vector<Bond>& newBonds, BondsVis* bondsVis, const ParticlesObject* particles, ExecutionContext executionContext, const std::vector<PropertyPtr>& bondProperties, DataOORef<const BondType> bondType)
{
	OVITO_ASSERT(isSafeToModify());

	if(bondsVis)
		setVisElement(bondsVis);

	// Are there existing bonds?
	if(elementCount() == 0) {
		setElementCount(newBonds.size());

		// Create essential bond properties.
		PropertyAccess<ParticleIndexPair> topologyProperty = createProperty(BondsObject::TopologyProperty, false, executionContext);
		PropertyAccess<Vector3I> periodicImageProperty = createProperty(BondsObject::PeriodicImageProperty, false, executionContext);
		PropertyObject* bondTypeProperty = bondType ? createProperty(BondsObject::TypeProperty, false, executionContext) : nullptr;

		// Transfer per-bond data into the standard property arrays.
		auto t = topologyProperty.begin();
		auto pbc = periodicImageProperty.begin();
		for(const Bond& bond : newBonds) {
			OVITO_ASSERT(!particles || bond.index1 < particles->elementCount());
			OVITO_ASSERT(!particles || bond.index2 < particles->elementCount());
			(*t)[0] = bond.index1;
			(*t)[1] = bond.index2;
			++t;
			*pbc++ = bond.pbcShift;
		}
		topologyProperty.reset();
		periodicImageProperty.reset();

		// Insert bond type.
		if(bondTypeProperty) {
			bondTypeProperty->fill<int>(bondType->numericId());
			bondTypeProperty->addElementType(std::move(bondType));
		}

		// Insert other bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondsObject::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondsObject::PeriodicImageProperty);
			OVITO_ASSERT(!bondTypeProperty || bprop->type() != BondsObject::TypeProperty);
			createProperty(bprop);
		}

		return newBonds.size();
	}
	else {
		// This is needed to determine which bonds already exist.
		ParticleBondMap bondMap(*this);

		// Check which bonds are new and need to be merged.
		size_t originalBondCount = elementCount();
		size_t outputBondCount = originalBondCount;
		std::vector<size_t> mapping(newBonds.size());
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			// Check if there is already a bond like this.
			const Bond& bond = newBonds[bondIndex];
			auto existingBondIndex = bondMap.findBond(bond);
			if(existingBondIndex == originalBondCount) {
				// It's a new bond.
				mapping[bondIndex] = outputBondCount;
				outputBondCount++;
			}
			else {
				// It's an already existing bond.
				mapping[bondIndex] = existingBondIndex;
			}
		}
		if(outputBondCount == originalBondCount)
			return 0;

		// Resize the existing property arrays.
		setElementCount(outputBondCount);

		PropertyAccess<ParticleIndexPair> newBondsTopology = expectMutableProperty(BondsObject::TopologyProperty);
		PropertyAccess<Vector3I> newBondsPeriodicImages = createProperty(BondsObject::PeriodicImageProperty, true, executionContext);
		PropertyAccess<int> newBondTypeProperty = bondType ? createProperty(BondsObject::TypeProperty, true, executionContext) : nullptr;

		if(newBondTypeProperty && !newBondTypeProperty.buffer()->elementType(bondType->numericId()))
			newBondTypeProperty.buffer()->addElementType(bondType);

		// Copy bonds information into the extended arrays.
		for(size_t bondIndex = 0; bondIndex < newBonds.size(); bondIndex++) {
			if(mapping[bondIndex] >= originalBondCount) {
				const Bond& bond = newBonds[bondIndex];
				OVITO_ASSERT(!particles || bond.index1 < particles->elementCount());
				OVITO_ASSERT(!particles || bond.index2 < particles->elementCount());
				newBondsTopology[mapping[bondIndex]][0] = bond.index1;
				newBondsTopology[mapping[bondIndex]][1] = bond.index2;
				newBondsPeriodicImages[mapping[bondIndex]] = bond.pbcShift;
				if(newBondTypeProperty) 
					newBondTypeProperty[mapping[bondIndex]] = bondType->numericId();
			}
		}
		newBondsTopology.reset();
		newBondsPeriodicImages.reset();
		newBondTypeProperty.reset();

		// Initialize property values of existing properties for new bonds.
		for(PropertyObject* bondPropertyObject : makePropertiesMutable()) {
			if(bondPropertyObject->type() == BondsObject::ColorProperty) {
				if(particles) {
					ConstPropertyPtr bondColors;
					if(particles->bonds() != this) {
						// Create a temporary copy of the ParticlesObject, which is assigned this BondsObject. 
						DataOORef<ParticlesObject> particlesCopy = DataOORef<ParticlesObject>::makeCopy(particles);
						particlesCopy->setBonds(this);
						bondColors = particlesCopy->inputBondColors(true);
					}
					else {
						bondColors = particles->inputBondColors(true);
					}
					bondPropertyObject->copyRangeFrom(*bondColors, originalBondCount, originalBondCount, outputBondCount - originalBondCount);
				}
			}
		}

		// Merge new bond properties.
		for(const auto& bprop : bondProperties) {
			OVITO_ASSERT(bprop->size() == newBonds.size());
			OVITO_ASSERT(bprop->type() != BondsObject::TopologyProperty);
			OVITO_ASSERT(bprop->type() != BondsObject::PeriodicImageProperty);
			OVITO_ASSERT(!bondType || bprop->type() != BondsObject::TypeProperty);

			PropertyObject* propertyObject;
			if(bprop->type() != BondsObject::UserProperty) {
				propertyObject = createProperty(bprop->type(), true, executionContext);
			}
			else {
				propertyObject = createProperty(bprop->name(), bprop->dataType(), bprop->componentCount(), bprop->stride(), true);
			}

			// Copy bond property data.
			propertyObject->mappedCopyFrom(*bprop, mapping);
		}

		return outputBondCount - originalBondCount;
	}
}

/******************************************************************************
* Creates a storage object for standard bond properties.
******************************************************************************/
PropertyPtr BondsObject::OOMetaClass::createStandardPropertyInternal(DataSet* dataset, size_t bondsCount, int type, bool initializeMemory, ExecutionContext executionContext, const ConstDataObjectPath& containerPath) const
{
	// Initialize memory if requested.
	if(initializeMemory && containerPath.size() >= 2) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath[containerPath.size()-2])) {
				ConstPropertyPtr property = particles->inputBondColors();
				OVITO_ASSERT(property && property->size() == bondsCount && property->type() == ColorProperty);
				return std::move(property).makeMutable();
			}
		}
	}

	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case TypeProperty:
	case SelectionProperty:
		dataType = PropertyObject::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case LengthProperty:
	case TransparencyProperty:
		dataType = PropertyObject::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case ColorProperty:
		dataType = PropertyObject::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case TopologyProperty:
		dataType = PropertyObject::Int64;
		componentCount = 2;
		stride = componentCount * sizeof(qlonglong);
		break;
	case PeriodicImageProperty:
		dataType = PropertyObject::Int;
		componentCount = 3;
		stride = componentCount * sizeof(int);
		break;
	default:
		OVITO_ASSERT_MSG(false, "BondsObject::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard bond property type: %1").arg(type));
	}
	
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = PropertyPtr::create(dataset, executionContext, bondsCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		property->fillZero();
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void BondsObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a BondPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<BondPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, BondPropertyReference>();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QMetaType::registerComparators<BondPropertyReference>();
#endif

	setPropertyClassDisplayName(tr("Bonds"));
	setElementDescriptionName(QStringLiteral("bonds"));
	setPythonName(QStringLiteral("bonds"));

	const QStringList emptyList;
	const QStringList abList = QStringList() << "A" << "B";
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(TypeProperty, tr("Bond Type"), PropertyObject::Int, emptyList, &BondType::OOClass(), tr("Bond types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyObject::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyObject::Float, rgbList, nullptr, tr("Bond colors"));
	registerStandardProperty(LengthProperty, tr("Length"), PropertyObject::Float, emptyList);
	registerStandardProperty(TopologyProperty, tr("Topology"), PropertyObject::Int64, abList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), PropertyObject::Int, xyzList);
	registerStandardProperty(TransparencyProperty, tr("Transparency"), PropertyObject::Float, emptyList);
}

/******************************************************************************
* Returns the default color for a numeric type ID.
******************************************************************************/
Color BondsObject::OOMetaClass::getElementTypeDefaultColor(const PropertyReference& property, const QString& typeName, int numericTypeId, ExecutionContext executionContext) const
{
	if(property.type() == BondsObject::TypeProperty) {

		// Initial standard colors assigned to new bond types:
		static const Color defaultTypeColors[] = {
			Color(1.0,  1.0,  0.0), // 0
			Color(0.7,  0.0,  1.0), // 1
			Color(0.2,  1.0,  1.0), // 2
			Color(1.0,  0.4,  1.0), // 3
			Color(0.4,  1.0,  0.4), // 4
			Color(1.0,  0.4,  0.4), // 5
			Color(0.4,  0.4,  1.0), // 6
			Color(1.0,  1.0,  0.7), // 7
			Color(0.97, 0.97, 0.97) // 8
		};
		return defaultTypeColors[std::abs(numericTypeId) % (sizeof(defaultTypeColors) / sizeof(defaultTypeColors[0]))];
	}

	return PropertyContainerClass::getElementTypeDefaultColor(property, typeName, numericTypeId, executionContext);
}

/******************************************************************************
* Returns the index of the element that was picked in a viewport.
******************************************************************************/
std::pair<size_t, ConstDataObjectPath> BondsObject::OOMetaClass::elementFromPickResult(const ViewportPickResult& pickResult) const
{
	// Check if a bond was picked.
	if(BondPickInfo* pickInfo = dynamic_object_cast<BondPickInfo>(pickResult.pickInfo())) {
		size_t bondIndex = pickResult.subobjectId() / 2;
		if(pickInfo->particles()->bonds() && bondIndex < pickInfo->particles()->bonds()->elementCount()) {
			return std::make_pair(bondIndex, ConstDataObjectPath{{pickInfo->particles(), pickInfo->particles()->bonds()}});
		}
	}

	return std::pair<size_t, DataObjectPath>(std::numeric_limits<size_t>::max(), {});
}

/******************************************************************************
* Tries to remap an index from one property container to another, considering the
* possibility that elements may have been added or removed.
******************************************************************************/
size_t BondsObject::OOMetaClass::remapElementIndex(const ConstDataObjectPath& source, size_t elementIndex, const ConstDataObjectPath& dest) const
{
	const BondsObject* sourceBonds = static_object_cast<BondsObject>(source.back());
	const BondsObject* destBonds = static_object_cast<BondsObject>(dest.back());
	const ParticlesObject* sourceParticles = dynamic_object_cast<ParticlesObject>(source.size() >= 2 ? source[source.size()-2] : nullptr);
	const ParticlesObject* destParticles = dynamic_object_cast<ParticlesObject>(dest.size() >= 2 ? dest[dest.size()-2] : nullptr);
	if(sourceParticles && destParticles) {

		// Make sure the topology information is present.
		if(ConstPropertyAccess<ParticleIndexPair> sourceTopology = sourceBonds->getProperty(TopologyProperty)) {
			if(ConstPropertyAccess<ParticleIndexPair> destTopology = destBonds->getProperty(TopologyProperty)) {

				// If unique IDs are available try to use them to look up the bond in the other data collection.
				if(ConstPropertyAccess<qlonglong> sourceIdentifiers = sourceParticles->getProperty(ParticlesObject::IdentifierProperty)) {
					if(ConstPropertyAccess<qlonglong> destIdentifiers = destParticles->getProperty(ParticlesObject::IdentifierProperty)) {
						size_t index_a = sourceTopology[elementIndex][0];
						size_t index_b = sourceTopology[elementIndex][1];
						if(index_a < sourceIdentifiers.size() && index_b < sourceIdentifiers.size()) {
							qlonglong id_a = sourceIdentifiers[index_a];
							qlonglong id_b = sourceIdentifiers[index_b];

							// Quick test if the bond storage order is the same.
							if(elementIndex < destTopology.size()) {
								size_t index2_a = destTopology[elementIndex][0];
								size_t index2_b = destTopology[elementIndex][1];
								if(index2_a < destIdentifiers.size() && index2_b < destIdentifiers.size()) {
									if(destIdentifiers[index2_a] == id_a && destIdentifiers[index2_b] == id_b) {
										return elementIndex;
									}
								}
							}

							// Determine the indices of the two particles connected by the bond.
							size_t index2_a = boost::find(destIdentifiers, id_a) - destIdentifiers.cbegin();
							size_t index2_b = boost::find(destIdentifiers, id_b) - destIdentifiers.cbegin();
							if(index2_a < destIdentifiers.size() && index2_b < destIdentifiers.size()) {
								// Go through the whole bonds list to see if there is a bond connecting the particles with
								// the same IDs.
								for(const auto& bond : destTopology) {
									if((bond[0] == index2_a && bond[1] == index2_b) || (bond[0] == index2_b && bond[1] == index2_a)) {
										return (&bond - destTopology.cbegin());
									}
								}
							}
						}

						// Give up.
						return PropertyContainerClass::remapElementIndex(source, elementIndex, dest);
					}
				}

				// Try to find matching bond based on particle indices alone.
				if(ConstPropertyAccess<Point3> sourcePos = sourceParticles->getProperty(ParticlesObject::PositionProperty)) {
					if(ConstPropertyAccess<Point3> destPos = destParticles->getProperty(ParticlesObject::PositionProperty)) {
						size_t index_a = sourceTopology[elementIndex][0];
						size_t index_b = sourceTopology[elementIndex][1];
						if(index_a < sourcePos.size() && index_b < sourcePos.size()) {

							// Quick check if number of particles and bonds didn't change.
							if(sourcePos.size() == destPos.size() && sourceTopology.size() == destTopology.size()) {
								size_t index2_a = destTopology[elementIndex][0];
								size_t index2_b = destTopology[elementIndex][1];
								if(index_a == index2_a && index_b == index2_b) {
									return elementIndex;
								}
							}

							// Find matching bond by means of particle positions.
							const Point3& pos_a = sourcePos[index_a];
							const Point3& pos_b = sourcePos[index_b];
							size_t index2_a = boost::find(destPos, pos_a) - destPos.cbegin();
							size_t index2_b = boost::find(destPos, pos_b) - destPos.cbegin();
							if(index2_a < destPos.size() && index2_b < destPos.size()) {
								// Go through the whole bonds list to see if there is a bond connecting the particles with
								// the same positions.
								for(const auto& bond : destTopology) {
									if((bond[0] == index2_a && bond[1] == index2_b) || (bond[0] == index2_b && bond[1] == index2_a)) {
										return (&bond - destTopology.cbegin());
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Give up.
	return PropertyContainerClass::remapElementIndex(source, elementIndex, dest);
}

/******************************************************************************
* Determines which elements are located within the given
* viewport fence region (=2D polygon).
******************************************************************************/
boost::dynamic_bitset<> BondsObject::OOMetaClass::viewportFenceSelection(const QVector<Point2>& fence, const ConstDataObjectPath& objectPath, PipelineSceneNode* node, const Matrix4& projectionTM) const
{
	const BondsObject* bonds = static_object_cast<BondsObject>(objectPath.back());
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectPath.size() >= 2 ? objectPath[objectPath.size()-2] : nullptr);

	if(particles) {
		if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = bonds->getProperty(BondsObject::TopologyProperty)) {
			if(ConstPropertyAccess<Point3> posProperty = particles->getProperty(ParticlesObject::PositionProperty)) {

				if(!bonds->visElement() || bonds->visElement()->isEnabled() == false)
					node->throwException(tr("Cannot select bonds while the corresponding visual element is disabled. Please enable the display of bonds first."));

				boost::dynamic_bitset<> fullSelection(topologyProperty.size());
				QMutex mutex;
				parallelForChunks(topologyProperty.size(), [topologyProperty, posProperty, &projectionTM, &fence, &mutex, &fullSelection](size_t startIndex, size_t chunkSize) {
					boost::dynamic_bitset<> selection(fullSelection.size());
					for(size_t index = startIndex; chunkSize != 0; chunkSize--, index++) {
						const ParticleIndexPair& t = topologyProperty[index];
						int insideCount = 0;
						for(size_t i = 0; i < 2; i++) {
							if(t[i] >= (qlonglong)posProperty.size()) continue;
							const Point3& p = posProperty[t[i]];

							// Project particle center to screen coordinates.
							Point3 projPos = projectionTM * p;

							// Perform z-clipping.
							if(std::abs(projPos.z()) >= FloatType(1))
								break;

							// Perform point-in-polygon test.
							int intersectionsLeft = 0;
							int intersectionsRight = 0;
							for(auto p2 = fence.constBegin(), p1 = p2 + (fence.size()-1); p2 != fence.constEnd(); p1 = p2++) {
								if(p1->y() == p2->y()) continue;
								if(projPos.y() >= p1->y() && projPos.y() >= p2->y()) continue;
								if(projPos.y() < p1->y() && projPos.y() < p2->y()) continue;
								FloatType xint = (projPos.y() - p2->y()) / (p1->y() - p2->y()) * (p1->x() - p2->x()) + p2->x();
								if(xint >= projPos.x())
									intersectionsRight++;
								else
									intersectionsLeft++;
							}
							if(intersectionsRight & 1)
								insideCount++;
						}
						if(insideCount == 2)
							selection.set(index);
					}
					// Transfer thread-local results to output bit array.
					QMutexLocker locker(&mutex);
					fullSelection |= selection;
				});

				return fullSelection;
			}
		}
	}

	// Give up.
	return PropertyContainerClass::viewportFenceSelection(fence, objectPath, node, projectionTM);
}

}	// End of namespace
}	// End of namespace
