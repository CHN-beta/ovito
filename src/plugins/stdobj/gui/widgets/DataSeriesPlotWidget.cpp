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

#include <plugins/stdobj/gui/StdObjGui.h>
#include "DataSeriesPlotWidget.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_barchart.h>
#include <qwt/qwt_plot_legenditem.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_scale_widget.h>

namespace Ovito { namespace StdObj {

/******************************************************************************
* Constructor.
******************************************************************************/
DataSeriesPlotWidget::DataSeriesPlotWidget(QWidget* parent) : QwtPlot(parent)
{
	setCanvasBackground(Qt::white);

	// Show a grid in the background of the plot.
	QwtPlotGrid* plotGrid = new QwtPlotGrid();
	plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
	plotGrid->attach(this);
	plotGrid->setZ(0);
}

/******************************************************************************
* Sets the data series object to be plotted.
******************************************************************************/
void DataSeriesPlotWidget::setSeries(const DataSeriesObject* series) 
{ 
	if(series != _series) {
		_series = series;
		updateDataPlot();
	}
}

/******************************************************************************
* Regenerates the plot. 
* This function is called whenever a new data series has been loaded into 
* widget or if the current series data changes.
******************************************************************************/
void DataSeriesPlotWidget::updateDataPlot()
{
	static const Qt::GlobalColor curveColors[] = {
		Qt::black, Qt::red, Qt::blue, Qt::green,
		Qt::cyan, Qt::magenta, Qt::gray, Qt::darkRed, 
		Qt::darkGreen, Qt::darkBlue, Qt::darkCyan, Qt::darkMagenta,
		Qt::darkYellow, Qt::darkGray
	};
	
	setAxisTitle(QwtPlot::xBottom, QString{});
	setAxisTitle(QwtPlot::yLeft, QString{});
	setAxisMaxMinor(QwtPlot::xBottom, 5);
	setAxisMaxMajor(QwtPlot::xBottom, 8);
	plotLayout()->setCanvasMargin(4);

	const PropertyObject* y = series() ? series()->getY() : nullptr;

	if(y) {
		const PropertyObject* x = series()->getX();
		if(x || y->elementTypes().empty() || y->componentCount() != 1) {
			// Curve plot(s):
			if(_barChart) {
				delete _barChart;
				_barChart = nullptr;
			}
			if(_barChartScaleDraw) {
				setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw());
				_barChartScaleDraw = nullptr;
			}
			while(_curves.size() < y->componentCount()) {
				QwtPlotCurve* curve = new QwtPlotCurve();
				curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
				curve->setPen(QPen(curveColors[_curves.size() % (sizeof(curveColors)/sizeof(curveColors[0]))], 1));
				curve->setZ(0);
				curve->attach(this);
				_curves.push_back(curve);
			}
			while(_curves.size() > y->componentCount()) {
				delete _curves.back();
				_curves.pop_back();
			}
			if(_curves.size() == 1 && y->componentNames().empty()) {
				_curves[0]->setBrush(QColor(255, 160, 100));
			}
			else {
				for(QwtPlotCurve* curve : _curves)
					curve->setBrush({});
			}
			if(y->componentNames().empty()) {
				if(_legend) {
					delete _legend;
					_legend = nullptr;
				}
			}
			else {
				if(!_legend) {
					_legend = new QwtPlotLegendItem();
					_legend->setAlignment(Qt::AlignRight | Qt::AlignTop);
					_legend->attach(this);
				}
			}

			QVector<double> xcoords(y->size());
			if(!x || x->size() != xcoords.size() || !x->storage()->copyTo(xcoords.begin())) {
				if(series()->intervalStart() < series()->intervalEnd() && y->size() != 0) {
					FloatType binSize = (series()->intervalEnd() - series()->intervalStart()) / y->size();
					double xc = series()->intervalStart() + binSize / 2;
					for(auto& v : xcoords) {
						v = xc;
						xc += binSize;
					}
				}
				else {
					std::iota(xcoords.begin(), xcoords.end(), 0);
				}
			}

			QVector<double> ycoords(y->size());
			for(size_t cmpnt = 0; cmpnt < y->componentCount(); cmpnt++) {
				if(!y->storage()->copyTo(ycoords.begin(), cmpnt)) {
					std::fill(ycoords.begin(), ycoords.end(), 0.0);
				}
				_curves[cmpnt]->setSamples(xcoords, ycoords);
				if(cmpnt < y->componentNames().size())
					_curves[cmpnt]->setTitle(y->componentNames()[cmpnt]);
			}
		}
		else {
			// Bar chart:
			for(QwtPlotCurve* curve : _curves) delete curve;
			_curves.clear();
			if(!_barChart) {
				_barChart = new QwtPlotBarChart();
				_barChart->setRenderHint(QwtPlotItem::RenderAntialiased, true);
				_barChart->setZ(0);
				_barChart->attach(this);
			}
			if(!_barChartScaleDraw) {
				_barChartScaleDraw = new BarChartScaleDraw();
				_barChartScaleDraw->enableComponent(QwtScaleDraw::Backbone, false);
				_barChartScaleDraw->enableComponent(QwtScaleDraw::Ticks, false);
				setAxisScaleDraw(QwtPlot::xBottom, _barChartScaleDraw);
			}
			QVector<double> ycoords;
			QStringList labels;
			for(int i = 0; i < y->size(); i++) {
				if(ElementType* type = y->elementType(i)) {
					if(y->dataType() == PropertyStorage::Int)
						ycoords.push_back(y->getInt(i));
					else if(y->dataType() == PropertyStorage::Int64)
						ycoords.push_back(y->getInt64(i));
					else if(y->dataType() == PropertyStorage::Float)
						ycoords.push_back(y->getFloat(i));
					else 
						continue;
					labels.push_back(type->name());
				}
			}
			setAxisMaxMinor(QwtPlot::xBottom, 0);
			setAxisMaxMajor(QwtPlot::xBottom, labels.size());
			_barChart->setSamples(std::move(ycoords));
			_barChartScaleDraw->setLabels(std::move(labels));

			// Extra call to replot() needed here as a workaround for a layout bug in QwtPlot.
			replot();
		}

		setAxisTitle(QwtPlot::xBottom, (!x || !series()->axisLabelX().isEmpty()) ? series()->axisLabelX() : x->name());
		setAxisTitle(QwtPlot::yLeft, (!y || !series()->axisLabelY().isEmpty()) ? series()->axisLabelY() : y->name());

		// Workaround for layout bug in QwtPlot:
		axisWidget(QwtPlot::yLeft)->setBorderDist(1, 1);
		axisWidget(QwtPlot::yLeft)->setBorderDist(0, 0);
	}
	else {
		for(QwtPlotCurve* curve : _curves) delete curve;
		_curves.clear();
		delete _barChart;
		_barChart = nullptr;
		if(_barChartScaleDraw) {
			setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw());
			_barChartScaleDraw = nullptr;
		}
		delete _legend;
		_legend = nullptr;
	}

	replot();
}

}	// End of namespace
}	// End of namespace
