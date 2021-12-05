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
#include <ovito/particles/objects/VectorVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/modifier/analysis/ReferenceConfigurationModifier.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>

namespace Ovito::Particles {

/**
 * \brief Calculates the per-particle displacement vectors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT CalculateDisplacementsModifier : public ReferenceConfigurationModifier
{
	Q_OBJECT
	OVITO_CLASS(CalculateDisplacementsModifier)

	Q_CLASSINFO("DisplayName", "Displacement vectors");
	Q_CLASSINFO("Description", "Calculate the displacements of particles based on two input configurations.");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE CalculateDisplacementsModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ObjectInitializationHints hints) override;	
	
protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngineInternal(const ModifierEvaluationRequest& request, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) override;

private:

	/// Computes the modifier's results.
	class DisplacementEngine : public RefConfigEngineBase
	{
	public:

		/// Constructor.
		DisplacementEngine(
				const ModifierEvaluationRequest& request,
				const TimeInterval& validityInterval, 
				ConstPropertyPtr positions, 
				const SimulationCellObject* simCell,
				ParticleOrderingFingerprint fingerprint,
				ConstPropertyPtr refPositions, 
				const SimulationCellObject* simCellRef,
				ConstPropertyPtr identifiers, 
				ConstPropertyPtr refIdentifiers,
				AffineMappingType affineMapping, 
				bool useMinimumImageConvention) :
			RefConfigEngineBase(request, validityInterval, positions, simCell, std::move(refPositions), simCellRef,
				std::move(identifiers), std::move(refIdentifiers), affineMapping, useMinimumImageConvention),
			_displacements(ParticlesObject::OOClass().createStandardProperty(request.dataset(), fingerprint.particleCount(), ParticlesObject::DisplacementProperty, false, request.initializationHints())),
			_displacementMagnitudes(ParticlesObject::OOClass().createStandardProperty(request.dataset(), fingerprint.particleCount(), ParticlesObject::DisplacementMagnitudeProperty, false, request.initializationHints())),
			_inputFingerprint(std::move(fingerprint)) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(const ModifierEvaluationRequest& request, PipelineFlowState& state) override;

		/// Returns the property storage that contains the computed displacement vectors.
		const PropertyPtr& displacements() const { return _displacements; }

		/// Returns the property storage that contains the computed displacement vector magnitudes.
		const PropertyPtr& displacementMagnitudes() const { return _displacementMagnitudes; }

	private:

		const PropertyPtr _displacements;
		const PropertyPtr _displacementMagnitudes;
		ParticleOrderingFingerprint _inputFingerprint;
	};

	/// The vis element for rendering the displacement vectors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<VectorVis>, vectorVis, setVectorVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
