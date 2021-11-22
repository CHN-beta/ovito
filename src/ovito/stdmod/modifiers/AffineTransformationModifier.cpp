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

#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/simcell/PeriodicDomainDataObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "AffineTransformationModifier.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(AffineTransformationModifier);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, transformationTM);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, selectionOnly);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, targetCell);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, relativeMode);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, translationReducedCoordinates);
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, transformationTM, "Transformation");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, selectionOnly, "Transform selected elements only");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, targetCell, "Target cell shape");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, relativeMode, "Relative transformation");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, translationReducedCoordinates, "Relative transformation");

IMPLEMENT_OVITO_CLASS(AffineTransformationModifierDelegate);
IMPLEMENT_OVITO_CLASS(SimulationCellAffineTransformationModifierDelegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AffineTransformationModifier::AffineTransformationModifier(DataSet* dataset) : MultiDelegatingModifier(dataset),
	_selectionOnly(false),
	_transformationTM(AffineTransformation::Identity()),
	_targetCell(AffineTransformation::Zero()),
	_relativeMode(true),
	_translationReducedCoordinates(false)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void AffineTransformationModifier::initializeObject(ExecutionContext executionContext)
{
	// Generate the list of delegate objects.
	createModifierDelegates(AffineTransformationModifierDelegate::OOClass(), executionContext);
	
	MultiDelegatingModifier::initializeObject(executionContext);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a PipelineObject.
******************************************************************************/
void AffineTransformationModifier::initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext)
{
	MultiDelegatingModifier::initializeModifier(time, modApp, executionContext);

	// Take the simulation cell from the input object as the default destination cell geometry for absolute scaling.
	if(targetCell() == AffineTransformation::Zero()) {
		const PipelineFlowState& input = modApp->evaluateInputSynchronous(time);
		if(const SimulationCellObject* cell = input.getObject<SimulationCellObject>())
			setTargetCell(cell->cellMatrix());
	}
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void AffineTransformationModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Validate parameters and input data.
	if(!relativeMode()) {
		const SimulationCellObject* simCell = state.getObject<SimulationCellObject>();
		if(!simCell || simCell->cellMatrix().determinant() == 0)
			throwException(tr("Input simulation cell does not exist or is degenerate. Transformation to target cell would be singular."));
	}

	// Apply all enabled modifier delegates to the input data.
	MultiDelegatingModifier::evaluateSynchronous(time, modApp, state);
}

/******************************************************************************
* Returns the effective affine transformation matrix to be applied to points.
* It depends on the linear matrix, the translation vector, relative/target cell mode, and 
* whether the translation is specified in terms of reduced cell coordinates.
* Thus, the affine transformation may depend on the current simulation cell shape.
******************************************************************************/
AffineTransformation AffineTransformationModifier::effectiveAffineTransformation(const PipelineFlowState& state) const
{
	AffineTransformation tm;
	if(relativeMode()) {
		tm = transformationTM();
		if(translationReducedCoordinates()) {
			tm.translation() = tm * (state.expectObject<SimulationCellObject>()->matrix() * tm.translation());
		}	
	}
	else {
		tm = targetCell() * state.expectObject<SimulationCellObject>()->inverseMatrix();
	}

	return tm;
}

/******************************************************************************
* Asks the metaclass which data objects in the given input data collection the
* modifier delegate can operate on.
******************************************************************************/
QVector<DataObjectReference> SimulationCellAffineTransformationModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<SimulationCellObject>())
		return { DataObjectReference(&SimulationCellObject::OOClass()) };
	if(input.containsObject<PeriodicDomainDataObject>())
		return { DataObjectReference(&PeriodicDomainDataObject::OOClass()) };
	return {};
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus SimulationCellAffineTransformationModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	AffineTransformationModifier* mod = static_object_cast<AffineTransformationModifier>(modifier);
	const AffineTransformation tm = mod->effectiveAffineTransformation(state);

	// Transform the SimulationCellObject.
	if(const SimulationCellObject* inputCell = state.getObject<SimulationCellObject>()) {
		SimulationCellObject* outputCell = state.makeMutable(inputCell);
		outputCell->setCellMatrix(mod->relativeMode() ? (tm * inputCell->cellMatrix()) : mod->targetCell());
	}

	if(mod->selectionOnly())
		return PipelineStatus::Success;

	// Transform the domains of PeriodicDomainDataObjects.
	for(const DataObject* obj : state.data()->objects()) {
		if(const PeriodicDomainDataObject* existingObject = dynamic_object_cast<PeriodicDomainDataObject>(obj)) {
			if(existingObject->domain()) {
				PeriodicDomainDataObject* newObject = state.makeMutable(existingObject);
				newObject->mutableDomain()->setCellMatrix(tm * existingObject->domain()->cellMatrix());
			}
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
