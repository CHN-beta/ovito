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
* Allows the user to edit a boolean parameter.
* The check box is displayed in the title bar of a QGroupBox.
******************************************************************************/
class OVITO_GUI_EXPORT BooleanGroupBoxParameterUI : public PropertyParameterUI
{
	OVITO_CLASS(BooleanGroupBoxParameterUI)

public:

	/// Constructor for a Qt property.
	BooleanGroupBoxParameterUI(PropertiesEditor* parentEditor, const char* propertyName, const QString& label);

	/// Constructor for a PropertyField property.
	BooleanGroupBoxParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField);

	/// Destructor, which releases all GUI controls.
	virtual ~BooleanGroupBoxParameterUI();

	/// This returns the group box managed by this parameter UI.
	QGroupBox* groupBox() const { return _groupBox; }

	/// This returns the container managed by this parameter UI where child widgets should be inserted into.
	QWidget* childContainer() const { return _childContainer; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the tooltip text for the check box.
	void setToolTip(const QString& text) const { if(groupBox()) groupBox()->setToolTip(text); }

	/// Sets the What's This helper text for the check box.
	void setWhatsThis(const QString& text) const { if(groupBox()) groupBox()->setWhatsThis(text); }

public:

	Q_PROPERTY(QGroupBox groupBox READ groupBox)

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

protected:

	/// The QGroupBox control of the UI component.
	QPointer<QGroupBox> _groupBox;

	/// A container widget where child widgets should be inserted into.
	QWidget* _childContainer;
};

}	// End of namespace


