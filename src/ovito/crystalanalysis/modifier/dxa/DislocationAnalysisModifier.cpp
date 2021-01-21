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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/mesh/tri/TriMeshVis.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "DislocationAnalysisModifier.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationAnalysisModifier);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, inputCrystalStructure);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, maxTrialCircuitSize);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, circuitStretchability);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, outputInterfaceMesh);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, onlyPerfectDislocations);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, defectMeshSmoothingLevel);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, lineSmoothingEnabled);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, lineSmoothingLevel);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, lineCoarseningEnabled);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, linePointInterval);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, dislocationVis);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, defectMeshVis);
DEFINE_REFERENCE_FIELD(DislocationAnalysisModifier, interfaceMeshVis);
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, maxTrialCircuitSize, "Trial circuit length");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, circuitStretchability, "Circuit stretchability");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, outputInterfaceMesh, "Output interface mesh");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, onlyPerfectDislocations, "Generate perfect dislocations");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, defectMeshSmoothingLevel, "Surface smoothing level");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, lineSmoothingEnabled, "Line smoothing");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, lineSmoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, lineCoarseningEnabled, "Line coarsening");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, linePointInterval, "Point separation");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, maxTrialCircuitSize, IntegerParameterUnit, 3);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, circuitStretchability, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, defectMeshSmoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, lineSmoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, linePointInterval, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DislocationAnalysisModifier::DislocationAnalysisModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_inputCrystalStructure(StructureAnalysis::LATTICE_FCC),
	_maxTrialCircuitSize(14),
	_circuitStretchability(9),
	_outputInterfaceMesh(false),
	_onlyPerfectDislocations(false),
	_defectMeshSmoothingLevel(8),
	_lineSmoothingEnabled(true),
	_lineCoarseningEnabled(true),
	_lineSmoothingLevel(1),
	_linePointInterval(2.5)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void DislocationAnalysisModifier::initializeObject(ExecutionContext executionContext)
{
	// Create the vis elements.
	setDislocationVis(OORef<DislocationVis>::create(dataset(), executionContext));

	setDefectMeshVis(OORef<SurfaceMeshVis>::create(dataset(), executionContext));
	defectMeshVis()->setShowCap(true);
	defectMeshVis()->setSmoothShading(true);
	defectMeshVis()->setReverseOrientation(true);
	defectMeshVis()->setCapTransparency(0.5);
	defectMeshVis()->setObjectTitle(tr("Defect mesh"));

	setInterfaceMeshVis(OORef<SurfaceMeshVis>::create(dataset(), executionContext));
	interfaceMeshVis()->setShowCap(false);
	interfaceMeshVis()->setSmoothShading(false);
	interfaceMeshVis()->setReverseOrientation(true);
	interfaceMeshVis()->setCapTransparency(0.5);
	interfaceMeshVis()->setObjectTitle(tr("Interface mesh"));

	// Create the structure types.
	ParticleType::PredefinedStructureType predefTypes[] = {
			ParticleType::PredefinedStructureType::OTHER,
			ParticleType::PredefinedStructureType::FCC,
			ParticleType::PredefinedStructureType::HCP,
			ParticleType::PredefinedStructureType::BCC,
			ParticleType::PredefinedStructureType::CUBIC_DIAMOND,
			ParticleType::PredefinedStructureType::HEX_DIAMOND
	};
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == StructureAnalysis::NUM_LATTICE_TYPES);
	for(int id = 0; id < StructureAnalysis::NUM_LATTICE_TYPES; id++) {
		DataOORef<MicrostructurePhase> stype = DataOORef<MicrostructurePhase>::create(dataset(), executionContext);
		stype->setNumericId(id);
		stype->setDimensionality(MicrostructurePhase::Dimensionality::Volumetric);
		stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ElementType::getDefaultColor(ParticlePropertyReference(ParticlesObject::StructureTypeProperty), stype->name(), id, executionContext));
		addStructureType(std::move(stype));
	}

	// Create Burgers vector families.

	MicrostructurePhase* fccPattern = structureById(StructureAnalysis::LATTICE_FCC);
	fccPattern->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
	fccPattern->setShortName(QStringLiteral("fcc"));
	fccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext));
	fccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 1, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
	fccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 2, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	fccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 3, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
	fccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 4, tr("1/3<100> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
	fccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 5, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

	MicrostructurePhase* bccPattern = structureById(StructureAnalysis::LATTICE_BCC);
	bccPattern->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
	bccPattern->setShortName(QStringLiteral("bcc"));
	bccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext));
	bccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 11, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
	bccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 12, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
	bccPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 13, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));

	MicrostructurePhase* hcpPattern = structureById(StructureAnalysis::LATTICE_HCP);
	hcpPattern->setShortName(QStringLiteral("hcp"));
	hcpPattern->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::HexagonalSymmetry);
	hcpPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext));
	hcpPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 21, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
	hcpPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 22, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
	hcpPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 23, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
	hcpPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 24, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
	hcpPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 25, tr("1/3<1-213>"), Vector3(sqrt(0.5f), 0.0f, sqrt(4.0f/3.0f)), Color(1,1,0)));

	MicrostructurePhase* cubicDiaPattern = structureById(StructureAnalysis::LATTICE_CUBIC_DIAMOND);
	cubicDiaPattern->setShortName(QStringLiteral("diamond"));
	cubicDiaPattern->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry);
	cubicDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext));
	cubicDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 31, tr("1/2<110>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
	cubicDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 32, tr("1/6<112>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	cubicDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 33, tr("1/6<110>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f), Color(1,0,1)));
	cubicDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 34, tr("1/3<111>"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

	MicrostructurePhase* hexDiaPattern = structureById(StructureAnalysis::LATTICE_HEX_DIAMOND);
	hexDiaPattern->setShortName(QStringLiteral("hex_diamond"));
	hexDiaPattern->setCrystalSymmetryClass(MicrostructurePhase::CrystalSymmetryClass::HexagonalSymmetry);
	hexDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext));
	hexDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 41, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
	hexDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 42, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
	hexDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 43, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
	hexDiaPattern->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext, 44, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));

	StructureIdentificationModifier::initializeObject(executionContext);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> DislocationAnalysisModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, ExecutionContext executionContext)
{
	// Get modifier inputs.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();
	if(simCell->is2D())
		throwException(tr("The DXA modifier does not support 2d simulation cells."));

	// Get particle selection.
	const PropertyObject* selectionProperty = onlySelectedParticles() ? particles->expectProperty(ParticlesObject::SelectionProperty) : nullptr;

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Get cluster property.
#if 0
	const PropertyObject* clusterProperty = particles->getProperty(ParticlesObject::ClusterProperty);
#else
	const PropertyObject* clusterProperty = nullptr;
#endif

	// Create an empty surface mesh object.
	DataOORef<SurfaceMesh> defectMesh = DataOORef<SurfaceMesh>::create(dataset(), executionContext, tr("Defect mesh"));
	defectMesh->setIdentifier(input.generateUniqueIdentifier<SurfaceMesh>(QStringLiteral("dxa-defect-mesh")));
	defectMesh->setDataSource(modApp);
	defectMesh->setDomain(simCell);
	defectMesh->setVisElement(defectMeshVis());

	// Create an empty surface mesh object for the optional interface mesh.
	DataOORef<SurfaceMesh> interfaceMesh;
	if(outputInterfaceMesh()) {
		interfaceMesh = DataOORef<SurfaceMesh>::create(dataset(), executionContext, tr("Interface mesh"));
		interfaceMesh->setIdentifier(input.generateUniqueIdentifier<SurfaceMesh>(QStringLiteral("dxa-interface-mesh")));
		interfaceMesh->setDataSource(modApp);
		interfaceMesh->setDomain(simCell);
		interfaceMesh->setVisElement(interfaceMeshVis());
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DislocationAnalysisEngine>(
			modApp,
			executionContext, 
			dataset(), 
			particles,
			posProperty,
			simCell,
			structureTypes(),
			inputCrystalStructure(),
			maxTrialCircuitSize(),
			circuitStretchability(),
			selectionProperty,
			clusterProperty,
			std::move(preferredCrystalOrientations),
			onlyPerfectDislocations(),
			defectMeshSmoothingLevel(),
			std::move(defectMesh),
			std::move(interfaceMesh),
			lineSmoothingEnabled() ? lineSmoothingLevel() : 0,
			lineCoarseningEnabled() ? linePointInterval() : 0);
}

}	// End of namespace
}	// End of namespace
