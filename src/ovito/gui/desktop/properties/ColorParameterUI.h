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
#include <ovito/gui/desktop/widgets/general/ColorPickerWidget.h>
#include "ParameterUI.h"

namespace Ovito {

/******************************************************************************
* Allows the user to edit a color parameter in the properties panel.
******************************************************************************/
class OVITO_GUI_EXPORT ColorParameterUI : public PropertyParameterUI
{
	Q_OBJECT
	OVITO_CLASS(ColorParameterUI)

public:

	/// Constructor.
	ColorParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& propField);

	/// Destructor.
	virtual ~ColorParameterUI();

	/// This returns the color picker widget managed by this parameter UI.
	ColorPickerWidget* colorPicker() const { return _colorPicker; }

	/// This returns a label for the color picker managed by this ColorPropertyUI.
	/// The text of the label widget is taken from the description text stored along
	/// with the property field.
	QLabel* label() const { return _label; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the What's This helper text for the label and the color picker.
	void setWhatsThis(const QString& text) const {
		if(label()) label()->setWhatsThis(text);
		if(colorPicker()) colorPicker()->setWhatsThis(text);
	}

public:

	Q_PROPERTY(QLabel* label READ label)
	Q_PROPERTY(QWidget* colorPicker READ colorPicker)

public Q_SLOTS:

	/// Is called when the user has changed the color.
	void onColorPickerChanged();

protected:

	/// The color picker control of the UI component.
	QPointer<ColorPickerWidget> _colorPicker;

	/// The label of the UI component.
	QPointer<QLabel> _label;
};

}	// End of namespace


