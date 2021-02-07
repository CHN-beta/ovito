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
#include <ovito/gui/desktop/properties/FilenameParameterUI.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(FilenameParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
FilenameParameterUI::FilenameParameterUI(QObject* parentEditor, const char* propertyName) :
	PropertyParameterUI(parentEditor, propertyName)
{
	// Create UI widget.
	_selectorButton = new QPushButton(QStringLiteral(" "));
	connect(_selectorButton.data(), &QPushButton::clicked, this, &FilenameParameterUI::showSelectionDialog);
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
FilenameParameterUI::FilenameParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) :
	PropertyParameterUI(parentEditor, propField)
{
	// Create UI widget.
	_selectorButton = new QPushButton(QStringLiteral(" "));
	connect(_selectorButton.data(), &QPushButton::clicked, this, &FilenameParameterUI::showSelectionDialog);
}

/******************************************************************************
* Destructor.
******************************************************************************/
FilenameParameterUI::~FilenameParameterUI()
{
	// Release GUI controls.
	delete selectorWidget();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void FilenameParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();

	if(selectorWidget())
		selectorWidget()->setEnabled(editObject() && isEnabled());
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void FilenameParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();

	if(selectorWidget() && editObject()) {
		QVariant val;
		if(propertyName()) {
			val = editObject()->property(propertyName());
			OVITO_ASSERT_MSG(val.isValid() && val.canConvert<QString>(), "FilenameParameterUI::updateUI()", qPrintable(QString("The object class %1 does not define a property with the name %2 that can be cast to string type.").arg(editObject()->metaObject()->className(), QString(propertyName()))));
			if(!val.isValid() || !val.canConvert<QString>()) {
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to string type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
			}
		}
		else if(propertyField()) {
			val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(val.isValid());
		}

		QString filename = val.toString();
		if(filename.isEmpty() == false) {
			selectorWidget()->setText(QFileInfo(filename).fileName());
		}
		else {
			selectorWidget()->setText(tr("[Choose File...]"));
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void FilenameParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(selectorWidget()) 
		selectorWidget()->setEnabled(editObject() && isEnabled());
}

}	// End of namespace
