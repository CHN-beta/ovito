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
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "CreateBondsModifier.h"

#include <boost/range/numeric.hpp>

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(CreateBondsModifier);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, cutoffMode);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, uniformCutoff);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, pairwiseCutoffs);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, minimumCutoff);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, vdwPrefactor);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, onlyIntraMoleculeBonds);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, skipHydrogenHydrogenBonds);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, autoDisableBondDisplay);
DEFINE_REFERENCE_FIELD(CreateBondsModifier, bondType);
DEFINE_REFERENCE_FIELD(CreateBondsModifier, bondsVis);
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, cutoffMode, "Cutoff mode");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, uniformCutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, pairwiseCutoffs, "Pair-wise cutoffs");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, minimumCutoff, "Lower cutoff");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, vdwPrefactor, "VdW prefactor");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, onlyIntraMoleculeBonds, "Suppress inter-molecular bonds");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, bondType, "Bond type");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, bondsVis, "Visual element");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, skipHydrogenHydrogenBonds, "Don't generate H-H bonds");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, autoDisableBondDisplay, "Auto-disable bond display");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, uniformCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, minimumCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, vdwPrefactor, PercentParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateBondsModifier::CreateBondsModifier(ObjectCreationParams params) : AsynchronousModifier(params),
	_cutoffMode(UniformCutoff),
	_uniformCutoff(3.2),
	_onlyIntraMoleculeBonds(false),
	_minimumCutoff(0),
	_autoDisableBondDisplay(true),
	_skipHydrogenHydrogenBonds(true),
	_vdwPrefactor(0.6) // Value 0.6 has been adopted from VMD source code.
{
	if(params.createSubObjects()) {
		// Create the bond type that will be assigned to the newly created bonds.
		setBondType(OORef<BondType>::create(params));
		bondType()->initializeType(BondPropertyReference(BondsObject::TypeProperty), params.loadUserDefaults());

		// Create the vis element for rendering the bonds generated by the modifier.
		setBondsVis(OORef<BondsVis>::create(params));
	}
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool CreateBondsModifier::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == bondsVis() && event.type() == ReferenceEvent::TargetEnabledOrDisabled && bondsVis()->isEnabled()) {
		// If the user explicitly re-enables the display of bonds, then the modifier should stop turning it off
		// again in the future.
		setAutoDisableBondDisplay(false);
	}
	return AsynchronousModifier::referenceEvent(source, event);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CreateBondsModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Sets the cutoff radius for a pair of particle types.
******************************************************************************/
void CreateBondsModifier::setPairwiseCutoff(const QVariant& typeA, const QVariant& typeB, FloatType cutoff)
{
	PairwiseCutoffsList newList = pairwiseCutoffs();
	if(cutoff > 0) {
		newList[qMakePair(typeA, typeB)] = cutoff;
		newList[qMakePair(typeB, typeA)] = cutoff;
	}
	else {
		newList.remove(qMakePair(typeA, typeB));
		newList.remove(qMakePair(typeB, typeA));
	}
	setPairwiseCutoffs(std::move(newList));
}

