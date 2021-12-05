////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
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
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>

class QwtPlotMarker;

namespace Ovito::Grid {

/**
 * \brief A properties editor for the CreateIsosurfaceModifier class.
 */
class CreateIsosurfaceModifierEditor : public PropertiesEditor
{
	OVITO_CLASS(CreateIsosurfaceModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE CreateIsosurfaceModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the value histogram computed by the modifier.
	void plotHistogram();

	/// Is called when the user starts or stops picking a location in the plot widget.
	void onPickerActivated(bool on);

	/// Is called when the user picks a location in the plot widget.
	void onPickerPoint(const QPointF& pt);

private:

	/// The graph widget to display the histogram.
	StdObj::DataTablePlotWidget* _plotWidget;

	/// The plot item for indicating the current iso level value.
	QwtPlotMarker* _isoLevelIndicator;

	/// Indicates that the user is currently interacting with the plot widget.
	bool _interactionInProgress = false;
};

}	// End of namespace
