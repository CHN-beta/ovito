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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/stdobj/properties/PropertyContainerClass.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include "PropertyContainerParameterUI.h"

namespace Ovito::StdObj {

IMPLEMENT_OVITO_CLASS(PropertyContainerParameterUI);

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyContainerParameterUI::PropertyContainerParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField) :
	PropertyParameterUI(parentEditor, propField),
	_comboBox(new QComboBox())
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(comboBox(), &QComboBox::textActivated, this, &PropertyContainerParameterUI::updatePropertyValue);
#else
	connect(comboBox(), qOverload<const QString&>(&QComboBox::activated), this, &PropertyContainerParameterUI::updatePropertyValue);
#endif

	// Update the list whenever the pipeline input changes.
	connect(parentEditor, &PropertiesEditor::pipelineInputChanged, this, &PropertyContainerParameterUI::updateUI);
}

/******************************************************************************
* Destructor.
******************************************************************************/
PropertyContainerParameterUI::~PropertyContainerParameterUI()
{
	delete comboBox();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void PropertyContainerParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(comboBox())
		comboBox()->setEnabled(editObject() && isEnabled());
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the
* properties owner this parameter UI belongs to.
******************************************************************************/
void PropertyContainerParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(comboBox() && editObject()) {

		// Get the current property class.
		QVariant val = editObject()->getPropertyFieldValue(propertyField());
		OVITO_ASSERT_MSG(val.isValid() && val.canConvert<PropertyContainerReference>(), "PropertyContainerParameterUI::updateUI()", qPrintable(QString("The property field of object class %1 is not of type <PropertyContainerClassPtr> or <PropertyContainerReference>.").arg(editObject()->metaObject()->className())));
		PropertyContainerReference selectedPropertyContainer = val.value<PropertyContainerReference>();

		// Update list of property containers available in the pipeline.
		comboBox()->clear();
		int selectedIndex = -1;
		bool currentContainerFilteredOut = false;
		for(const PipelineFlowState& state : editor()->getPipelineInputs()) {
			std::vector<ConstDataObjectPath> containers = state.getObjectsRecursive(PropertyContainer::OOClass());
			for(const ConstDataObjectPath& path : containers) {
				const PropertyContainer* container = static_object_cast<PropertyContainer>(path.back());

				PropertyContainerReference propRef(path);

				// The client can apply a custom filter function to the container list.
				if(_containerFilter && !_containerFilter(container)) {
					if(selectedPropertyContainer == propRef)
						currentContainerFilteredOut = true;
					continue;
				}

				// Do not add the same container to the list more than once.
				bool existsAlready = false;
				for(int i = 0; i < comboBox()->count(); i++) {
					PropertyContainerReference containerRef = comboBox()->itemData(i).value<PropertyContainerReference>();
					if(containerRef == propRef) {
						existsAlready = true;
						break;
					}
				}
				if(existsAlready)
					continue;

				if(propRef == selectedPropertyContainer)
					selectedIndex = comboBox()->count();

				comboBox()->addItem(propRef.dataTitle(), QVariant::fromValue(propRef));
			}
		}

		static QIcon warningIcon(QStringLiteral(":/guibase/mainwin/status/status_warning.png"));
		if(selectedIndex < 0) {
			if(selectedPropertyContainer) {
				// Add a place-holder item if the selected container does not exist anymore.
				QString title = selectedPropertyContainer.dataTitle();
				if(title.isEmpty() && selectedPropertyContainer.dataClass())
					title = selectedPropertyContainer.dataClass()->propertyClassDisplayName();
				if(!currentContainerFilteredOut)
					title += tr(" (not available)");
				comboBox()->addItem(title, QVariant::fromValue(selectedPropertyContainer));
				QStandardItem* item = static_cast<QStandardItemModel*>(comboBox()->model())->item(comboBox()->count()-1);
				item->setIcon(warningIcon);
			}
			else if(comboBox()->count() != 0) {
				comboBox()->addItem(tr("<Please select a data object>"));
			}
			selectedIndex = comboBox()->count() - 1;
		}
		if(comboBox()->count() == 0) {
			comboBox()->addItem(tr("<No available data objects>"));
			QStandardItem* item = static_cast<QStandardItemModel*>(comboBox()->model())->item(0);
			item->setIcon(warningIcon);
			selectedIndex = 0;
		}

		comboBox()->setCurrentIndex(selectedIndex);

		// Sort list entries alphabetically.
		static_cast<QStandardItemModel*>(comboBox()->model())->sort(0);
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void PropertyContainerParameterUI::updatePropertyValue()
{
	if(comboBox() && editObject()) {
		undoableTransaction(tr("Select input data object"), [this]() {

			PropertyContainerReference containerRef = comboBox()->currentData().value<PropertyContainerReference>();

			// Check if new value differs from old value.
			QVariant oldval = editObject()->getPropertyFieldValue(propertyField());
			if(containerRef == oldval.value<PropertyContainerReference>())
				return;

			editObject()->setPropertyFieldValue(propertyField(), QVariant::fromValue(containerRef));

			Q_EMIT valueEntered();
		});
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void PropertyContainerParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(comboBox()) 
		comboBox()->setEnabled(editObject() && isEnabled());
}

}	// End of namespace