/******************************************************************************
* Returns the pair-wise cutoff radius for a pair of particle types.
******************************************************************************/
FloatType CreateBondsModifier::getPairwiseCutoff(const QVariant& typeA, const QVariant& typeB) const
{
	auto iter = pairwiseCutoffs().find(qMakePair(typeA, typeB));
	if(iter != pairwiseCutoffs().end()) return iter.value();
	iter = pairwiseCutoffs().find(qMakePair(typeB, typeA));
	if(iter != pairwiseCutoffs().end()) return iter.value();
	return 0;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateBondsModifier::initializeModifier(const ModifierInitializationRequest& request)
{
	AsynchronousModifier::initializeModifier(request);

	int bondTypeId = 1;
	const PipelineFlowState& input = request.modApp()->evaluateInputSynchronous(request);
	if(const ParticlesObject* particles = input.getObject<ParticlesObject>()) {
		// Adopt the upstream BondsVis object if there already is one.
		// Also choose a unique numeric bond type ID, which does not conflict with any existing bond type.
		if(const BondsObject* bonds = particles->bonds()) {
			if(BondsVis* bondsVis = bonds->visElement<BondsVis>()) {
				setBondsVis(bondsVis);
			}
			if(const PropertyObject* bondTypeProperty = bonds->getProperty(BondsObject::TypeProperty)) {
				bondTypeId = bondTypeProperty->generateUniqueElementTypeId();
			}
		}

		// Initialize the pair-wise cutoffs based on the van der Waals radii of the particle types.
		if(ExecutionContext::isInteractive() && pairwiseCutoffs().empty()) {
			if(const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty)) {
				PairwiseCutoffsList cutoffList;
				for(const ElementType* type1 : typeProperty->elementTypes()) {
					if(const ParticleType* ptype1 = dynamic_object_cast<ParticleType>(type1)) {
						if(ptype1->vdwRadius() > 0.0) {
							QVariant key1 = ptype1->name().isEmpty() ? QVariant::fromValue(ptype1->numericId()) : QVariant::fromValue(ptype1->name());
							for(const ElementType* type2 : typeProperty->elementTypes()) {
								if(const ParticleType* ptype2 = dynamic_object_cast<ParticleType>(type2)) {
									if(ptype2->vdwRadius() > 0.0 && (ptype1->name() != QStringLiteral("H") || ptype2->name() != QStringLiteral("H"))) {
										// Note: Prefactor 0.6 has been adopted from VMD source code.
										FloatType cutoff = 0.6 * (ptype1->vdwRadius() + ptype2->vdwRadius());
										QVariant key2 = ptype2->name().isEmpty() ? QVariant::fromValue(ptype2->numericId()) : QVariant::fromValue(ptype2->name());
										cutoffList[qMakePair(key1, key2)] = cutoff;
									}
								}
							}
						}
					}
				}
				setPairwiseCutoffs(std::move(cutoffList));
			}
		}
	}
	if(bondType() && bondType()->numericId() == 0) {
		bondType()->setNumericId(bondTypeId);
		bondType()->initializeType(BondPropertyReference(BondsObject::TypeProperty));
	}
}

