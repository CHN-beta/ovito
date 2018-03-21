///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "GrainSegmentationModifier.h"
#include "GrainSegmentationEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {


DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, misorientationThreshold, "MisorientationThreshold", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, fluctuationTolerance, "FluctuationTolerance", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, minGrainAtomCount, "MinGrainAtomCount", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier, patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, smoothingLevel, "SmoothingLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, probeSphereRadius, "Radius", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier, meshDisplay, "MeshDisplay", PartitionMeshVis, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier, onlySelectedParticles, "OnlySelectedParticles");
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier, outputPartitionMesh, "OutputPartitionMesh");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, misorientationThreshold, "Misorientation threshold");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, fluctuationTolerance, "Tolerance");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, minGrainAtomCount, "Minimum grain size");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, meshDisplay, "Surface mesh display");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, outputPartitionMesh, "Generate mesh");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, misorientationThreshold, AngleParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, fluctuationTolerance, AngleParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, minGrainAtomCount, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, smoothingLevel, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
GrainSegmentationModifier::GrainSegmentationModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(StructureAnalysis::LATTICE_FCC),
		_misorientationThreshold(3.0 * FLOATTYPE_PI / 180.0),
		_fluctuationTolerance(2.0 * FLOATTYPE_PI / 180.0),
		_minGrainAtomCount(10),
		_smoothingLevel(8),
		_probeSphereRadius(4),
		_onlySelectedParticles(false),
		_outputPartitionMesh(false)
{











	// Create the display object.
	_meshDisplay = new PartitionMeshVis(dataset);

	// Create pattern catalog.
	_patternCatalog = new PatternCatalog(dataset);

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
		OORef<StructurePattern> stype = _patternCatalog->structureById(id);
		if(!stype) {
			stype = new StructurePattern(dataset);
			stype->setId(id);
			stype->setStructureType(StructurePattern::Lattice);
			_patternCatalog->addPattern(stype);
		}
		stype->setName(ParticleType::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleType::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		addStructureType(stype);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void GrainSegmentationModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(inputCrystalStructure) ||
			field == PROPERTY_FIELD(misorientationThreshold) ||
			field == PROPERTY_FIELD(fluctuationTolerance) ||
			field == PROPERTY_FIELD(minGrainAtomCount) ||
			field == PROPERTY_FIELD(smoothingLevel) ||
			field == PROPERTY_FIELD(probeSphereRadius) ||
			field == PROPERTY_FIELD(onlySelectedParticles) ||
			field == PROPERTY_FIELD(outputPartitionMesh))
		invalidateCachedResults();
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void GrainSegmentationModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
	_partitionMesh.reset();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> GrainSegmentationModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticleProperty* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Get particle selection.
	PropertyStorage* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<GrainSegmentationEngine>(posProperty->storage(),
			simCell->data(), selectionProperty, inputCrystalStructure(), misorientationThreshold(),
			fluctuationTolerance(), minGrainAtomCount(), outputPartitionMesh() ? probeSphereRadius() : 0, smoothingLevel());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void GrainSegmentationModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	GrainSegmentationEngine* eng = static_cast<GrainSegmentationEngine*>(engine);
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->outputClusterGraph();
	_partitionMesh = eng->mesh();
	_spaceFillingRegion = eng->spaceFillingGrain();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus GrainSegmentationModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_atomClusters)
		throwException(tr("No computation results available."));

	if(outputParticleCount() != _atomClusters->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	if(_clusterGraph) {
		// Output cluster graph.
		OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(dataset(), _clusterGraph.data()));
		output().addObject(clusterGraphObj);
	}

	// Output pattern catalog.
	if(_patternCatalog) {
		output().addObject(_patternCatalog);
	}

	// Output particle properties.
	outputStandardProperty(_atomClusters.data());

	if(_partitionMesh) {
		// Create the output data object for the partition mesh.
		OORef<PartitionMesh> meshObj(new PartitionMesh(dataset(), _partitionMesh.data()));
		meshObj->setSpaceFillingRegion(_spaceFillingRegion);
		meshObj->addDisplayObject(_meshDisplay);

		// Insert output object into the pipeline.
		output().addObject(meshObj);
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

