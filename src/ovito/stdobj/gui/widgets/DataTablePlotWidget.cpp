////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "DataTablePlotWidget.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_spectrocurve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_barchart.h>
#include <qwt/qwt_plot_legenditem.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_plot_zoomer.h>
#include <qwt/qwt_plot_magnifier.h>
#include <qwt/qwt_plot_panner.h>

namespace Ovito { namespace StdObj {

/******************************************************************************
* Constructor.
******************************************************************************/
DataTablePlotWidget::DataTablePlotWidget(QWidget* parent) : QwtPlot(parent)
{
	setCanvasBackground(Qt::white);

	// Show a grid in the background of the plot.
	QwtPlotGrid* plotGrid = new QwtPlotGrid();
	plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
	plotGrid->attach(this);
	plotGrid->setZ(0);

	// Choose a smaller font size for the axis labels.
    QFont fscl(fontInfo().family(), 8);
    QFont fttl(fontInfo().family(), 8, QFont::Bold);
    for(int axisId = 0; axisId < QwtPlot::axisCnt; axisId++) {
        axisWidget(axisId)->setFont(fscl);
		QwtText text = axisWidget(axisId)->title();
		text.setFont(fttl);
		axisWidget(axisId)->setTitle(text);
	}

	// Left button: zoom region
	// Left button + Shift: panning
	// Right button: zoom out to full size
	// Mouse wheel: zoom in/out

	_zoomer = new QwtPlotZoomer(canvas());
	_zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton);

	_magnifier = new QwtPlotMagnifier(canvas());
	_magnifier->setMouseButton(Qt::NoButton);
	_magnifier->setWheelFactor(1.0 / _magnifier->wheelFactor()); // Flip mouse wheel direction.

	_panner = new QwtPlotPanner(canvas());
	_panner->setMouseButton(Qt::LeftButton, Qt::ShiftModifier);
}

/******************************************************************************
* Sets the data table to be plotted.
******************************************************************************/
void DataTablePlotWidget::setTable(const DataTable* table)
{
	if(table != _table) {
		_table = table;
		updateDataPlot();
	}
}

/******************************************************************************
* Regenerates the plot.
* This function is called whenever a new data table has been loaded into
* widget or if the current table data changes.
******************************************************************************/
void DataTablePlotWidget::updateDataPlot()
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

	// Determine the current plotting mode.
	DataTable::PlotMode plotMode = DataTable::None;
	const PropertyObject* y = table() ? table()->getY() : nullptr;
	const PropertyObject* x = table() ? table()->getX() : nullptr;
	if(y) {
		if(y->size() > (size_t)std::numeric_limits<int>::max())
			qWarning() << "Number of plot data points exceeds limit:" << y->size() << ">" << std::numeric_limits<int>::max();
		else if(x && x->size() != y->size())
			qWarning() << "Detected inconsistent lengths of X and Y data arrays in data table:" << table()->objectTitle();
		else
			plotMode = table()->plotMode();
	}

	// Release plot items if plot mode has changed.
	if(plotMode != DataTable::Line && plotMode != DataTable::Histogram) {
		for(QwtPlotCurve* curve : _curves) delete curve;
		_curves.clear();
	}
	if(plotMode != DataTable::Scatter) {
		for(QwtPlotSpectroCurve* curve : _spectroCurves) delete curve;
		_spectroCurves.clear();
	}
	if(plotMode != DataTable::BarChart) {
		if(_barChart) {
			delete _barChart;
			_barChart = nullptr;
		}
		if(_barChartScaleDraw) {
			setAxisScaleDraw(QwtPlot::xBottom, new QwtScaleDraw());
			_barChartScaleDraw = nullptr;
		}
	}

	// Determine if a legend should be displayed.
	bool showLegend = false;
	if(y && !y->componentNames().empty()) {
		if(plotMode == DataTable::Line || plotMode == DataTable::Histogram)
			showLegend = true;
	}

	// Show/hide plot item for chart legend.
	if(showLegend) {
		if(!_legend) {
			_legend = new QwtPlotLegendItem();
			_legend->setAlignment(Qt::AlignRight | Qt::AlignTop);
			_legend->attach(this);
		}
	}
	else {
		delete _legend;
		_legend = nullptr;
	}

	bool enableMouseInteraction = true;

