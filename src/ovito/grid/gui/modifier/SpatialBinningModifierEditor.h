///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/gui/GUI.h>
#include <ovito/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <ovito/gui/properties/ModifierPropertiesEditor.h>
#include <ovito/gui/properties/BooleanParameterUI.h>
#include <ovito/gui/properties/IntegerParameterUI.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

class QwtPlotSpectrogram;
class QwtMatrixRasterData;
class QwtPlotTextLabel;

namespace Ovito { namespace Grid { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito::StdObj;

/**
 * A properties editor for the SpatialBinningModifier class.
 */
class SpatialBinningModifierEditor : public ModifierPropertiesEditor
{
	OVITO_CLASS(SpatialBinningModifierEditor)
	Q_OBJECT

public:

	/// Default constructor.
	Q_INVOKABLE SpatialBinningModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Plots the data computed by the modifier.
	void plotData();

    /// Enable/disable the editor for number of y-bins and the first derivative button.
    void updateWidgets();

private:

    /// Widget controlling computation of the first derivative.
    BooleanParameterUI* _firstDerivativePUI;

    /// Widget controlling the number of y-bins.
    IntegerParameterUI* _numBinsYPUI;

    /// Widget controlling the number of z-bins.
    IntegerParameterUI* _numBinsZPUI;

	/// The graph widget to display the 1d data.
	DataSeriesPlotWidget* _plotWidget1d;

	/// The graph widget to display the 2d data.
	QwtPlot* _plotWidget2d;

	/// The plot item for the 2D color plot.
	QwtPlotSpectrogram* _plotRaster;

	/// The data storage for the 2D color plot.
	QwtMatrixRasterData* _rasterData;

	/// Text label indicating that no plot is available, because a 3d grid has been computed.
	QwtPlotTextLabel* _mode3dLabel;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<SpatialBinningModifierEditor, &SpatialBinningModifierEditor::plotData> plotLater;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace