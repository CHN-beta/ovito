////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(IntegerRadioButtonParameterUI);

/******************************************************************************
* The constructor.
******************************************************************************/
IntegerRadioButtonParameterUI::IntegerRadioButtonParameterUI(PropertiesEditor* parentEditor, const char* propertyName) :
	PropertyParameterUI(parentEditor, propertyName)
{
	_buttonGroup = new QButtonGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_buttonGroup.data(), &QButtonGroup::idClicked, this, &IntegerRadioButtonParameterUI::updatePropertyValue);
#else
	connect(_buttonGroup.data(), static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &IntegerRadioButtonParameterUI::updatePropertyValue);
#endif
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
IntegerRadioButtonParameterUI::IntegerRadioButtonParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& propField) :
	PropertyParameterUI(parentEditor, propField)
{
	_buttonGroup = new QButtonGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_buttonGroup.data(), &QButtonGroup::idClicked, this, &IntegerRadioButtonParameterUI::updatePropertyValue);
#else
	connect(_buttonGroup.data(), static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &IntegerRadioButtonParameterUI::updatePropertyValue);
#endif
}

/******************************************************************************
* Creates a new radio button widget that can be selected by the user
* to set the property value to the given value.
******************************************************************************/
QRadioButton* IntegerRadioButtonParameterUI::addRadioButton(int value, const QString& caption)
{
	QRadioButton* button = new QRadioButton(caption);
	if(buttonGroup()) {
		button->setEnabled(editObject() && isEnabled());
		buttonGroup()->addButton(button, value);
	}
	return button;
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void IntegerRadioButtonParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(buttonGroup()) {
		for(QAbstractButton* button : buttonGroup()->buttons()) {
			if(isReferenceFieldUI())
				button->setEnabled(parameterObject() && isEnabled());
			else
				button->setEnabled(editObject() && isEnabled());
		}
	}

	if(isReferenceFieldUI() && editObject()) {
		// Update the displayed value when the animation time has changed.
		connect(dataset()->container(), &DataSetContainer::timeChanged, this, &IntegerRadioButtonParameterUI::updateUI, Qt::UniqueConnection);
	}
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void IntegerRadioButtonParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(buttonGroup() && editObject()) {
		int id = buttonGroup()->checkedId();
		if(isReferenceFieldUI()) {
			if(Controller* ctrl = dynamic_object_cast<Controller>(parameterObject()))
				id = ctrl->currentIntValue();
		}
		else {
			if(isQtPropertyUI()) {
				QVariant val = editObject()->property(propertyName());
				OVITO_ASSERT_MSG(val.isValid() && val.canConvert<int>(), "IntegerRadioButtonParameterUI::updateUI()", qPrintable(QString("The object class %1 does not define a property with the name %2 that can be cast to integer type.").arg(editObject()->metaObject()->className(), QString(propertyName()))));
				if(!val.isValid() || !val.canConvert<int>()) {
					editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to integer type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
				}
				id = val.toInt();
			}
			else if(isPropertyFieldUI()) {
				QVariant val = editObject()->getPropertyFieldValue(*propertyField());
				OVITO_ASSERT(val.isValid());
				id = val.toInt();
			}
		}
		QAbstractButton* btn = buttonGroup()->button(id);
		if(btn != NULL)
			btn->setChecked(true);
		else {
			btn = buttonGroup()->checkedButton();
			if(btn) btn->setChecked(false);
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void IntegerRadioButtonParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(buttonGroup()) {
		for(QAbstractButton* button : buttonGroup()->buttons()) {
			if(isReferenceFieldUI())
				button->setEnabled(parameterObject() && isEnabled());
			else
				button->setEnabled(editObject() && isEnabled());
		}
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void IntegerRadioButtonParameterUI::updatePropertyValue()
{
	if(buttonGroup() && editObject()) {
		int id = buttonGroup()->checkedId();
		if(id != -1) {
			undoableTransaction(tr("Change parameter"), [this, id]() {
				if(isReferenceFieldUI()) {
					if(Controller* ctrl = dynamic_object_cast<Controller>(parameterObject())) {
						ctrl->setCurrentIntValue(id);
						updateUI();
					}
				}
				else if(isQtPropertyUI()) {
					if(!editObject()->setProperty(propertyName(), id)) {
						OVITO_ASSERT_MSG(false, "IntegerRadioButtonPropertyUI::updatePropertyValue()", qPrintable(QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className())));
					}
				}
				else if(isPropertyFieldUI()) {
					editor()->changePropertyFieldValue(*propertyField(), id);
				}
				Q_EMIT valueEntered();
			});
		}
	}
}

}	// End of namespace
