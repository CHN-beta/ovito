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


#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/stdobj/gui/widgets/DataTablePlotWidget.h>

class QwtPlotZoneItem;

namespace Ovito::StdMod {

/**
 * A properties editor for the ScatterPlotModifier class.
 */
class ScatterPlotModifierEditor : public PropertiesEditor
{
	OVITO_CLASS(ScatterPlotModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE ScatterPlotModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the scatter plot.
	void plotScatterPlot();

private:

	/// The graph widget to display the plot.
	DataTablePlotWidget* _plotWidget;

	/// Marks the range of selected points in the X direction.
	QwtPlotZoneItem* _selectionRangeIndicatorX;

	/// Marks the range of selected points in the Y direction.
	QwtPlotZoneItem* _selectionRangeIndicatorY;
};

}	// End of namespace
