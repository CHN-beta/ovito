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
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/app/Application.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "ColorByTypeModifier.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ColorByTypeModifier);
DEFINE_PROPERTY_FIELD(ColorByTypeModifier, sourceProperty);
SET_PROPERTY_FIELD_LABEL(ColorByTypeModifier, sourceProperty, "Property");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ColorByTypeModifier::ColorByTypeModifier(DataSet* dataset) : GenericPropertyModifier(dataset)
{
	// Operate on particles by default.
	setDefaultSubject(QStringLiteral("Particles"), QStringLiteral("ParticlesObject"));
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ColorByTypeModifier::initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext)
{
	GenericPropertyModifier::initializeModifier(time, modApp, executionContext);

	if(sourceProperty().isNull() && subject()) {

		// When the modifier is first inserted, automatically select the most recently added
		// typed property (in GUI mode) or the canonical type property (in script mode).
		const PipelineFlowState& input = modApp->evaluateInputSynchronous(time);
		if(const PropertyContainer* container = input.getLeafObject(subject())) {
			PropertyReference bestProperty;
			for(const PropertyObject* property : container->properties()) {
				if(property->elementTypes().empty() == false && property->componentCount() == 1 && property->dataType() == PropertyObject::Int) {
					if(executionContext == ExecutionContext::Interactive || property->type() == PropertyObject::GenericTypeProperty) {
						bestProperty = PropertyReference(subject().dataClass(), property);
					}
				}
			}
			if(!bestProperty.isNull())
				setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ColorByTypeModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Whenever the selected property class of this modifier is changed, update the source property reference accordingly.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::subject) && !isBeingLoaded() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToContainerClass(subject().dataClass()));
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void ColorByTypeModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
#ifdef OVITO_BUILD_BASIC
	throwException(tr("%1: This program feature is only available in OVITO Pro. Please visit our website www.ovito.org for more information.").arg(objectTitle()));
#else
	if(!subject())
		throwException(tr("No input element type selected."));
	if(sourceProperty().isNull())
		throwException(tr("No input property selected."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().containerClass() != subject().dataClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(subject().dataClass()->pythonName()).arg(sourceProperty().containerClass()->propertyClassDisplayName()));

	PropertyContainer* container = state.expectMutableLeafObject(subject());
	container->verifyIntegrity();

	// Get the input property.
	const PropertyObject* typePropertyObject = sourceProperty().findInContainer(container);
	if(!typePropertyObject)
		throwException(tr("The selected input property '%1' is not present.").arg(sourceProperty().name()));
	if(typePropertyObject->componentCount() != 1)
		throwException(tr("The input property '%1' has the wrong number of components. Must be a scalar property.").arg(typePropertyObject->name()));
	if(typePropertyObject->dataType() != PropertyObject::Int)
		throwException(tr("The input property '%1' has the wrong data type. Must be an integer property.").arg(typePropertyObject->name()));
	ConstPropertyAccess<int> typeProperty = typePropertyObject;

	// Create the color output property.
	PropertyAccess<Color> colorProperty = container->createProperty(PropertyObject::GenericColorProperty, false, Application::instance()->executionContext());

	// Create color lookup table.
	const std::map<int,Color> colorMap = typePropertyObject->typeColorMap();

	// Fill color property.
	boost::transform(typeProperty, colorProperty.begin(), [&](int type) {
		auto c = colorMap.find(type);
		if(c == colorMap.end())
			return Color(1,1,1);
		else
			return c->second;
	});
#endif
}

}	// End of namespace
}	// End of namespace