	// Create plot items.
	if(plotMode == DataTable::Scatter) {
		// Scatter plot:
		size_t colCount = std::min(x ? x->componentCount() : 1, y ? y->componentCount() : 0);
		while(_spectroCurves.size() < colCount) {
			QwtPlotSpectroCurve* curve = new QwtPlotSpectroCurve();
			curve->setPenWidth(3);
			curve->setZ(0);
			curve->attach(this);
			_spectroCurves.push_back(curve);
		}
		while(_spectroCurves.size() > colCount) {
			delete _spectroCurves.back();
			_spectroCurves.pop_back();
		}

		// Set legend titles.
		for(size_t cmpnt = 0; cmpnt < colCount; cmpnt++) {
			if(cmpnt < y->componentNames().size())
				_spectroCurves[cmpnt]->setTitle(y->componentNames()[cmpnt]);
			else
				_spectroCurves[cmpnt]->setTitle(tr("Component %1").arg(cmpnt+1));
		}

		ConstPropertyPtr xvalues = table()->getXValues();
		QVector<QwtPoint3D> coords(y->size());
		for(size_t cmpnt = 0; cmpnt < colCount; cmpnt++) {
			xvalues->forEach(cmpnt, [&coords](size_t i, auto v) { coords[i].rx() = v; });
			y->forEach(cmpnt, [&coords](size_t i, auto v) { coords[i].ry() = v; });
			_spectroCurves[cmpnt]->setSamples(coords);
		}

#if 0
		class ColorMap : public QwtColorMap {
			std::unordered_map<int,QRgb> _map;
		public:
			ColorMap(const std::map<int,Color>& map) {
				for(const auto& e : map) {
					int r = 255 * e.second.r();
					int g = 255 * e.second.g();
					int b = 255 * e.second.b();
					_map.insert(std::make_pair(e.first, qRgb(r,g,b)));
				}
			}
			virtual unsigned char colorIndex(const QwtInterval& interval, double value) const override { return 0; }
			virtual QRgb rgb(const QwtInterval& interval, double value) const override {
				auto iter = _map.find((int)value);
				if(iter != _map.end()) return iter->second;
				else return qRgb(0,0,200);
			}
		};
		_plotCurve->setColorMap(new ColorMap(modApp->colorMap()));
#endif

	}
	else if(plotMode == DataTable::Line || plotMode == DataTable::Histogram) {
		// Line and histogram charts:
		while(_curves.size() < y->componentCount()) {
			QwtPlotCurve* curve = new QwtPlotCurve();
			curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
			curve->setPen(curveColors[_curves.size() % (sizeof(curveColors)/sizeof(curveColors[0]))], 1);
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

		// Set legend titles.
		for(size_t cmpnt = 0; cmpnt < y->componentCount(); cmpnt++) {
			if(cmpnt < y->componentNames().size())
				_curves[cmpnt]->setTitle(y->componentNames()[cmpnt]);
			else
				_curves[cmpnt]->setTitle(tr("Component %1").arg(cmpnt+1));
		}

		QVector<double> xcoords(y->size());
		if(!x || x->size() != xcoords.size() || !x->copyTo(xcoords.begin())) {
			if(table()->intervalStart() < table()->intervalEnd() && y->size() != 0) {
				FloatType binSize = (table()->intervalEnd() - table()->intervalStart()) / y->size();
				double xc = table()->intervalStart() + binSize / 2;
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
			if(!y->copyTo(ycoords.begin(), cmpnt)) {
				std::fill(ycoords.begin(), ycoords.end(), 0.0);
			}
			_curves[cmpnt]->setSamples(xcoords, ycoords);
		}
	}
	else if(plotMode == DataTable::BarChart) {
		// Bar chart:
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
		ConstPropertyAccess<void,true> yarray(y);
		for(int i = 0; i < y->size(); i++) {
			const ElementType* type = y->elementType(i);
			if(!type && x) type = x->elementType(i);
			if(type) {
				ycoords.push_back(yarray.get<double>(i, 0));
				labels.push_back(type->name());
			}
		}
		setAxisMaxMinor(QwtPlot::xBottom, 0);
		setAxisMaxMajor(QwtPlot::xBottom, labels.size());
		_barChart->setSamples(std::move(ycoords));
		_barChartScaleDraw->setLabels(std::move(labels));

		// Extra call to replot() needed here as a workaround for a layout bug in QwtPlot.
		replot();

		enableMouseInteraction = false;
	}
	if(_axisAutoscaleEnabled[QwtPlot::xBottom])
		QwtPlot::setAxisAutoScale(QwtPlot::xBottom);
	if(_axisAutoscaleEnabled[QwtPlot::yLeft])
		QwtPlot::setAxisAutoScale(QwtPlot::yLeft);

	if(plotMode != DataTable::PlotMode::None) {
		setAxisTitle(QwtPlot::xBottom, (!x || !table()->axisLabelX().isEmpty()) ? table()->axisLabelX() : x->name());
		setAxisTitle(QwtPlot::yLeft, (!y || !table()->axisLabelY().isEmpty()) ? table()->axisLabelY() : y->name());
	}

	// Workaround for layout bug in QwtPlot:
	axisWidget(QwtPlot::yLeft)->setBorderDist(1, 1);
	axisWidget(QwtPlot::yLeft)->setBorderDist(0, 0);

	replot();
	
	_zoomer->setEnabled(enableMouseInteraction && _mouseNavigationEnabled);
	_magnifier->setEnabled(enableMouseInteraction && _mouseNavigationEnabled);
	_panner->setEnabled(enableMouseInteraction && _mouseNavigationEnabled);
	_zoomer->setZoomBase(false);
}

}	// End of namespace
}	// End of namespace