/******************************************************************************
* Looks up a particle type in the type list based on the name or the numeric ID.
******************************************************************************/
const ElementType* CreateBondsModifier::lookupParticleType(const PropertyObject* typeProperty, const QVariant& typeSpecification)
{
	if(getQVariantTypeId(typeSpecification) == QMetaType::Int) {
		return typeProperty->elementType(typeSpecification.toInt());
	}
	else {
		const QString& name = typeSpecification.toString();
		for(const ElementType* type : typeProperty->elementTypes())
			if(type->nameOrNumericId() == name)
				return type;
		return nullptr;
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> CreateBondsModifier::createEngine(const ModifierEvaluationRequest& request, const PipelineFlowState& input)
{
	// Get modifier input.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// The neighbor list cutoff.
	FloatType maxCutoff = uniformCutoff();
	// The list of per-type VdW radii.
	std::vector<FloatType> typeVdWRadiusMap;
	// Flags indicating which type(s) are hydrogens.
	std::vector<bool> isHydrogenType;

	// Build table of pair-wise cutoff radii.
	const PropertyObject* typeProperty = nullptr;
	std::vector<std::vector<FloatType>> pairCutoffSquaredTable;
	if(cutoffMode() == PairCutoff) {
		maxCutoff = 0;
		typeProperty = particles->expectProperty(ParticlesObject::TypeProperty);
		if(typeProperty) {
			for(auto entry = pairwiseCutoffs().begin(); entry != pairwiseCutoffs().end(); ++entry) {
				FloatType cutoff = entry.value();
				if(cutoff > 0) {
					const ElementType* ptype1 = lookupParticleType(typeProperty, entry.key().first);
					const ElementType* ptype2 = lookupParticleType(typeProperty, entry.key().second);
					if(ptype1 && ptype2 && ptype1->numericId() >= 0 && ptype2->numericId() >= 0) {
						int id1 = ptype1->numericId();
						int id2 = ptype2->numericId();
						if((int)pairCutoffSquaredTable.size() <= std::max(id1, id2)) pairCutoffSquaredTable.resize(std::max(id1, id2) + 1);
						if((int)pairCutoffSquaredTable[id1].size() <= id2) pairCutoffSquaredTable[id1].resize(id2 + 1, FloatType(0));
						if((int)pairCutoffSquaredTable[id2].size() <= id1) pairCutoffSquaredTable[id2].resize(id1 + 1, FloatType(0));
						pairCutoffSquaredTable[id1][id2] = cutoff * cutoff;
						pairCutoffSquaredTable[id2][id1] = cutoff * cutoff;
						if(cutoff > maxCutoff) maxCutoff = cutoff;
					}
				}
			}
			if(maxCutoff <= 0)
				throwException(tr("At least one positive bond cutoff must be set for a valid pair of particle types."));
		}
	}
	else if(cutoffMode() == TypeRadiusCutoff) {
		maxCutoff = 0;
		if(vdwPrefactor() <= 0.0)
			throwException(tr("Van der Waal radius scaling factor must be positive."));
		typeProperty = particles->expectProperty(ParticlesObject::TypeProperty);
		if(typeProperty) {
			for(const ElementType* type : typeProperty->elementTypes()) {
				if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(type)) {
					if(ptype->vdwRadius() > 0.0 && ptype->numericId() >= 0) {
						if(ptype->vdwRadius() > maxCutoff)
							maxCutoff = ptype->vdwRadius();
						if(type->numericId() >= typeVdWRadiusMap.size())
							typeVdWRadiusMap.resize(type->numericId() + 1, 0.0);
						typeVdWRadiusMap[type->numericId()] = ptype->vdwRadius();
						if(skipHydrogenHydrogenBonds()) {
							if(type->numericId() >= isHydrogenType.size())
								isHydrogenType.resize(type->numericId() + 1, false);
							isHydrogenType[type->numericId()] = (ptype->name() == QStringLiteral("H"));
						}
					}
				}
			}
			maxCutoff *= vdwPrefactor() * 2.0;
			if(maxCutoff == 0.0)
				throwException(tr("The van der Waals (VdW) radii of all particle types are undefined or zero. Creating bonds based on the VdW radius requires at least one particle type with a positive radius value."));
		}
		OVITO_ASSERT(!typeVdWRadiusMap.empty());
	}
	if(maxCutoff <= 0.0)
		throwException(tr("Maximum bond cutoff range is zero. A positive value is required."));

	// Get molecule IDs.
	const PropertyObject* moleculeProperty = onlyIntraMoleculeBonds() ? particles->getProperty(ParticlesObject::MoleculeProperty) : nullptr;

	// Create the bonds object that will store the generated bonds.
	DataOORef<BondsObject> bondsObject;
	if(particles->bonds()) {
		bondsObject = DataOORef<BondsObject>::makeCopy(particles->bonds());
		bondsObject->verifyIntegrity();
	}
	else {
		bondsObject = DataOORef<BondsObject>::create(dataset(), ObjectCreationParams::WithoutVisElement);
		bondsObject->setDataSource(request.modApp());
		bondsObject->setVisElement(bondsVis());
	}

	// Pass a deep copy of the original bond type to the data pipeline.
	DataOORef<BondType> clonedBondType = DataOORef<BondType>::makeDeepCopy(bondType());

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<BondsEngine>(request, particles, posProperty,
			typeProperty, simCell, std::move(bondsObject), std::move(clonedBondType), particles, cutoffMode(),
			maxCutoff, minimumCutoff(), std::move(pairCutoffSquaredTable), 
			std::move(typeVdWRadiusMap), vdwPrefactor(), moleculeProperty, std::move(isHydrogenType));
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateBondsModifier::BondsEngine::perform()
{
	setProgressText(tr("Generating bonds"));
	
	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_maxCutoff, _positions, _simCell, {}, this))
		return;

	// The lower bond length cutoff squared.
	FloatType minCutoffSquared = _minCutoff * _minCutoff;

	ConstPropertyAccess<qlonglong> moleculeIDsArray(_moleculeIDs);
	ConstPropertyAccess<int> particleTypesArray(_particleTypes);

	// Generate bonds.
	size_t particleCount = _particles->elementCount();
	// Multi-threaded loop over all particles, each thread producing a partial bonds list.
	auto partialBondsLists = parallelForCollect<std::vector<Bond>>(particleCount, *this, [&](size_t particleIndex, std::vector<Bond>& bondList) {

		// Get the type of the central particles.
		int type1;
		bool isHydrogenType1 = false;
		if(particleTypesArray) {
			type1 = particleTypesArray[particleIndex];
			if(type1 < 0) return;
			if(type1 < _isHydrogenType.size())
				isHydrogenType1 = _isHydrogenType[type1];
		}

		// Kernel called for each particle: Iterate over the particle's neighbors withing the cutoff range.
		for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
			if(neighborQuery.distanceSquared() < minCutoffSquared)
				continue;
			if(moleculeIDsArray && moleculeIDsArray[particleIndex] != moleculeIDsArray[neighborQuery.current()])
				continue;

			if(particleTypesArray) {
				int type2 = particleTypesArray[neighborQuery.current()];
				if(type2 < 0) continue;
				if(type1 < (int)_typeVdWRadiusMap.size() && type2 < (int)_typeVdWRadiusMap.size()) {
					// Avoid generating H-H bonds.
					if(isHydrogenType1 && type2 < _isHydrogenType.size()) {
						if(_isHydrogenType[type2]) 
							continue;
					}
					FloatType cutoff = _vdwPrefactor * (_typeVdWRadiusMap[type1] + _typeVdWRadiusMap[type2]);
					if(neighborQuery.distanceSquared() > cutoff*cutoff) 
						continue;
				}
				else if(type1 < (int)_pairCutoffsSquared.size() && type2 < (int)_pairCutoffsSquared[type1].size()) {
					if(neighborQuery.distanceSquared() > _pairCutoffsSquared[type1][type2])
						continue;
				}
				else continue;
			}

			Bond bond = { particleIndex, neighborQuery.current(), neighborQuery.unwrappedPbcShift() };

			// Skip every other bond to create only one bond per particle pair.
			if(!bond.isOdd())
				bondList.push_back(bond);
		}
	});
	if(isCanceled())
		return;

	// Flatten the bonds list into a single std::vector.
	size_t totalBondCount = boost::accumulate(partialBondsLists, (size_t)0, [](size_t n, const std::vector<Bond>& bonds) { return n + bonds.size(); });
	std::vector<Bond>& bondsList = partialBondsLists.front();
	bondsList.reserve(totalBondCount);
	std::for_each(std::next(partialBondsLists.begin()), partialBondsLists.end(), [&](const std::vector<Bond>& bonds) { bondsList.insert(bondsList.end(), bonds.begin(), bonds.end()); });
	if(isCanceled())
		return;

	// Insert bonds into BondsObject.
	_numGeneratedBonds = bonds()->addBonds(bondsList, nullptr, _particles.get(), {}, std::move(_bondType));

	// Release data that is no longer needed.
	_positions.reset();
	_particleTypes.reset();
	_moleculeIDs.reset();
	_simCell.reset();
	_particles.reset();
	decltype(_typeVdWRadiusMap){}.swap(_typeVdWRadiusMap);
	decltype(_pairCutoffsSquared){}.swap(_pairCutoffsSquared);
	decltype(_isHydrogenType){}.swap(_isHydrogenType);
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CreateBondsModifier::BondsEngine::applyResults(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	CreateBondsModifier* modifier = static_object_cast<CreateBondsModifier>(request.modifier());
	OVITO_ASSERT(modifier);

	// Make the parent particle system mutable.
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	// Bonds have been created for a specific particle ordering. Make sure it's still the same.
	if(_inputFingerprint.hasChanged(particles))
		request.modApp()->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	// Add our bonds to the system.
	particles->setBonds(bonds());

	// Output the number of newly added bonds to the pipeline.
	state.addAttribute(QStringLiteral("CreateBonds.num_bonds"), QVariant::fromValue(_numGeneratedBonds), request.modApp());

	// If the total number of bonds is unusually high, we better turn off bonds display to prevent the program from freezing.
	size_t bondsCount = bonds()->elementCount();
	if(bondsCount > 1000000 && modifier->autoDisableBondDisplay() && modifier->bondsVis() && ExecutionContext::isInteractive()) {
		modifier->bondsVis()->setEnabled(false);
		state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Created %1 bonds, which is a lot. As a precaution, the display of bonds has been disabled. You can manually enable it again if needed.").arg(_numGeneratedBonds)));
	}
	else {
		state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Created %1 bonds.").arg(_numGeneratedBonds)));
	}
}

/******************************************************************************
* This function is called from AsynchronousModifier::evaluateSynchronous() to 
* apply the results from the last asycnhronous compute engine during a 
* synchronous pipeline evaluation.
******************************************************************************/
bool CreateBondsModifier::applyCachedResultsSynchronous(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	// If results are still available from the last pipeline evaluation, apply them to the input data.
	if(AsynchronousModifier::applyCachedResultsSynchronous(request, state))
		return true;

	// Bonds have not been computed yet, but still add the empty BondsObject to the pipeline output
	// so that subsequent modifiers in the pipeline see it.
	state.expectMutableObject<ParticlesObject>()->addBonds({}, bondsVis(), {}, bondType());
	OVITO_ASSERT(state.expectObject<ParticlesObject>()->bonds());

	return false;
}

}	// End of namespace
