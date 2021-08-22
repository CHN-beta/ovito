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

#include <ovito/gui/qml/GUI.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "PropertyReferenceParameterUI.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyReferenceParameterUI);

/******************************************************************************
* Sets the property container from which the user can choose a property.
******************************************************************************/
void PropertyReferenceParameterUI::setPropertyContainer(const QVariant& dataObjectReference)
{
	OVITO_ASSERT(!dataObjectReference.isValid() || dataObjectReference.canConvert<PropertyContainerReference>());
	_containerReference = dataObjectReference.value<PropertyContainerReference>();
	// The selected property container has changed -> update list of available properties.
	updatePropertyList();
	// The index of the selected list item may have changed. Update the index as well.
	updateUI();
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PropertyReferenceParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ModifierInputChanged) {
		// The modifier's input from the pipeline has changed -> update list of available input properties.
		updatePropertyList();
		// The index of the selected list item may have changed. Update the index as well.
		updateUI();
	}
	return ParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Rebuilds the list of available property objects the user can choose from.
******************************************************************************/
void PropertyReferenceParameterUI::updatePropertyList()
{
	std::vector<PropertyReference> acceptedProperties;
	std::vector<QString> acceptedTexts;

	if(_containerReference) {
		if(Modifier* mod = static_object_cast<Modifier>(editObject())) {
			for(ModifierApplication* modApp : mod->modifierApplications()) {
				const PipelineFlowState& state = modApp->evaluateInputSynchronous(mod->dataset()->animationSettings()->time());

				if(const PropertyContainer* container = state ? state.getLeafObject(_containerReference) : nullptr) {
					for(const PropertyObject* property : container->properties()) {

						// The client can apply a filter to the displayed property list.
						if(acceptablePropertyType() == OnlyTypedProperties && !property->isTypedProperty())
							continue;

						// Properties with a non-numeric data type cannot be used as input properties.
						if(property->dataType() != PropertyObject::Int && property->dataType() != PropertyObject::Int64 && property->dataType() != PropertyObject::Float)
							continue;

						if(_componentsMode != ShowOnlyComponents || (property->componentCount() <= 1 && property->componentNames().empty())) {
							// Property without component:
							acceptedProperties.emplace_back(&container->getOOMetaClass(), property);
							acceptedTexts.push_back(property->name());
						}
						if(_componentsMode != ShowNoComponents && property->componentCount() > 1) {
							// Components of vector property:
							for(int vectorComponent = 0; vectorComponent < (int)property->componentCount(); vectorComponent++) {
								acceptedProperties.emplace_back(&container->getOOMetaClass(), property, vectorComponent);
								if(_componentsMode == ShowComponentsAndVectorProperties)
									acceptedTexts.push_back(QStringLiteral("  ") + property->nameWithComponent(vectorComponent));
								else
									acceptedTexts.push_back(property->nameWithComponent(vectorComponent));
							}
						}
					}
				}
			}

			// Get the currently selected property from the modifier's parameter field.
			QVariant val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT_MSG(val.canConvert<PropertyReference>(), "PropertyReferenceParameterUI::updatePropertyList()", qPrintable(QString("The property field of object class %1 is not of type <PropertyReference>.").arg(editObject()->metaObject()->className())));
			PropertyReference selectedProperty = val.value<PropertyReference>();

			// Add a placeholder item if the selected property does not exist anymore in the modifier's input.
			if(selectedProperty && boost::find(acceptedProperties, selectedProperty) == acceptedProperties.end()) {
				QString title;
				selectedProperty = selectedProperty.convertToContainerClass(_containerReference.dataClass());
				if(selectedProperty.type() != PropertyObject::GenericUserProperty)
					title = selectedProperty.containerClass()->standardPropertyName(selectedProperty.type());
				else
					title = selectedProperty.name();
				title += tr(" (not available)");
				acceptedProperties.push_back(std::move(selectedProperty));
				acceptedTexts.push_back(std::move(title));
			}
		}
	}

	_model->resetList(std::move(acceptedProperties), std::move(acceptedTexts));
}

/******************************************************************************
* Obtains the current value of the parameter from the C++ object.
******************************************************************************/
QVariant PropertyReferenceParameterUI::getCurrentValue() const
{
	if(_containerReference && editObject()) {

		// Get the currently selected property from the modifier's parameter field.
		QVariant val = editObject()->getPropertyFieldValue(*propertyField());
		OVITO_ASSERT_MSG(val.canConvert<PropertyReference>(), "PropertyReferenceParameterUI::getCurrentValue()", qPrintable(QString("The property field of object class %1 is not of type <PropertyReference>.").arg(editObject()->metaObject()->className())));
		PropertyReference selectedProperty = val.value<PropertyReference>();

		// Look up its index in the list.
		auto iter = boost::find(_model->properties(), selectedProperty);
		if(iter != _model->properties().cend())
			return QVariant::fromValue(std::distance(_model->properties().cbegin(), iter));
	}
	return -1;
}

/******************************************************************************
* Changes the current value of the C++ object parameter.
******************************************************************************/
void PropertyReferenceParameterUI::setCurrentValue(const QVariant& val)
{
	if(_containerReference && editObject()) {
		int index = val.toInt();
		if(index >= 0 && index < _model->properties().size()) {
			const PropertyReference& oldVal = editObject()->getPropertyFieldValue(*propertyField()).value<PropertyReference>();
			const PropertyReference& newVal = _model->properties()[index];
			if(newVal != oldVal) {
				UndoableTransaction::handleExceptions(editObject()->dataset()->undoStack(), tr("Select property"), [&]() {
					editObject()->setPropertyFieldValue(*propertyField(), QVariant::fromValue(newVal));
				});
			}
		}
	}
}

/******************************************************************************
* Returns the data stored in the model under the given role.
******************************************************************************/
QVariant PropertyReferenceParameterUI::Model::data(const QModelIndex& index, int role) const
{
	if(index.isValid()) {
		if(index.row() < properties().size()) {
			if(role == Qt::DisplayRole)
				return _texts[index.row()];
			else if(role == Qt::UserRole)
				return QVariant::fromValue(_properties[index.row()]);
		}
		else if(properties().empty()) {
			if(role == Qt::DisplayRole)
				return tr("<No available properties>");
		}
	}
	return QVariant();
}

}	// End of namespace
}	// End of namespace
