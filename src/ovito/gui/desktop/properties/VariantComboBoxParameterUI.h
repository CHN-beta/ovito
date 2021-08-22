////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 OVITO GmbH, Germany
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
* This UI lets the user change a property using a combo box widget.
******************************************************************************/
class OVITO_GUI_EXPORT VariantComboBoxParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(VariantComboBoxParameterUI)

public:

	/// Constructor.
	VariantComboBoxParameterUI(QObject* parentEditor, const char* propertyName);

	/// Constructor for a PropertyField property.
	VariantComboBoxParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField);

	/// Destructor.
	virtual ~VariantComboBoxParameterUI();

	/// This returns the combo box managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the tooltip text for the combo box widget.
	void setToolTip(const QString& text) const {
		if(comboBox()) comboBox()->setToolTip(text);
	}

	/// Sets the What's This helper text for the combo box.
	void setWhatsThis(const QString& text) const {
		if(comboBox()) comboBox()->setWhatsThis(text);
	}

public:

	Q_PROPERTY(QComboBox comboBox READ comboBox)

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

protected:

	/// The combo box of the UI component.
	QPointer<QComboBox> _comboBox;
};

}	// End of namespace
