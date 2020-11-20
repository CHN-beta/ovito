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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/crystalanalysis/modifier/dxa/StructureAnalysis.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>

namespace Ovito { namespace CrystalAnalysis {

/*
 * Identifies dislocation lines in a crystal and generates a line model of these defects.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationAnalysisModifier : public StructureIdentificationModifier
{
	Q_OBJECT
	OVITO_CLASS(DislocationAnalysisModifier)
	Q_CLASSINFO("DisplayName", "Dislocation analysis (DXA)");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE DislocationAnalysisModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;	
	
	/// Returns the crystal structure with the given ID, or null if no such structure exists.
	MicrostructurePhase* structureById(int id) const {
		for(ElementType* stype : structureTypes())
			if(stype->numericId() == id)
				return dynamic_object_cast<MicrostructurePhase>(stype);
		return nullptr;
	}

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, ExecutionContext executionContext) override;

private:

	/// The type of crystal to be analyzed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(StructureAnalysis::LatticeStructureType, inputCrystalStructure, setInputCrystalStructure, PROPERTY_FIELD_MEMORIZE);

	/// The maximum length of trial circuits.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, maxTrialCircuitSize, setMaxTrialCircuitSize);

	/// The maximum elongation of Burgers circuits while they are being advanced.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, circuitStretchability, setCircuitStretchability);

	/// Controls the output of the interface mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outputInterfaceMesh, setOutputInterfaceMesh);

	/// Restricts the identification to perfect lattice dislocations.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlyPerfectDislocations, setOnlyPerfectDislocations);

	/// The number of iterations of the mesh smoothing algorithm.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, defectMeshSmoothingLevel, setDefectMeshSmoothingLevel);

	/// Stores whether smoothing is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, lineSmoothingEnabled, setLineSmoothingEnabled);

	/// Controls the degree of smoothing applied to the dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, lineSmoothingLevel, setLineSmoothingLevel);

	/// Stores whether coarsening is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, lineCoarseningEnabled, setLineCoarseningEnabled);

	/// Controls the coarsening of dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, linePointInterval, setLinePointInterval);

	/// The visualization element for rendering the defect mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<SurfaceMeshVis>, defectMeshVis, setDefectMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);

	/// The visualization element for rendering the interface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<SurfaceMeshVis>, interfaceMeshVis, setInterfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);

	/// The visualization element for rendering the dislocations.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<DislocationVis>, dislocationVis, setDislocationVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
