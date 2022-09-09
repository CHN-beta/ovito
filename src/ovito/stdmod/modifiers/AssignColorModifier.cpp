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
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/app/Application.h>
#include "AssignColorModifier.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(AssignColorModifierDelegate);

IMPLEMENT_OVITO_CLASS(AssignColorModifier);
DEFINE_REFERENCE_FIELD(AssignColorModifier, colorController);
DEFINE_PROPERTY_FIELD(AssignColorModifier, keepSelection);
SET_PROPERTY_FIELD_LABEL(AssignColorModifier, colorController, "Color");
SET_PROPERTY_FIELD_LABEL(AssignColorModifier, keepSelection, "Keep selection");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AssignColorModifier::AssignColorModifier(ObjectCreationParams params) : DelegatingModifier(params),
	// In the graphical environment, we clear the selection by default to make the assigned colors visible.
	_keepSelection(!params.loadUserDefaults())
{
	if(params.createSubObjects()) {
		setColorController(ControllerManager::createColorController(dataset()));
		colorController()->setColorValue(0, Color(0.3f, 0.3f, 1.0f));

		// Let this modifier operate on particles by default.
		createDefaultModifierDelegate(AssignColorModifierDelegate::OOClass(), QStringLiteral("ParticlesAssignColorModifierDelegate"), params);
	}
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval AssignColorModifier::validityInterval(const ModifierEvaluationRequest& request) const
{
	TimeInterval iv = DelegatingModifier::validityInterval(request);
	if(colorController()) 
		iv.intersect(colorController()->validityInterval(request.time()));
	return iv;
}

/******************************************************************************
* Applies the modifier operation to the data in a pipeline flow state.
******************************************************************************/
PipelineStatus AssignColorModifierDelegate::apply(const ModifierEvaluationRequest& request, PipelineFlowState& state, const PipelineFlowState& inputState, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	const AssignColorModifier* mod = static_object_cast<AssignColorModifier>(request.modifier());
	if(!mod->colorController())
		return PipelineStatus::Success;

	// Look up the property container object and make sure we can safely modify it.
   	DataObjectPath objectPath = state.expectMutableObject(inputContainerRef());
	PropertyContainer* container = static_object_cast<PropertyContainer>(objectPath.back());

	// Get the input selection property.
	ConstPropertyPtr selProperty;
	if(container->getOOMetaClass().isValidStandardPropertyId(PropertyObject::GenericSelectionProperty)) {
		if(const PropertyObject* selPropertyObj = container->getProperty(PropertyObject::GenericSelectionProperty)) {
			selProperty = selPropertyObj;

			// Clear selection if requested.
			if(!mod->keepSelection())
				container->removeProperty(selPropertyObj);
		}
	}

	// Get modifier's color parameter value.
	Color color;
	mod->colorController()->getColorValue(request.time(), color, state.mutableStateValidity());

	// Create the color output property.
    PropertyObject* colorProperty = container->createProperty(outputColorPropertyId(), (bool)selProperty ? DataBuffer::InitializeMemory : DataBuffer::NoFlags, objectPath);
	// Assign color to selected elements (or all elements if there is no selection).
	colorProperty->fillSelected(color, selProperty.get());

	return PipelineStatus::Success;
}

}	// End of namespace
