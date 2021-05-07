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

/**
 * \file ColorPickerWidget.h
 * \brief Contains the definition of the Ovito::ColorPickerWidget class.
 */

#pragma once


#include <ovito/gui/desktop/GUI.h>

namespace Ovito {

/**
 * \brief A UI control lets the user choose a color.
 */
class OVITO_GUI_EXPORT ColorPickerWidget : public QAbstractButton
{
	Q_OBJECT

public:

	/// \brief Constructs the color picker control.
	/// \param parent The parent widget for the widget.
	ColorPickerWidget(QWidget* parent = 0);

	/// \brief Gets the current value of the color picker.
	/// \return The current color.
	/// \sa setColor()
	const Color& color() const { return _color; }

	/// \brief Sets the current value of the color picker.
	/// \param newVal The new color value.
	/// \param emitChangeSignal Controls whether the control should emit
	///                         a colorChanged() signal when \a newVal is
	///                         not equal to the old color.
	/// \sa color()
	void setColor(const Color& newVal, bool emitChangeSignal = false);

	/// Returns the preferred size of the widget.
	virtual QSize sizeHint() const override;

Q_SIGNALS:

	/// \brief This signal is emitted by the color picker after its value has been changed by the user.
	void colorChanged();

protected Q_SLOTS:

	/// \brief Is called when the user has clicked on the color picker control.
	///
	/// This will open the color selection dialog.
	void activateColorPicker();

protected:

	/// Paints the widget.
	virtual void paintEvent(QPaintEvent* event) override;

	/// The currently selected color.
	Color _color;
};

}	// End of namespace


