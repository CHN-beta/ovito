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
#include <ovito/gui/desktop/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(BooleanRadioButtonParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
BooleanRadioButtonParameterUI::BooleanRadioButtonParameterUI(PropertiesEditor* parentEditor, const char* propertyName) :
	PropertyParameterUI(parentEditor, propertyName)
{
	_buttonGroup = new QButtonGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_buttonGroup.data(), &QButtonGroup::idClicked, this, &BooleanRadioButtonParameterUI::updatePropertyValue);
#else
	connect(_buttonGroup.data(), (void (QButtonGroup::*)(int))&QButtonGroup::buttonClicked, this, &BooleanRadioButtonParameterUI::updatePropertyValue);
#endif

	QRadioButton* buttonNo = new QRadioButton();
	QRadioButton* buttonYes = new QRadioButton();
	_buttonGroup->addButton(buttonNo, 0);
	_buttonGroup->addButton(buttonYes, 1);
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
BooleanRadioButtonParameterUI::BooleanRadioButtonParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& propField) :
	PropertyParameterUI(parentEditor, propField)
{
	_buttonGroup = new QButtonGroup(this);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	connect(_buttonGroup.data(), &QButtonGroup::idClicked, this, &BooleanRadioButtonParameterUI::updatePropertyValue);
#else
	connect(_buttonGroup.data(), (void (QButtonGroup::*)(int))&QButtonGroup::buttonClicked, this, &BooleanRadioButtonParameterUI::updatePropertyValue);
#endif

	QRadioButton* buttonNo = new QRadioButton();
	QRadioButton* buttonYes = new QRadioButton();
	_buttonGroup->addButton(buttonNo, 0);
	_buttonGroup->addButton(buttonYes, 1);
}

/******************************************************************************
* Destructor.
******************************************************************************/
BooleanRadioButtonParameterUI::~BooleanRadioButtonParameterUI()
{
	// Release GUI controls.
	delete buttonTrue();
	delete buttonFalse();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanRadioButtonParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(buttonGroup()) {
		for(QAbstractButton* button : buttonGroup()->buttons())
			button->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void BooleanRadioButtonParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(buttonGroup() && editObject()) {
		QVariant val;
		if(propertyName()) {
			val = editObject()->property(propertyName());
			OVITO_ASSERT_MSG(val.isValid(), "BooleanRadioButtonParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to boolean type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
			if(!val.isValid()) {
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to boolean type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
			}
		}
		else if(propertyField()) {
			val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(val.isValid());
		}
		bool state = val.toBool();
		if(state && buttonTrue())
			buttonTrue()->setChecked(true);
		else if(!state && buttonFalse())
			buttonFalse()->setChecked(true);
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void BooleanRadioButtonParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(buttonGroup()) {
		for(QAbstractButton* button : buttonGroup()->buttons())
			button->setEnabled(editObject() != NULL && isEnabled());
	}
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field
* this property UI is bound to.
******************************************************************************/
void BooleanRadioButtonParameterUI::updatePropertyValue()
{
	if(buttonGroup() && editObject()) {
		undoableTransaction(tr("Change parameter"), [this]() {
			int id = buttonGroup()->checkedId();
			if(id != -1) {
				QVariant oldval;
				if(propertyName()) {
					oldval = editObject()->property(propertyName());
				}
				else if(propertyField()) {
					oldval = editObject()->getPropertyFieldValue(*propertyField());
				}
				if((bool)id != oldval.toBool()) {
					if(propertyName()) {
						if(!editObject()->setProperty(propertyName(), (bool)id)) {
							OVITO_ASSERT_MSG(false, "BooleanRadioButtonParameterUI::updatePropertyValue()", qPrintable(QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className())));
						}
					}
					else if(propertyField()) {
						editor()->changePropertyFieldValue(*propertyField(), (bool)id);
					}
					Q_EMIT valueEntered();
				}
			}
		});
	}
}

}	// End of namespace
