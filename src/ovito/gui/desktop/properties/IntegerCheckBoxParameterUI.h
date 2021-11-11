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

#pragma once


#include <ovito/gui/desktop/GUI.h>
#include "ParameterUI.h"

namespace Ovito {

/******************************************************************************
* This UI element lets the user change an integer-value property of the object
* being edited using a check box.
******************************************************************************/
class OVITO_GUI_EXPORT IntegerCheckBoxParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(IntegerCheckBoxParameterUI)

public:

	/// Constructor.
	IntegerCheckBoxParameterUI(PropertiesEditor* parentEditor, const char* propertyName, const QString& checkBoxLabel, int uncheckedValue, int checkedValue);

	/// Constructor for a PropertyField property.
	IntegerCheckBoxParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField, int uncheckedValue, int checkedValue);

	/// Destructor.
	virtual ~IntegerCheckBoxParameterUI();

	/// This returns the checkbox managed by this parameter UI.
	QCheckBox* checkBox() const { return _checkBox; }

	/// Changes the parameter value that represents the unchecked state.
	void setUncheckedValue(int uncheckedValue) {
		if(_uncheckedValue != uncheckedValue) {
			_uncheckedValue = uncheckedValue;
			updateUI();
		}
	}

	/// Changes the parameter value that represents the checked state.
	void setCheckedValue(int checkedValue) {
		if(_checkedValue != checkedValue) {
			_checkedValue = checkedValue;
			updateUI();
		}
	}

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the tooltip text for the check box.
	void setToolTip(const QString& text) const { if(checkBox()) checkBox()->setToolTip(text); }

	/// Sets the What's This helper text for the check box.
	void setWhatsThis(const QString& text) const { if(checkBox()) checkBox()->setWhatsThis(text); }

public:

	Q_PROPERTY(QCheckBox checkBox READ checkBox)

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

protected:

	/// The check box of the UI component.
	QPointer<QCheckBox> _checkBox;

	/// The parameter value that represents the unchecked state.
	int _uncheckedValue = 0;

	/// The parameter value that represents the checked state.
	int _checkedValue = 1;
};

}	// End of namespace
