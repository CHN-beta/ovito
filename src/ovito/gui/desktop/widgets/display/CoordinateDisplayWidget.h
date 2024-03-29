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
#include <ovito/gui/desktop/widgets/general/SpinnerWidget.h>

namespace Ovito {

/**
 * The coordinate display widget at the bottom of the main window,
 * which displays the current mouse coordinates and the transform of the selected object.
 */
class OVITO_GUI_EXPORT CoordinateDisplayWidget : public QFrame
{
	Q_OBJECT

public:

	/// \brief Constructs the widget.
	CoordinateDisplayWidget(DataSetContainer& datasetContainer, QWidget* parent = nullptr);

	/// \brief Shows the coordinate display widget.
	void activate(const QString& undoOperationName);

	/// \brief Deactivates the coordinate display widget.
	void deactivate();

	/// Sets the values displayed by the coordinate display widget.
	void setValues(const Vector3& xyz) {
		if(!_spinners[0]->isDragging()) _spinners[0]->setFloatValue(xyz.x());
		if(!_spinners[1]->isDragging()) _spinners[1]->setFloatValue(xyz.y());
		if(!_spinners[2]->isDragging()) _spinners[2]->setFloatValue(xyz.z());
	}

	/// Returns the values displayed by the coordinate display widget.
	Vector3 getValues() const {
		return Vector3(
				_spinners[0]->floatValue(),
				_spinners[1]->floatValue(),
				_spinners[2]->floatValue());
	}

	/// Sets the units of the displayed values.
	void setUnit(ParameterUnit* unit) {
		_spinners[0]->setUnit(unit);
		_spinners[1]->setUnit(unit);
		_spinners[2]->setUnit(unit);
	}

Q_SIGNALS:

	/// This signal is emitted when the user has changed the value of one of the vector components.
	void valueEntered(int component, FloatType value);

	/// This signal is emitted when the user presses the "Animate transformation" button.
	void animatePressed();

protected Q_SLOT:

	/// Is called when a spinner value has been changed by the user.
	void onSpinnerValueChanged();

	/// Is called when the user has started a spinner drag operation.
	void onSpinnerDragStart();

	/// Is called when the user has finished the spinner drag operation.
	void onSpinnerDragStop();

	/// Is called when the user has aborted the spinner drag operation.
	void onSpinnerDragAbort();

private:

	DataSetContainer& _datasetContainer;
	SpinnerWidget* _spinners[3];
	QString _undoOperationName;
};

}	// End of namespace


