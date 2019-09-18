///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include "DataSeriesPlotExporter.h"

#include <qwt/qwt_plot_renderer.h>

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(DataSeriesPlotExporter);
DEFINE_PROPERTY_FIELD(DataSeriesPlotExporter, plotWidth);
DEFINE_PROPERTY_FIELD(DataSeriesPlotExporter, plotHeight);
DEFINE_PROPERTY_FIELD(DataSeriesPlotExporter, plotDPI);
SET_PROPERTY_FIELD_LABEL(DataSeriesPlotExporter, plotWidth, "Width (mm)");
SET_PROPERTY_FIELD_LABEL(DataSeriesPlotExporter, plotHeight, "Height (mm)");
SET_PROPERTY_FIELD_LABEL(DataSeriesPlotExporter, plotDPI, "Resolution (DPI)");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DataSeriesPlotExporter, plotWidth, FloatParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DataSeriesPlotExporter, plotHeight, FloatParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DataSeriesPlotExporter, plotDPI, IntegerParameterUnit, 1);

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
DataSeriesPlotExporter::DataSeriesPlotExporter(DataSet* dataset) : FileExporter(dataset),
	_plotWidth(150),
	_plotHeight(100),
	_plotDPI(200)
{
}

/******************************************************************************
 * This is called once for every output file to be written.
 *****************************************************************************/
bool DataSeriesPlotExporter::openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	_outputFile.setFileName(filePath);

	return true;
}

/******************************************************************************
 * This is called once for every output file written .
 *****************************************************************************/
void DataSeriesPlotExporter::closeOutputFile(bool exportCompleted)
{
	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool DataSeriesPlotExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Evaluate pipeline.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation);
	if(operation.isCanceled())
		return false;

	// Look up the DataSeries to be exported in the pipeline state.
	DataObjectReference objectRef(&DataSeriesObject::OOClass(), dataObjectToExport().dataPath());
	const DataSeriesObject* series = static_object_cast<DataSeriesObject>(state.getLeafObject(objectRef));
	if(!series) {
		throwException(tr("The pipeline output does not contain the data series to be exported (animation frame: %1; object key: %2). Available data series keys: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, DataSeriesObject::OOClass())));
	}

	operation.setProgressText(tr("Writing file %1").arg(filePath));

	DataSeriesPlotWidget plotWidget;
	plotWidget.setSeries(series);
	plotWidget.axisScaleDraw(QwtPlot::yLeft)->setPenWidth(1);
	plotWidget.axisScaleDraw(QwtPlot::xBottom)->setPenWidth(1);
	QwtPlotRenderer plotRenderer;
	plotRenderer.setDiscardFlag(QwtPlotRenderer::DiscardFlag::DiscardBackground);
	plotRenderer.setDiscardFlag(QwtPlotRenderer::DiscardFlag::DiscardCanvasBackground);
	plotRenderer.setDiscardFlag(QwtPlotRenderer::DiscardFlag::DiscardCanvasFrame);
	plotRenderer.renderDocument(&plotWidget, outputFile().fileName(), QSizeF(plotWidth(), plotHeight()), plotDPI());

	return !operation.isCanceled();
}

}	// End of namespace
}	// End of namespace