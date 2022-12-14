////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
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
#include <ovito/core/utilities/units/UnitsManager.h>

namespace Ovito {

/**
 * \brief A spinner control for editing a numeric value.
 */
class OVITO_GUI_EXPORT SpinnerWidget : public QWidget
{
	Q_OBJECT

public:

	/// \brief Constructs the spinner control.
	/// \param parent The parent widget for the spinner widget.
	/// \param textBox The text box to which this spinner should be connected.
	SpinnerWidget(QWidget* parent = nullptr, QLineEdit* textBox = nullptr);

	/// \brief Returns the text box connect to this spinner.
	/// \return The text box widget or \c NULL if the spinner has not been connected to a text box.
	/// \sa setTextBox()
	QLineEdit* textBox() const { return _textBox; }

	/// \brief Connects this spinner to the given text box widget.
	/// \param textBox The text box or \c NULL to disconnect the spinner from the old text box.
	/// \sa textBox()
	void setTextBox(QLineEdit* textBox);

	/// \brief Gets the current value of the spinner.
	/// \return The current value in native units.
	/// \sa setFloatValue()
	/// \sa intValue()
	FloatType floatValue() const { return _value; }

	/// \brief Sets the current value of the spinner.
	/// \param newVal The new value in native units.
	/// \param emitChangeSignal Controls whether the spinner should emit
	///                         a spinnerValueChanged() signal when \a newVal is
	///                         not equal to the old spinner value.
	void setFloatValue(FloatType newVal, bool emitChangeSignal = false);

	/// \brief Gets the current value of the spinner as an integer.
	/// \return The current value in native units.
	/// \sa setIntValue()
	/// \sa floatValue()
	int intValue() const { return (int)_value; }

	/// \brief Sets the current value of the spinner.
	/// \param newVal The new value in native units.
	/// \param emitChangeSignal Controls whether the spinner should emit
	///                         a spinnerValueChanged() signal when \a newVal is
	///                         not equal to the old spinner value.
	void setIntValue(int newVal, bool emitChangeSignal = false);

	/// \brief Gets the minimum allowed value of the spinner.
	/// \return The minimum value that can be specified for this spinner by the user.
	/// \sa maxValue()
	/// \sa setMinValue()
	FloatType minValue() const { return _minValue; }

	/// \brief Sets the minimum allowed value of the spinner.
	/// \param minValue The new minimum value.
	///
	/// If the current value of the spinner is less than the new minimum value,
	/// it will be set to the new minimum value.
	///
	/// \sa minValue()
	void setMinValue(FloatType minValue);

	/// \brief Gets the maximum allowed value of the spinner.
	/// \return The maximum value that can be specified for this spinner by the user.
	/// \sa minValue()
	/// \sa setMaxValue()
	FloatType maxValue() const { return _maxValue; }

	/// \brief sets the maximum allowed value of the spinner.
	/// \param maxValue The new maximum value.
	///
	/// If the current value of the spinner is greater than the new maximum value,
	/// it will be set to the new maximum value.
	///
	/// \sa maxValue()
	void setMaxValue(FloatType maxValue);

	/// \brief Returns the standard value that, if the spinner is set to this special value, should be highlighted in the input field.
	FloatType standardValue() const { return _standardValue; }

	/// \brief Specifies the standard value that, if the spinner is set to this special value, should be highlighted in the input field.
	void setStandardValue(FloatType value);

	/// \brief Returns the units of this spinner's value.
	/// \return The parameter unit object that performs the conversion from native units
	///         to user units and back.
	/// \sa setUnit()
	ParameterUnit* unit() const { return _unit; }

	/// \brief Sets the units of this spinner's value.
	/// \param unit The parameter unit object that performs the conversion from native units
	///         to user units and back.
	/// \sa unit()
	void setUnit(ParameterUnit* unit);

	/// \brief Returns whether the user currently dragging the spinner and changing its value interactively.
	/// \return \c true if the spinner is currently being dragged.
	bool isDragging() const { return _upperBtnPressed && _lowerBtnPressed; }

	/// \brief Returns recommended size for the widget.
	virtual QSize sizeHint() const override;

	/// \brief Returns the minimum size of the widget.
	virtual QSize minimumSizeHint() const override { return sizeHint(); }

Q_SIGNALS:

	/// \brief This signal is emitted by the spinner after its value has been changed by the user.
	void spinnerValueChanged();

	/// \brief This signal is emitted by the spinner when the user has started a drag operation.
	void spinnerDragStart();

	/// \brief This signal is emitted by the spinner when the user has finished the drag operation.
	void spinnerDragStop();

	/// \brief This signal is emitted by the spinner when the user has aborted the drag operation.
	void spinnerDragAbort();

public Q_SLOTS:

	/// \brief Updates the text of the connected text box after the spinner's value has changed.
	virtual void updateTextBox();

protected Q_SLOTS:

	/// \brief Is called when the user has entered a new text into the text box.
	///
	/// The text will be parsed and taken as the new value of the spinner.
	void onTextChanged();

protected:

	/// The edit box this spinner is connected to.
	QPointer<QLineEdit> _textBox;

	/// Returns the current formatting mode that is used to convert the spinner value
	/// to the string shown in the text box and vice versa.
	QPointer<ParameterUnit> _unit;

	/// The current value of the spinner.
	FloatType _value = 0;

	/// The lower limit of the spinner value.
	FloatType _minValue = FLOATTYPE_MIN;

	/// The upper limit of the spinner value.
	FloatType _maxValue = FLOATTYPE_MAX;

	/// The standard value that, if set in the spinner, should be highlighted.
	FloatType _standardValue = std::numeric_limits<FloatType>::quiet_NaN();

	/// The current step size used by the spinner.
	/// This stays constant during a drag operation.
	FloatType _currentStepSize;

	/// Backup value for when aborting spinner change.
	FloatType _oldValue;

	/// The text that has been set in the text box by the spinner control.
	QString _originalText;

	/// Indicates if the upper spinner button is currently pressed.
	bool _upperBtnPressed = false;

	/// Indicates if the lower spinner button is currently pressed.
	bool _lowerBtnPressed = false;

	/// Saves the start mouse position for dragging.
	int _startMouseY;

	/// Saves the last mouse position for dragging.
	int _lastMouseY;

protected:

	virtual void paintEvent(QPaintEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void changeEvent(QEvent* event) override;
	virtual void focusOutEvent(QFocusEvent* event) override;
};

}	// End of namespace
