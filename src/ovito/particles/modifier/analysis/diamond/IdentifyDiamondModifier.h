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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>

namespace Ovito { namespace Particles {

/**
 * \brief A modifier that identifies local diamond structures.
 */
class OVITO_PARTICLES_EXPORT IdentifyDiamondModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(IdentifyDiamondModifier)

	Q_CLASSINFO("DisplayName", "Identify diamond structure");
	Q_CLASSINFO("Description", "Identify particles arranged in cubic and hexagonal diamond structures.");
#ifndef OVITO_BUILD_WEBGUI
	Q_CLASSINFO("ModifierCategory", "Structure identification");
#else
	Q_CLASSINFO("ModifierCategory", "-");
#endif

public:

	/// The structure types recognized by the modifier.
	enum StructureType {
		OTHER = 0,					//< Unidentified structure
		CUBIC_DIAMOND,				//< Cubic diamond structure
		CUBIC_DIAMOND_FIRST_NEIGH,	//< First neighbor of a cubic diamond atom
		CUBIC_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a cubic diamond atom
		HEX_DIAMOND,				//< Hexagonal diamond structure
		HEX_DIAMOND_FIRST_NEIGH,	//< First neighbor of a hexagonal diamond atom
		HEX_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a hexagonal diamond atom

		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);

public:

	/// Constructor.
	Q_INVOKABLE IdentifyDiamondModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(Application::ExecutionContext executionContext) override;	
	
protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, Application::ExecutionContext executionContext) override;

private:

	/// Analysis engine that performs the structure identification
	class DiamondIdentificationEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		using StructureIdentificationEngine::StructureIdentificationEngine;

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;
	};
};

}	// End of namespace
}	// End of namespace
