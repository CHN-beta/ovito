////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

/// This comparison operator is required for using QVariant as key-type in a QMap as done by CreateBondsModifier.
/// The < operator for QVariant, which is part of the key-type, has been removed in Qt 6. Redefining it here is an ugly hack and should be 
/// solved in a different way in the future.
template<> inline bool qMapLessThanKey<QPair<QVariant, QVariant>>(const QPair<QVariant, QVariant>& key1, const QPair<QVariant, QVariant>& key2)
{
	return key1.first.toString() < key2.first.toString() || (!(key2.first.toString() < key1.first.toString()) && key1.second.toString() < key2.second.toString());
}

namespace Ovito { namespace Particles {

/**
 * \brief A modifier that creates bonds between pairs of particles based on their distance.
 */
class OVITO_PARTICLES_EXPORT CreateBondsModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CreateBondsModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(CreateBondsModifier, CreateBondsModifierClass)

	Q_CLASSINFO("DisplayName", "Create bonds");
	Q_CLASSINFO("Description", "Creates bonds between particles.");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	enum CutoffMode {
		UniformCutoff,		///< A uniform distance cutoff for all pairs of particles.
		PairCutoff,			///< Individual cutoff for each pair-wise combination of particle types.
		TypeRadiusCutoff,	///< Cutoff based on Van der Waals radii of the two particle types involved.
	};
	Q_ENUM(CutoffMode);

	/// The container type used to store the pair-wise cutoffs.
	using PairwiseCutoffsList = QMap<QPair<QVariant,QVariant>, FloatType>;

private:

	/// Compute engine that creates bonds between particles.
	class BondsEngine : public Engine
	{
	public:

		/// Constructor.
		BondsEngine(const PipelineObject* dataSource, ExecutionContext executionContext, ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, ConstPropertyPtr particleTypes,
				const SimulationCellObject* simCell, CutoffMode cutoffMode, FloatType maxCutoff, FloatType minCutoff, std::vector<std::vector<FloatType>> pairCutoffsSquared,
				std::vector<FloatType> typeVdWRadiusMap, FloatType vdwPrefactor, ConstPropertyPtr moleculeIDs) :
					Engine(dataSource, executionContext),
					_positions(std::move(positions)),
					_particleTypes(std::move(particleTypes)),
					_simCell(simCell),
					_cutoffMode(cutoffMode),
					_maxCutoff(maxCutoff),
					_minCutoff(minCutoff),
					_pairCutoffsSquared(std::move(pairCutoffsSquared)),
					_typeVdWRadiusMap(std::move(typeVdWRadiusMap)),
					_vdwPrefactor(vdwPrefactor),
					_moleculeIDs(std::move(moleculeIDs)),
					_inputFingerprint(std::move(fingerprint)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// This method is called by the system whenever the preliminary pipeline input changes.
		virtual bool pipelineInputChanged() override { return false; }

		/// Returns the list of generated bonds.
		std::vector<Bond>& bonds() { return _bonds; }

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

	private:

		const CutoffMode _cutoffMode;
		const FloatType _maxCutoff;
		const FloatType _minCutoff;
		const std::vector<std::vector<FloatType>> _pairCutoffsSquared;
		const std::vector<FloatType> _typeVdWRadiusMap;		
		const FloatType _vdwPrefactor;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _particleTypes;
		ConstPropertyPtr _moleculeIDs;
		DataOORef<const SimulationCellObject> _simCell;
		ParticleOrderingFingerprint _inputFingerprint;
		std::vector<Bond> _bonds;
	};

public:

	/// Constructor.
	Q_INVOKABLE CreateBondsModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;	

	/// \brief This method is called by the system when the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext) override;

	/// Sets the cutoff radius for a pair of particle types.
	void setPairwiseCutoff(const QVariant& typeA, const QVariant& typeB, FloatType cutoff);

	/// Returns the pair-wise cutoff radius for a pair of particle types.
	FloatType getPairwiseCutoff(const QVariant& typeA, const QVariant& typeB) const;

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, ExecutionContext executionContext) override;

	/// This function is called from AsynchronousModifier::evaluateSynchronous() to apply the results from the last 
	/// asycnhronous compute engine during a synchronous pipeline evaluation.
	virtual bool applyCachedResultsSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Looks up a particle type in the type list based on the name or the numeric ID.
	static const ElementType* lookupParticleType(const PropertyObject* typeProperty, const QVariant& typeSpecification);

private:

	/// The mode of determing the bond cutoff.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(CutoffMode, cutoffMode, setCutoffMode, PROPERTY_FIELD_MEMORIZE);

	/// The uniform cutoff distance for bond generation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, uniformCutoff, setUniformCutoff, PROPERTY_FIELD_MEMORIZE);

	/// The minimum bond length.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, minimumCutoff, setMinimumCutoff);

	/// The prefactor to be used for computing the cutoff distance from the Van der Waals radii.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, vdwPrefactor, setVdwPrefactor);

	/// The cutoff radii for pairs of particle types.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PairwiseCutoffsList, pairwiseCutoffs, setPairwiseCutoffs);

	/// If true, bonds will only be created between atoms from the same molecule.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, onlyIntraMoleculeBonds, setOnlyIntraMoleculeBonds, PROPERTY_FIELD_MEMORIZE);

	/// The bond type object that will be assigned to the newly created bonds.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<BondType>, bondType, setBondType, PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// The vis element for rendering the bonds.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<BondsVis>, bondsVis, setBondsVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// Controls whether the modifier should automatically turn off the display in case the number of bonds is unusually large.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, autoDisableBondDisplay, setAutoDisableBondDisplay, PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_NO_UNDO);
};

}	// End of namespace
}	// End of namespace
