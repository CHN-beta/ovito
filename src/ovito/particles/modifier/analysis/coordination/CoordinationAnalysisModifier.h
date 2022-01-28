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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito::Particles {

/**
 * \brief This modifier computes the coordination number of each particle (i.e. the number of neighbors within a given cutoff radius).
 */
class OVITO_PARTICLES_EXPORT CoordinationAnalysisModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CoordinationAnalysisModifierClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	OVITO_CLASS_META(CoordinationAnalysisModifier, CoordinationAnalysisModifierClass)

	Q_CLASSINFO("ClassNameAlias", "CoordinationNumberModifier");
	Q_CLASSINFO("Description", "Determine number of neighbors and compute the radial distribution function (RDF).");
	Q_CLASSINFO("DisplayName", "Coordination analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE CoordinationAnalysisModifier(ObjectCreationParams params);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const ModifierEvaluationRequest& request, const PipelineFlowState& input) override;

private:

	/// Computes the modifier's results.
	class CoordinationAnalysisEngine : public Engine
	{
	public:

		/// Constructor.
		CoordinationAnalysisEngine(const ModifierEvaluationRequest& request, ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCellObject* simCell,
				FloatType cutoff, int rdfSampleCount, ConstPropertyPtr particleTypes, boost::container::flat_map<int,QString> uniqueTypeIds) :
			Engine(request),
			_positions(std::move(positions)),
			_selection(std::move(selection)),
			_simCell(simCell),
			_cutoff(cutoff),
			_computePartialRdfs(particleTypes),
			_particleTypes(std::move(particleTypes)),
			_uniqueTypeIds(std::move(uniqueTypeIds)),
			_coordinationNumbers(ParticlesObject::OOClass().createStandardProperty(request.dataset(), fingerprint.particleCount(), ParticlesObject::CoordinationProperty, DataBuffer::InitializeMemory)),
			_inputFingerprint(std::move(fingerprint))
		{
			size_t componentCount = _computePartialRdfs ? (this->uniqueTypeIds().size() * (this->uniqueTypeIds().size()+1) / 2) : 1;
			QStringList componentNames;
			if(_computePartialRdfs) {
				for(const auto& t1 : this->uniqueTypeIds()) {
					for(const auto& t2 : this->uniqueTypeIds()) {
						if(t1.first <= t2.first)
							componentNames.push_back(QStringLiteral("%1-%2").arg(t1.second, t2.second));
					}
				}
			}
			_rdfY = DataTable::OOClass().createUserProperty(request.dataset(), rdfSampleCount, PropertyObject::Float, componentCount, tr("g(r)"), DataBuffer::InitializeMemory, 0, std::move(componentNames));
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(const ModifierEvaluationRequest& request, PipelineFlowState& state) override;

		/// Returns the property storage that contains the computed coordination numbers.
		const PropertyPtr& coordinationNumbers() const { return _coordinationNumbers; }

		/// Returns the property storage array containing the y-coordinates of the data points of the RDF histograms.
		const PropertyPtr& rdfY() const { return _rdfY; }

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the input particle types.
		const ConstPropertyPtr& particleTypes() const { return _particleTypes; }

		/// Returns the property storage that contains the input particle selection states.
		const ConstPropertyPtr& selection() const { return _selection; }

		/// Returns the simulation cell data.
		const DataOORef<const SimulationCellObject>& cell() const { return _simCell; }

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

		/// Returns the set of particle type identifiers in the system.
		const boost::container::flat_map<int,QString>& uniqueTypeIds() const { return _uniqueTypeIds; }

	private:

		const FloatType _cutoff;
		DataOORef<const SimulationCellObject> _simCell;
		bool _computePartialRdfs;
		boost::container::flat_map<int,QString> _uniqueTypeIds;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _particleTypes;
		ConstPropertyPtr _selection;
		const PropertyPtr _coordinationNumbers;
		PropertyPtr _rdfY;
		ParticleOrderingFingerprint _inputFingerprint;
	};

private:

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls the number of RDF histogram bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfBins, setNumberOfBins, PROPERTY_FIELD_MEMORIZE);

	/// Controls the computation of partials RDFs.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, computePartialRDF, setComputePartialRDF, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the modifier acts only on currently selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);
};

}	// End of namespace
