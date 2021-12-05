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
#include <ovito/gui/desktop/widgets/general/SpinnerWidget.h>
#include "ParameterUI.h"

namespace Ovito {

/******************************************************************************
* Base class for UI components that allow the user to edit a numerical
* property of an object via a spinner widget and a text box.
******************************************************************************/
class OVITO_GUI_EXPORT NumericalParameterUI : public PropertyParameterUI
{
	OVITO_CLASS(NumericalParameterUI)

public:

	/// Constructor for a Qt property.
	NumericalParameterUI(PropertiesEditor* parentEditor, const char* propertyName, const QMetaObject* defaultParameterUnitType, const QString& labelText = QString());

	/// Constructor for a PropertyField or ReferenceField property.
	NumericalParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField, const QMetaObject* defaultParameterUnitType);

	/// Destructor.
	virtual ~NumericalParameterUI();

	/// Returns a label for the control widget managed by this ParameterUI.
	QLabel* label() const { return _label; }

	/// Returns the spinner widget managed by this ParameterUI.
	SpinnerWidget* spinner() const { return _spinner; }

	/// Returns the text box managed by this ParameterUI.
	QLineEdit* textBox() const { return _textBox; }

	/// Returns the button, which invokes the animation key editor for this animatable parameter.
	QAbstractButton* animateButton() const { return _animateButton; }

	/// Creates a QLayout that contains the text box, the spinner widget, the animate button (if parameter is animatable).
	QLayout* createFieldLayout() const;

	/// Returns the type of unit conversion service, which is used to format the parameter value as a text string.
	const QMetaObject* parameterUnitType() const { return _parameterUnitType; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the tooltip text for the text box and the label widget.
	void setToolTip(const QString& text) const {
		if(label()) label()->setToolTip(text);
		if(textBox()) textBox()->setToolTip(text);
	}

	/// Sets the What's This helper text for the label, textbox, and the spinner.
	void setWhatsThis(const QString& text) const {
		if(label()) label()->setWhatsThis(text);
		if(textBox()) textBox()->setWhatsThis(text);
		if(spinner()) spinner()->setWhatsThis(text);
	}

public:

	Q_PROPERTY(SpinnerWidget spinner READ spinner)
	Q_PROPERTY(QLineEdit textBox READ textBox)
	Q_PROPERTY(QLabel label READ label)
	Q_PROPERTY(QAbstractButton animateButton READ animateButton)

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to. This method must be implemented by derived classes.
	virtual void updatePropertyValue() = 0;

protected Q_SLOTS:

	/// Is called when the spinner value has changed.
	void onSpinnerValueChanged();

	/// Is called when the user begins dragging the spinner interactively.
	void onSpinnerDragStart();

	/// Is called when the user stops dragging the spinner interactively.
	void onSpinnerDragStop();

	/// Is called when the user aborts dragging the spinner interactively.
	void onSpinnerDragAbort();

protected:

	/// the spinner control of the UI component.
	QPointer<SpinnerWidget> _spinner;

	/// The text box of the UI component.
	QPointer<QLineEdit> _textBox;

	/// The label of the UI component.
	QPointer<QLabel> _label;

	/// The button for editing animatable parameters.
	QPointer<QAbstractButton> _animateButton;

	/// The type of unit conversion service, which is used to format the parameter value as a text string.
	const QMetaObject* _parameterUnitType;

	/// Indicates that the user is currently dragging the spinner using the mouse.
	bool _isDraggingSpinner = false;

private:

	/// Creates the widgets for this property UI.
	void initUIControls(const QString& labelText);

	/// The signal/slot connection that informs the parameter UI about animation time changes.
	QMetaObject::Connection _animationTimeChangedConnection;
};

}	// End of namespace
