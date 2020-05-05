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
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "ParticlesReplicateModifierDelegate.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesReplicateModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesReplicateModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus ParticlesReplicateModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	ReplicateModifier* mod = static_object_cast<ReplicateModifier>(modifier);
	const ParticlesObject* inputParticles = state.getObject<ParticlesObject>();

	std::array<int,3> nPBC;
	nPBC[0] = std::max(mod->numImagesX(),1);
	nPBC[1] = std::max(mod->numImagesY(),1);
	nPBC[2] = std::max(mod->numImagesZ(),1);

	// Calculate new number of particles.
	size_t numCopies = nPBC[0] * nPBC[1] * nPBC[2];
	if(numCopies <= 1 || !inputParticles || inputParticles->elementCount() == 0)
		return PipelineStatus::Success;

	// Extend particle property arrays.
	size_t oldParticleCount = inputParticles->elementCount();
	size_t newParticleCount = oldParticleCount * numCopies;

	const SimulationCell cell = state.expectObject<SimulationCellObject>()->data();
	const AffineTransformation& cellMatrix = cell.matrix();

	// Ensure that the particles can be modified.
	ParticlesObject* outputParticles = state.makeMutable(inputParticles);
	outputParticles->replicate(numCopies);

	// Replicate particle property values.
	Box3I newImages = mod->replicaRange();
	for(PropertyObject* property : outputParticles->properties()) {
		OVITO_ASSERT(property->size() == newParticleCount);

		// Shift particle positions by the periodicity vector.
		if(property->type() == ParticlesObject::PositionProperty) {
			PropertyAccess<Point3> positionArray(property);
			Point3* p = positionArray.begin();
			for(int imageX = newImages.minc.x(); imageX <= newImages.maxc.x(); imageX++) {
				for(int imageY = newImages.minc.y(); imageY <= newImages.maxc.y(); imageY++) {
					for(int imageZ = newImages.minc.z(); imageZ <= newImages.maxc.z(); imageZ++) {
						if(imageX != 0 || imageY != 0 || imageZ != 0) {
							const Vector3 imageDelta = cellMatrix * Vector3(imageX, imageY, imageZ);
							for(size_t i = 0; i < oldParticleCount; i++)
								*p++ += imageDelta;
						}
						else {
							p += oldParticleCount;
						}
					}
				}
			}
		}

		// Assign unique IDs to duplicated particles.
		if(mod->uniqueIdentifiers() && (property->type() == ParticlesObject::IdentifierProperty || property->type() == ParticlesObject::MoleculeProperty)) {
			PropertyAccess<qlonglong> propertyData(property);
			auto minmax = std::minmax_element(propertyData.cbegin(), propertyData.cbegin() + oldParticleCount);
			auto minID = *minmax.first;
			auto maxID = *minmax.second;
			for(size_t c = 1; c < numCopies; c++) {
				auto offset = (maxID - minID + 1) * c;
				for(auto id = propertyData.begin() + c * oldParticleCount, id_end = id + oldParticleCount; id != id_end; ++id)
					*id += offset;
			}
		}
	}

	// Replicate bonds.
	if(outputParticles->bonds()) {
		size_t oldBondCount = outputParticles->bonds()->elementCount();
		size_t newBondCount = oldBondCount * numCopies;

		ConstPropertyAccessAndRef<Vector3I> oldPeriodicImages = outputParticles->bonds()->getProperty(BondsObject::PeriodicImageProperty);

		// Replicate bond property values.
		outputParticles->makeBondsMutable();
		outputParticles->bonds()->makePropertiesMutable();
		outputParticles->bonds()->replicate(numCopies);
		for(PropertyObject* property : outputParticles->bonds()->properties()) {
			OVITO_ASSERT(property->size() == newBondCount);

			size_t destinationIndex = 0;
			Point3I image;

			// Special handling for the topology property.
			if(property->type() == BondsObject::TopologyProperty) {
				PropertyAccess<ParticleIndexPair> topologyArray(property);
				for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
					for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
						for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
							for(size_t bindex = 0; bindex < oldBondCount; bindex++, destinationIndex++) {
								Point3I newImage;
								for(size_t dim = 0; dim < 3; dim++) {
									int i = image[dim] + (oldPeriodicImages ? oldPeriodicImages[bindex][dim] : 0) - newImages.minc[dim];
									newImage[dim] = SimulationCell::modulo(i, nPBC[dim]) + newImages.minc[dim];
								}
								OVITO_ASSERT(newImage.x() >= newImages.minc.x() && newImage.x() <= newImages.maxc.x());
								OVITO_ASSERT(newImage.y() >= newImages.minc.y() && newImage.y() <= newImages.maxc.y());
								OVITO_ASSERT(newImage.z() >= newImages.minc.z() && newImage.z() <= newImages.maxc.z());
								size_t imageIndex1 =  ((image.x()-newImages.minc.x()) * nPBC[1] * nPBC[2])
													+ ((image.y()-newImages.minc.y()) * nPBC[2])
													+  (image.z()-newImages.minc.z());
								size_t imageIndex2 =  ((newImage.x()-newImages.minc.x()) * nPBC[1] * nPBC[2])
													+ ((newImage.y()-newImages.minc.y()) * nPBC[2])
													+  (newImage.z()-newImages.minc.z());
								topologyArray[destinationIndex][0] += imageIndex1 * oldParticleCount;
								topologyArray[destinationIndex][1] += imageIndex2 * oldParticleCount;
								OVITO_ASSERT(topologyArray[destinationIndex][0] < newParticleCount);
								OVITO_ASSERT(topologyArray[destinationIndex][1] < newParticleCount);
							}
						}
					}
				}
			}
			else if(property->type() == BondsObject::PeriodicImageProperty) {
				// Special handling for the PBC shift vector property.
				OVITO_ASSERT(oldPeriodicImages);
				PropertyAccess<Vector3I> pbcImagesArray(property);
				for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
					for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
						for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
							for(size_t bindex = 0; bindex < oldBondCount; bindex++, destinationIndex++) {
								Vector3I newShift;
								for(size_t dim = 0; dim < 3; dim++) {
									int i = image[dim] + oldPeriodicImages[bindex][dim] - newImages.minc[dim];
									newShift[dim] = i >= 0 ? (i / nPBC[dim]) : ((i-nPBC[dim]+1) / nPBC[dim]);
									if(!mod->adjustBoxSize())
										newShift[dim] *= nPBC[dim];
								}
								pbcImagesArray[destinationIndex] = newShift;
							}
						}
					}
				}
			}
		}
	}

	// Replicate angles.
	if(outputParticles->angles()) {
		size_t oldAngleCount = outputParticles->angles()->elementCount();

		// Replicate angle property values.
		outputParticles->makeAnglesMutable();
		outputParticles->angles()->makePropertiesMutable();
		outputParticles->angles()->replicate(numCopies);
		for(PropertyObject* property : outputParticles->angles()->properties()) {
			size_t destinationIndex = 0;
			Point3I image;

			// Special handling for the topology property.
			if(property->type() == AnglesObject::TopologyProperty) {
				PropertyAccess<ParticleIndexTriplet> topologyArray(property);
				ConstPropertyAccess<Point3> positionArray(inputParticles->expectProperty(ParticlesObject::PositionProperty));
				for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
					for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
						for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
							for(size_t index = 0; index < oldAngleCount; index++, destinationIndex++) {
								qlonglong referenceParticle = topologyArray[destinationIndex][1];
								for(qlonglong& pindex : topologyArray[destinationIndex]) {
									Point3I newImage = image;
									if(pindex >= 0 && pindex < positionArray.size() && referenceParticle >= 0 && referenceParticle < positionArray.size()) {
										Vector3 delta = positionArray[pindex] - positionArray[referenceParticle];
										for(size_t dim = 0; dim < 3; dim++) {
											if(cell.pbcFlags()[dim]) {
												int imageDelta = (int)std::floor(cell.inverseMatrix().prodrow(delta, dim) + FloatType(0.5));
												int i = image[dim] - newImages.minc[dim] - imageDelta;
												newImage[dim] = SimulationCell::modulo(i, nPBC[dim]) + newImages.minc[dim];
											}
										}
									}
									int imageIndex =   ((newImage.x() - newImages.minc.x()) * nPBC[1] * nPBC[2])
													 + ((newImage.y() - newImages.minc.y()) * nPBC[2])
													 +  (newImage.z() - newImages.minc.z());
									pindex += imageIndex * oldParticleCount;
									OVITO_ASSERT(pindex < newParticleCount);
								}
							}
						}
					}
				}
			}
		}
	}

	// Replicate dihedrals.
	if(outputParticles->dihedrals()) {
		size_t oldDihedralCount = outputParticles->dihedrals()->elementCount();

		// Replicate dihedral property values.
		outputParticles->makeDihedralsMutable();
		outputParticles->dihedrals()->makePropertiesMutable();
		outputParticles->dihedrals()->replicate(numCopies);
		for(PropertyObject* property : outputParticles->dihedrals()->properties()) {
			size_t destinationIndex = 0;
			Point3I image;

			// Special handling for the topology property.
			if(property->type() == DihedralsObject::TopologyProperty) {
				PropertyAccess<ParticleIndexQuadruplet> topologyArray(property);
				ConstPropertyAccess<Point3> positionArray(inputParticles->expectProperty(ParticlesObject::PositionProperty));
				for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
					for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
						for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
							for(size_t index = 0; index < oldDihedralCount; index++, destinationIndex++) {
								qlonglong referenceParticle = topologyArray[destinationIndex][1];
								for(qlonglong& pindex : topologyArray[destinationIndex]) {
									Point3I newImage = image;
									if(pindex >= 0 && pindex < positionArray.size() && referenceParticle >= 0 && referenceParticle < positionArray.size()) {
										Vector3 delta = positionArray[pindex] - positionArray[referenceParticle];
										for(size_t dim = 0; dim < 3; dim++) {
											if(cell.pbcFlags()[dim]) {
												int imageDelta = (int)std::floor(cell.inverseMatrix().prodrow(delta, dim) + FloatType(0.5));
												int i = image[dim] - newImages.minc[dim] - imageDelta;
												newImage[dim] = SimulationCell::modulo(i, nPBC[dim]) + newImages.minc[dim];
											}
										}
									}
									int imageIndex =   ((newImage.x() - newImages.minc.x()) * nPBC[1] * nPBC[2])
													 + ((newImage.y() - newImages.minc.y()) * nPBC[2])
													 +  (newImage.z() - newImages.minc.z());
									pindex += imageIndex * oldParticleCount;
									OVITO_ASSERT(pindex < newParticleCount);
								}
							}
						}
					}
				}
			}
		}
	}

	// Replicate impropers.
	if(outputParticles->impropers()) {
		size_t oldImproperCount = outputParticles->impropers()->elementCount();

		// Replicate improper property values.
		outputParticles->makeImpropersMutable();
		outputParticles->impropers()->makePropertiesMutable();
		outputParticles->impropers()->replicate(numCopies);
		for(PropertyObject* property : outputParticles->impropers()->properties()) {
			size_t destinationIndex = 0;
			Point3I image;

			// Special handling for the topology property.
			if(property->type() == ImpropersObject::TopologyProperty) {
				PropertyAccess<ParticleIndexQuadruplet> topologyArray(property);
				ConstPropertyAccess<Point3> positionArray(inputParticles->expectProperty(ParticlesObject::PositionProperty));
				for(image[0] = newImages.minc.x(); image[0] <= newImages.maxc.x(); image[0]++) {
					for(image[1] = newImages.minc.y(); image[1] <= newImages.maxc.y(); image[1]++) {
						for(image[2] = newImages.minc.z(); image[2] <= newImages.maxc.z(); image[2]++) {
							for(size_t index = 0; index < oldImproperCount; index++, destinationIndex++) {
								qlonglong referenceParticle = topologyArray[destinationIndex][1];
								for(qlonglong& pindex : topologyArray[destinationIndex]) {
									Point3I newImage = image;
									if(pindex >= 0 && pindex < positionArray.size() && referenceParticle >= 0 && referenceParticle < positionArray.size()) {
										Vector3 delta = positionArray[pindex] - positionArray[referenceParticle];
										for(size_t dim = 0; dim < 3; dim++) {
											if(cell.pbcFlags()[dim]) {
												int imageDelta = (int)std::floor(cell.inverseMatrix().prodrow(delta, dim) + FloatType(0.5));
												int i = image[dim] - newImages.minc[dim] - imageDelta;
												newImage[dim] = SimulationCell::modulo(i, nPBC[dim]) + newImages.minc[dim];
											}
										}
									}
									int imageIndex =   ((newImage.x() - newImages.minc.x()) * nPBC[1] * nPBC[2])
													 + ((newImage.y() - newImages.minc.y()) * nPBC[2])
													 +  (newImage.z() - newImages.minc.z());
									pindex += imageIndex * oldParticleCount;
									OVITO_ASSERT(pindex < newParticleCount);
								}
							}
						}
					}
				}
			}
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
