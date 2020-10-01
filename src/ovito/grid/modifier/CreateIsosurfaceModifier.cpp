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

#include <ovito/grid/Grid.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "CreateIsosurfaceModifier.h"
#include "MarchingCubes.h"

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(CreateIsosurfaceModifier);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, subject);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, transferFieldValues);
DEFINE_REFERENCE_FIELD(CreateIsosurfaceModifier, isolevelController);
DEFINE_REFERENCE_FIELD(CreateIsosurfaceModifier, surfaceMeshVis);
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, isolevelController, "Isolevel");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, transferFieldValues, "Transfer field values to surface");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateIsosurfaceModifier::CreateIsosurfaceModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_transferFieldValues(false)
{
	setIsolevelController(ControllerManager::createFloatController(dataset));

	// Create the vis element for rendering the surface generated by the modifier.
	setSurfaceMeshVis(new SurfaceMeshVis(dataset));
	surfaceMeshVis()->setShowCap(false);
	surfaceMeshVis()->setSmoothShading(true);
	surfaceMeshVis()->setObjectTitle(tr("Isosurface"));
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval CreateIsosurfaceModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = AsynchronousModifier::validityInterval(request, modApp);
	if(isolevelController()) 
		iv.intersect(isolevelController()->validityInterval(request.time()));
	return iv;
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
		const PipelineFlowState& input = modApp->evaluateInputSynchronous(dataset()->animationSettings()->time());
		if(const VoxelGrid* grid = input.getObject<VoxelGrid>()) {
			setSubject(PropertyContainerReference(&grid->getOOMetaClass(), grid->identifier()));
		}
	}

	// Use the first available property from the input grid as data source when the modifier is newly created.
	if(sourceProperty().isNull() && subject() && Application::instance()->executionContext() == Application::ExecutionContext::Interactive) {
		const PipelineFlowState& input = modApp->evaluateInputSynchronous(dataset()->animationSettings()->time());
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
Future<AsynchronousModifier::EnginePtr> CreateIsosurfaceModifier::createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input)
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
	voxelGrid->verifyIntegrity();
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
	FloatType isolevel = isolevelController() ? isolevelController()->getFloatValue(request.time(), validityInterval) : 0;

	// Collect the set of voxel grid properties that should be transferred over to the isosurface mesh vertices.
	std::vector<ConstPropertyPtr> auxiliaryProperties;
	if(transferFieldValues()) {
		for(const PropertyObject* property : voxelGrid->properties()) {
			auxiliaryProperties.push_back(property->storage());
		}
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeIsosurfaceEngine>(validityInterval, voxelGrid->shape(), property->storage(),
			sourceProperty().vectorComponent(), voxelGrid->domain()->data(), isolevel, std::move(auxiliaryProperties));
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateIsosurfaceModifier::ComputeIsosurfaceEngine::perform()
{
	setProgressText(tr("Constructing isosurface"));

	if(_mesh.cell().is2D())
		throw Exception(tr("Cannot construct isosurfaces for two-dimensional voxel grids."));
	if(property()->dataType() != PropertyStorage::Float)
		throw Exception(tr("Wrong data type. Can construct isosurface only for floating-point values."));
	if(property()->size() != _gridShape[0] * _gridShape[1] * _gridShape[2])
		throw Exception(tr("Input voxel property has wrong array size, which is incompatible with the grid's dimensions."));

	// Set up callback function returning the field value, which will be passed to the marching cubes algorithm.
	ConstPropertyAccess<FloatType, true> data(property());
    auto getFieldValue = [
			_data = data.cbegin() + _vectorComponent,
			_pbcFlags = cell().pbcFlags(),
			_gridShape = _gridShape,
			_dataStride = data.componentCount()
			](int i, int j, int k) -> FloatType {
        if(_pbcFlags[0]) {
            if(i == _gridShape[0]) i = 0;
        }
        else {
            if(i == 0 || i == _gridShape[0] + 1) return std::numeric_limits<FloatType>::lowest();
            i--;
        }
        if(_pbcFlags[1]) {
            if(j == _gridShape[1]) j = 0;
        }
        else {
            if(j == 0 || j == _gridShape[1] + 1) return std::numeric_limits<FloatType>::lowest();
            j--;
        }
        if(_pbcFlags[2]) {
            if(k == _gridShape[2]) k = 0;
        }
        else {
            if(k == 0 || k == _gridShape[2] + 1) return std::numeric_limits<FloatType>::lowest();
            k--;
        }
        OVITO_ASSERT(i >= 0 && i < _gridShape[0]);
        OVITO_ASSERT(j >= 0 && j < _gridShape[1]);
        OVITO_ASSERT(k >= 0 && k < _gridShape[2]);
        return _data[(i + j*_gridShape[0] + k*_gridShape[0]*_gridShape[1]) * _dataStride];
    };

	MarchingCubes mc(mesh(), _gridShape[0], _gridShape[1], _gridShape[2], false, std::move(getFieldValue));
	if(!mc.generateIsosurface(_isolevel, *this))
		return;

	// Copy field values from voxel grid to surface mesh vertices.
	if(!transferPropertiesFromGridToMesh(*this, mesh(), auxiliaryProperties(), cell(), _gridShape))
		return;

	// Transform mesh vertices from orthogonal grid space to world space.
	const AffineTransformation tm = cell().matrix() * Matrix3(
		FloatType(1) / _gridShape[0], 0, 0,
		0, FloatType(1) / _gridShape[1], 0,
		0, 0, FloatType(1) / _gridShape[2]) *
		AffineTransformation::translation(Vector3(0.5, 0.5, 0.5));
	mesh().transformVertices(tm);

	// Flip surface orientation if cell matrix is a mirror transformation.
	if(tm.determinant() < 0)
		mesh().flipFaces();
	if(isCanceled())
		return;

	if(!mesh().connectOppositeHalfedges())
		throw Exception(tr("Something went wrong. Isosurface mesh is not closed."));
	if(isCanceled())
		return;

	// Determine min-max range of input field values.
	// Only used for informational purposes for the user.
	for(FloatType v : data.componentRange(_vectorComponent)) {
		updateMinMax(v);
	}

	// Compute a histogram of the input field values.
	PropertyAccess<qlonglong> histogramData(histogram());
	FloatType binSize = (maxValue() - minValue()) / histogramData.size();
	int histogramSizeMin1 = histogramData.size() - 1;
	for(FloatType v : data.componentRange(_vectorComponent)) {
		int binIndex = (v - minValue()) / binSize;
		histogramData[std::max(0, std::min(binIndex, histogramSizeMin1))]++;
	}

	// Release data that is no longer needed to reduce memory footprint.
	_property.reset();
	_auxiliaryProperties.clear();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void CreateIsosurfaceModifier::ComputeIsosurfaceEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
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

	// Output a data table with the field value histogram.
	DataTable* table = state.createObject<DataTable>(QStringLiteral("isosurface-histogram"), modApp, DataTable::Histogram, modifier->sourceProperty().nameWithComponent(), histogram());
	table->setAxisLabelX(modifier->sourceProperty().nameWithComponent());
	table->setIntervalStart(minValue());
	table->setIntervalEnd(maxValue());

	state.setStatus(PipelineStatus(PipelineStatus::Success, tr("Field value range: [%1, %2]").arg(minValue()).arg(maxValue())));
}

/******************************************************************************
* Transfers voxel grid properties to the vertices of a surfaces mesh.
******************************************************************************/
bool CreateIsosurfaceModifier::transferPropertiesFromGridToMesh(Task& task, SurfaceMeshData& mesh, const std::vector<ConstPropertyPtr>& fieldProperties, const SimulationCell& cell, VoxelGrid::GridDimensions gridShape)
{
	// Create destination properties for transferring voxel values to the surface vertices.
	std::vector<std::pair<ConstPropertyAccess<void,true>, PropertyAccess<void,true>>> propertyMapping;
	for(const ConstPropertyPtr& fieldProperty : fieldProperties) {
		PropertyPtr vertexProperty;
		if(SurfaceMeshVertices::OOClass().isValidStandardPropertyId(fieldProperty->type())) {
			// Input voxel property is also a standard property for mesh vertices.
			vertexProperty = mesh.createVertexProperty(static_cast<SurfaceMeshVertices::Type>(fieldProperty->type()), true);
			OVITO_ASSERT(vertexProperty->dataType() == fieldProperty->dataType());
			OVITO_ASSERT(vertexProperty->stride() == fieldProperty->stride());
		}
		else if(SurfaceMeshVertices::OOClass().standardPropertyTypeId(fieldProperty->name()) != 0) {
			// Input property name is that of a standard property for mesh vertices.
			// Must rename the property to avoid conflict, because user properties may not have a standard property name.
			QString newPropertyName = fieldProperty->name() + tr("_field");
			vertexProperty = mesh.createVertexProperty(fieldProperty->dataType(), fieldProperty->componentCount(), fieldProperty->stride(), newPropertyName, true, fieldProperty->componentNames());
		}
		else {
			// Input property is a user property for mesh vertices.
			vertexProperty = mesh.createVertexProperty(fieldProperty->dataType(), fieldProperty->componentCount(), fieldProperty->stride(), fieldProperty->name(), true, fieldProperty->componentNames());
		}
		propertyMapping.emplace_back(fieldProperty, std::move(vertexProperty));
	}

	// Transfer values of field properties to the created mesh vertices.
	if(!propertyMapping.empty()) {
		parallelFor(mesh.vertexCount(), task, [&](size_t vertexIndex) {
			// Trilinear interpolation scheme.
			size_t cornerIndices[8];
			FloatType cornerWeights[8];
			const Point3& p = mesh.vertexPosition(vertexIndex);
			OVITO_ASSERT(mesh.firstVertexEdge(vertexIndex) != HalfEdgeMesh::InvalidIndex);
			Vector3 x0, x1;
			Vector3I x0_vc, x1_vc;
			for(size_t dim = 0; dim < 3; dim++) {
				OVITO_ASSERT(p[dim] >= -0.5-FLOATTYPE_EPSILON);
				OVITO_ASSERT(p[dim] <= (FloatType)gridShape[dim] + FLOATTYPE_EPSILON + 0.5);
				FloatType fl = std::floor(p[dim]);
				x1[dim] = p[dim] - fl;
				x0[dim] = FloatType(1) - x1[dim];
				if(!cell.hasPbc(dim)) {
					x0_vc[dim] = qBound(0, (int)fl, (int)gridShape[dim] - 1);
					x1_vc[dim] = qBound(0, (int)fl + 1, (int)gridShape[dim] - 1);
				}
				else {
					x0_vc[dim] = SimulationCell::modulo((int)fl, gridShape[dim]);
					x1_vc[dim] = SimulationCell::modulo((int)fl + 1, gridShape[dim]);
				}
			}
			cornerWeights[0] = x0.x() * x0.y() * x0.z();
			cornerWeights[1] = x1.x() * x0.y() * x0.z();
			cornerWeights[2] = x0.x() * x1.y() * x0.z();
			cornerWeights[3] = x0.x() * x0.y() * x1.z();
			cornerWeights[4] = x1.x() * x0.y() * x1.z();
			cornerWeights[5] = x0.x() * x1.y() * x1.z();
			cornerWeights[6] = x1.x() * x1.y() * x0.z();
			cornerWeights[7] = x1.x() * x1.y() * x1.z();
			cornerIndices[0] = x0_vc.x() + x0_vc.y() * gridShape[0] + x0_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[1] = x1_vc.x() + x0_vc.y() * gridShape[0] + x0_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[2] = x0_vc.x() + x1_vc.y() * gridShape[0] + x0_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[3] = x0_vc.x() + x0_vc.y() * gridShape[0] + x1_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[4] = x1_vc.x() + x0_vc.y() * gridShape[0] + x1_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[5] = x0_vc.x() + x1_vc.y() * gridShape[0] + x1_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[6] = x1_vc.x() + x1_vc.y() * gridShape[0] + x0_vc.z() * gridShape[0] * gridShape[1];
			cornerIndices[7] = x1_vc.x() + x1_vc.y() * gridShape[0] + x1_vc.z() * gridShape[0] * gridShape[1];
			for(auto& p : propertyMapping) {
				for(size_t component = 0; component < p.first.componentCount(); component++) {
					FloatType v = 0;
					for(size_t i = 0; i < 8; i++)
						v += cornerWeights[i] * p.first.get<FloatType>(cornerIndices[i], component);
					p.second.set<FloatType>(vertexIndex, component, v);
				}
			}
		});
	}
	return !task.isCanceled();
}

}	// End of namespace
}	// End of namespace
