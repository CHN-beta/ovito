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
* This UI allows the user to select a filename as property value.
******************************************************************************/
class OVITO_GUI_EXPORT FilenameParameterUI : public PropertyParameterUI
{
	OVITO_CLASS(FilenameParameterUI)

public:

	/// Constructor for a Qt property.
	FilenameParameterUI(PropertiesEditor* parentEditor, const char* propertyName);

	/// Constructor for a PropertyField property.
	FilenameParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField);

	/// Destructor.
	virtual ~FilenameParameterUI();

	/// This returns the button managed by this ParameterUI.
	QPushButton* selectorWidget() const { return _selectorButton; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the What's This helper text for the selector widget.
	void setWhatsThis(const QString& text) const {
		if(selectorWidget()) selectorWidget()->setWhatsThis(text);
	}

public:

	Q_PROPERTY(QPushButton selectorWidget READ selectorWidget)

Q_SIGNALS:

	/// This signal is emitted when the file selector should be shown to let the user select a new file.
	void showSelectionDialog();

protected:

	/// The selector control.
	QPointer<QPushButton> _selectorButton;
};

}	// End of namespace


