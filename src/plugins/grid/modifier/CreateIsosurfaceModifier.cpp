///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <plugins/grid/Grid.h>
#include <plugins/grid/objects/VoxelGrid.h>
#include <plugins/mesh/surface/SurfaceMesh.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/app/Application.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include "CreateIsosurfaceModifier.h"
#include "MarchingCubes.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(CreateIsosurfaceModifier);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, subject);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, sourceProperty);
DEFINE_REFERENCE_FIELD(CreateIsosurfaceModifier, isolevelController);
DEFINE_REFERENCE_FIELD(CreateIsosurfaceModifier, surfaceMeshVis);
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, isolevelController, "Isolevel");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateIsosurfaceModifier::CreateIsosurfaceModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_subject(&VoxelGrid::OOClass())
{
	setIsolevelController(ControllerManager::createFloatController(dataset));

	// Create the vis element for rendering the surface generated by the modifier.
	setSurfaceMeshVis(new SurfaceMeshVis(dataset));
	surfaceMeshVis()->setShowCap(false);
	surfaceMeshVis()->setSmoothShading(true);
	surfaceMeshVis()->setObjectTitle(tr("Isosurface"));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval CreateIsosurfaceModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = AsynchronousModifier::modifierValidity(time);
	if(isolevelController()) interval.intersect(isolevelController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CreateIsosurfaceModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<VoxelGrid>();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateIsosurfaceModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Use the first available voxel grid from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull() && subject().dataPath().isEmpty() && Application::instance()->executionContext() == Application::ExecutionContext::Interactive) {
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const VoxelGrid* grid = input.getObject<VoxelGrid>()) {
			setSubject(PropertyContainerReference(&grid->getOOMetaClass(), grid->identifier()));
		}
	}

	// Use the first available property from the input grid as data source when the modifier is newly created.
	if(sourceProperty().isNull() && subject() && Application::instance()->executionContext() == Application::ExecutionContext::Interactive) {
		const PipelineFlowState& input = modApp->evaluateInputPreliminary();
		if(const VoxelGrid* grid = dynamic_object_cast<VoxelGrid>(input.getLeafObject(subject()))) {
			for(const PropertyObject* property : grid->properties()) {
				setSourceProperty(VoxelPropertyReference(property, (property->componentCount() > 1) ? 0 : -1));
				break;
			}
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CreateIsosurfaceModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(!subject())
		throwException(tr("No input voxel grid set."));
	if(subject().dataClass() != &VoxelGrid::OOClass())
		throwException(tr("Selected modifier input is not a voxel data grid."));
	if(sourceProperty().isNull())
		throwException(tr("Please select an input field quantity for the isosurface calculation."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().containerClass() != subject().dataClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(subject().dataClass()->pythonName()).arg(sourceProperty().containerClass()->propertyClassDisplayName()));

	// Get modifier inputs.
	const VoxelGrid* voxelGrid = static_object_cast<VoxelGrid>(input.expectLeafObject(subject()));
	OVITO_ASSERT(voxelGrid->domain());
	if(voxelGrid->domain()->is2D())
		throwException(tr("Cannot generate isosurface for a two-dimensional voxel grid. Input must be a 3d grid."));
	const PropertyObject* property = sourceProperty().findInContainer(voxelGrid);
	if(!property)
		throwException(tr("The selected voxel property with the name '%1' does not exist.").arg(sourceProperty().name()));
	if(sourceProperty().vectorComponent() >= (int)property->componentCount())
		throwException(tr("The selected vector component is out of range. The property '%1' contains only %2 values per voxel.").arg(sourceProperty().name()).arg(property->componentCount()));

	for(size_t dim = 0; dim < 3; dim++)
		if(voxelGrid->shape()[dim] <= 1)
			throwException(tr("Cannot generate isosurface for this voxel grid with dimensions %1 x %2 x %3. Must be at least 2 voxels wide in each spatial direction.")
				.arg(voxelGrid->shape()[0]).arg(voxelGrid->shape()[1]).arg(voxelGrid->shape()[2]));

	TimeInterval validityInterval = input.stateValidity();
	FloatType isolevel = isolevelController() ? isolevelController()->getFloatValue(time, validityInterval) : 0;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeIsosurfaceEngine>(validityInterval, voxelGrid->shape(), property->storage(),
			sourceProperty().vectorComponent(), voxelGrid->domain()->data(), isolevel);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateIsosurfaceModifier::ComputeIsosurfaceEngine::perform()
{
	task()->setProgressText(tr("Constructing isosurface"));

	if(_mesh.cell().is2D())
		throw Exception(tr("Cannot construct isosurfaces for two-dimensional voxel grids."));
	if(property()->dataType() != PropertyStorage::Float)
		throw Exception(tr("Wrong data type. Can construct isosurface only for floating-point values."));
	if(property()->size() != _gridShape[0] * _gridShape[1] * _gridShape[2])
		throw Exception(tr("Input voxel property has wrong array size, which is incompatible with the grid's dimensions."));

	const FloatType* fieldData = property()->constDataFloat() + std::max(_vectorComponent, 0);

	// Create the outer and the inner region.
	_mesh.createRegion();
	_mesh.createRegion();
	OVITO_ASSERT(_mesh.regionCount() == 2);

	MarchingCubes mc(_mesh, _gridShape[0], _gridShape[1], _gridShape[2], fieldData, property()->componentCount(), false);
	if(!mc.generateIsosurface(_isolevel, *task()))
		return;

	// Transform mesh vertices from orthogonal grid space to world space.
	const AffineTransformation tm = cell().matrix() * Matrix3(
		FloatType(1) / (_gridShape[0] - (cell().pbcFlags()[0]?0:1)), 0, 0,
		0, FloatType(1) / (_gridShape[1] - (cell().pbcFlags()[1]?0:1)), 0,
		0, 0, FloatType(1) / (_gridShape[2] - (cell().pbcFlags()[2]?0:1)));
	_mesh.transformVertices(tm);

	// Flip surface orientation if cell matrix is a mirror transformation.
	if(tm.determinant() < 0) {
		_mesh.flipFaces();
	}
	if(task()->isCanceled())
		return;

	if(!_mesh.connectOppositeHalfedges())
		throw Exception(tr("Something went wrong. Isosurface mesh is not closed."));
	if(task()->isCanceled())
		return;

	// Determine range of input field values.
	// Only used for informational purposes for the user.
	const FloatType* fieldDataEnd = fieldData + property()->size() * property()->componentCount();
	size_t componentCount = property()->componentCount();
	for(auto v = fieldData; v != fieldDataEnd; v += componentCount) {
		updateMinMax(*v);
	}

	// Compute a histogram of the input field values.
	auto histogramData = histogram()->dataInt64();
	FloatType binSize = (maxValue() - minValue()) / histogram()->size();
	int histogramSizeMin1 = histogram()->size() - 1;
	for(auto v = fieldData; v != fieldDataEnd; v += componentCount) {
		int binIndex = (*v - minValue()) / binSize;
		histogramData[std::max(0, std::min(binIndex, histogramSizeMin1))]++;
	}
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CreateIsosurfaceModifier::ComputeIsosurfaceEngine::emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(modApp->modifier());

	// Look up the input grid.
	if(const VoxelGrid* voxelGrid = dynamic_object_cast<VoxelGrid>(state.expectLeafObject(modifier->subject()))) {
		// Create the output mesh data object.
		SurfaceMesh* meshObj = state.createObject<SurfaceMesh>(QStringLiteral("isosurface"), modApp, tr("Isosurface"));
		mesh().transferTo(meshObj);
		meshObj->setDomain(voxelGrid->domain());
		meshObj->setVisElement(modifier->surfaceMeshVis());
	}

	// Output a data series object with the field value histogram.
	DataSeriesObject* seriesObj = state.createObject<DataSeriesObject>(QStringLiteral("isosurface-histogram"), modApp, DataSeriesObject::Histogram, modifier->sourceProperty().nameWithComponent(), histogram());
	seriesObj->setAxisLabelX(modifier->sourceProperty().nameWithComponent());
	seriesObj->setIntervalStart(minValue());
	seriesObj->setIntervalEnd(maxValue());

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Field value range: [%1, %2]").arg(minValue()).arg(maxValue())));
}

}	// End of namespace
}	// End of namespace
