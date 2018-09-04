///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/correlation/CorrelationFunctionModifier.h>
#include <plugins/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <core/oo/CloneHelper.h>
#include "CorrelationFunctionModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_scale_engine.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(CorrelationFunctionModifierEditor);
SET_OVITO_OBJECT_EDITOR(CorrelationFunctionModifier, CorrelationFunctionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CorrelationFunctionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Correlation function"), rolloutParams, "particles.modifiers.correlation_function.html");

	// Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	PropertyReferenceParameterUI* sourceProperty1UI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::sourceProperty1), &ParticlesObject::OOClass());
	layout->addWidget(new QLabel(tr("First property:"), rollout));
	layout->addWidget(sourceProperty1UI->comboBox());

	PropertyReferenceParameterUI* sourceProperty2UI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::sourceProperty2), &ParticlesObject::OOClass());
	layout->addWidget(new QLabel(tr("Second property:"), rollout));
	layout->addWidget(sourceProperty2UI->comboBox());

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// FFT grid spacing parameter.
	FloatParameterUI* fftGridSpacingRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::fftGridSpacing));
	gridlayout->addWidget(fftGridSpacingRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(fftGridSpacingRadiusPUI->createFieldLayout(), 0, 1);

	layout->addLayout(gridlayout);

	BooleanParameterUI* applyWindowUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::applyWindow));
	layout->addWidget(applyWindowUI->checkBox());

#if 0
	gridlayout = new QGridLayout();
	gridlayout->addWidget(new QLabel(tr("Average:"), rollout), 0, 0);
	VariantComboBoxParameterUI* averagingDirectionPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::averagingDirection));
    averagingDirectionPUI->comboBox()->addItem("radial", QVariant::fromValue(CorrelationFunctionModifier::RADIAL));
    averagingDirectionPUI->comboBox()->addItem("cell vector 1", QVariant::fromValue(CorrelationFunctionModifier::CELL_VECTOR_1));
    averagingDirectionPUI->comboBox()->addItem("cell vector 2", QVariant::fromValue(CorrelationFunctionModifier::CELL_VECTOR_2));
    averagingDirectionPUI->comboBox()->addItem("cell vector 3", QVariant::fromValue(CorrelationFunctionModifier::CELL_VECTOR_3));
    gridlayout->addWidget(averagingDirectionPUI->comboBox(), 0, 1);
    layout->addLayout(gridlayout);
#endif

	QGroupBox* realSpaceGroupBox = new QGroupBox(tr("Real-space correlation function"));
	layout->addWidget(realSpaceGroupBox);

	BooleanParameterUI* doComputeNeighCorrelationUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::doComputeNeighCorrelation));

	QGridLayout* realSpaceGridLayout = new QGridLayout();
	realSpaceGridLayout->setContentsMargins(4,4,4,4);
	realSpaceGridLayout->setColumnStretch(1, 1);

	// Neighbor cutoff parameter.
	FloatParameterUI *neighCutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::neighCutoff));
	neighCutoffRadiusPUI->setEnabled(false);
	realSpaceGridLayout->addWidget(neighCutoffRadiusPUI->label(), 1, 0);
	realSpaceGridLayout->addLayout(neighCutoffRadiusPUI->createFieldLayout(), 1, 1);

	// Number of bins parameter.
	IntegerParameterUI* numberOfNeighBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::numberOfNeighBins));
	numberOfNeighBinsPUI->setEnabled(false);
	realSpaceGridLayout->addWidget(numberOfNeighBinsPUI->label(), 2, 0);
	realSpaceGridLayout->addLayout(numberOfNeighBinsPUI->createFieldLayout(), 2, 1);

	connect(doComputeNeighCorrelationUI->checkBox(), &QCheckBox::toggled, neighCutoffRadiusPUI, &FloatParameterUI::setEnabled);
	connect(doComputeNeighCorrelationUI->checkBox(), &QCheckBox::toggled, numberOfNeighBinsPUI, &IntegerParameterUI::setEnabled);

	QGridLayout *normalizeRealSpaceLayout = new QGridLayout();
	normalizeRealSpaceLayout->addWidget(new QLabel(tr("Type of plot:"), rollout), 0, 0);
	VariantComboBoxParameterUI* normalizeRealSpacePUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::normalizeRealSpace));
    normalizeRealSpacePUI->comboBox()->addItem("Value correlation", QVariant::fromValue(CorrelationFunctionModifier::VALUE_CORRELATION));
    normalizeRealSpacePUI->comboBox()->addItem("Difference correlation", QVariant::fromValue(CorrelationFunctionModifier::DIFFERENCE_CORRELATION));
    normalizeRealSpaceLayout->addWidget(normalizeRealSpacePUI->comboBox(), 0, 1);

	BooleanParameterUI* normalizeRealSpaceByRDFUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::normalizeRealSpaceByRDF));
	BooleanParameterUI* normalizeRealSpaceByCovarianceUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::normalizeRealSpaceByCovariance));

	QGridLayout* typeOfRealSpacePlotLayout = new QGridLayout();
	IntegerRadioButtonParameterUI *typeOfRealSpacePlotPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::typeOfRealSpacePlot));
	typeOfRealSpacePlotLayout->addWidget(new QLabel(tr("Display as:")), 0, 0);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(0, tr("lin-lin")), 0, 1);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(1, tr("log-lin")), 0, 2);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(3, tr("log-log")), 0, 3);

	_realSpacePlot = new DataSeriesPlotWidget();
	_realSpacePlot->setMinimumHeight(200);
	_realSpacePlot->setMaximumHeight(200);
	_neighCurve = new QwtPlotCurve();
	_neighCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
	_neighCurve->setPen(QPen(Qt::red, 1));
	_neighCurve->setZ(1);
	_neighCurve->attach(_realSpacePlot);
	_neighCurve->hide();

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::fixRealSpaceXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::realSpaceXAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::realSpaceXAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}
	// y-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::fixRealSpaceYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::realSpaceYAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::realSpaceYAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}

	QVBoxLayout* realSpaceLayout = new QVBoxLayout(realSpaceGroupBox);
	realSpaceLayout->addWidget(doComputeNeighCorrelationUI->checkBox());
	realSpaceLayout->addLayout(realSpaceGridLayout);
	realSpaceLayout->addLayout(normalizeRealSpaceLayout);
	realSpaceLayout->addWidget(normalizeRealSpaceByRDFUI->checkBox());
	realSpaceLayout->addWidget(normalizeRealSpaceByCovarianceUI->checkBox());
	realSpaceLayout->addLayout(typeOfRealSpacePlotLayout);
	realSpaceLayout->addWidget(_realSpacePlot);
	realSpaceLayout->addWidget(axesBox);

	QGroupBox* reciprocalSpaceGroupBox = new QGroupBox(tr("Reciprocal-space correlation function"));
	layout->addWidget(reciprocalSpaceGroupBox);

	BooleanParameterUI* normalizeReciprocalSpaceUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::normalizeReciprocalSpace));

	QGridLayout* typeOfReciprocalSpacePlotLayout = new QGridLayout();
	IntegerRadioButtonParameterUI *typeOfReciprocalSpacePlotPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::typeOfReciprocalSpacePlot));
	typeOfReciprocalSpacePlotLayout->addWidget(new QLabel(tr("Display as:")), 0, 0);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(0, tr("lin-lin")), 0, 1);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(1, tr("log-lin")), 0, 2);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(3, tr("log-log")), 0, 3);

	_reciprocalSpacePlot = new DataSeriesPlotWidget();
	_reciprocalSpacePlot->setMinimumHeight(200);
	_reciprocalSpacePlot->setMaximumHeight(200);

	// Axes.
	axesBox = new QGroupBox(tr("Plot axes"), rollout);
	axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::fixReciprocalSpaceXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::reciprocalSpaceXAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::reciprocalSpaceXAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}
	// y-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::fixReciprocalSpaceYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::reciprocalSpaceYAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::reciprocalSpaceYAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}

	QVBoxLayout* reciprocalSpaceLayout = new QVBoxLayout(reciprocalSpaceGroupBox);
	reciprocalSpaceLayout->addWidget(normalizeReciprocalSpaceUI->checkBox());
	reciprocalSpaceLayout->addLayout(typeOfReciprocalSpacePlotLayout);
	reciprocalSpaceLayout->addWidget(_reciprocalSpacePlot);
	reciprocalSpaceLayout->addWidget(axesBox);

	connect(this, &CorrelationFunctionModifierEditor::contentsReplaced, this, &CorrelationFunctionModifierEditor::plotAllData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CorrelationFunctionModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == modifierApplication() && event.type() == ReferenceEvent::PipelineCacheUpdated) {
		plotAllDataLater(this);
	}
	else if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		plotAllDataLater(this);
	}
	return ModifierPropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots one of the correlation function computed by the modifier.
******************************************************************************/
std::pair<FloatType,FloatType> CorrelationFunctionModifierEditor::plotData(
												DataSeriesObject* series,
												DataSeriesPlotWidget* plotWidget,
												FloatType offset, 
												FloatType fac, 
												const PropertyPtr& normalization)
{
	// Duplicate the data series, then modify the stored values.
	UndoSuspender noUndo(series);
	CloneHelper cloneHelper;
	OORef<DataSeriesObject> clonedSeries = cloneHelper.cloneObject(series, true);

#if 0
	// Normalize function values.
	if(normalization) {
		OVITO_ASSERT(normalization->size() == clonedSeries->y()->size());
		auto pf = normalization->constDataFloat();
		for(FloatType& v : clonedSeries->modifiableY()->floatRange()) {
			FloatType factor = *pf++;
			v = (factor > FloatType(1e-12)) ? (v / factor) : FloatType(0);
		}
	}

	// Scale and shift function values.
	if(fac != 1 || offset != 0) {
		for(FloatType& v : clonedSeries->modifiableY()->floatRange())
			v = fac * (v - offset);
	}

	// Determine value range.
	auto minmax = std::minmax_element(clonedSeries->y()->constDataFloat(), clonedSeries->y()->constDataFloat() + clonedSeries->y()->size());

	// Hand data series over to plot widget.
	plotWidget->setSeries(std::move(clonedSeries));

	return { *minmax.first, *minmax.second };
#endif
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CorrelationFunctionModifierEditor::plotAllData()
{
	CorrelationFunctionModifier* modifier = static_object_cast<CorrelationFunctionModifier>(editObject());

	// Set type of plot.
	if(modifier && modifier->typeOfRealSpacePlot() & 1)
		_realSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
	else
		_realSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
	if(modifier && modifier->typeOfRealSpacePlot() & 2)
		_realSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
	else
		_realSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());

	if(modifier && modifier->typeOfReciprocalSpacePlot() & 1)
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
	else
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
	if(modifier && modifier->typeOfReciprocalSpacePlot() & 2)
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
	else
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());

	// Set axis ranges.
	if(modifier && modifier->fixRealSpaceXAxisRange())
		_realSpacePlot->setAxisScale(QwtPlot::xBottom, modifier->realSpaceXAxisRangeStart(), modifier->realSpaceXAxisRangeEnd());
	else
		_realSpacePlot->setAxisAutoScale(QwtPlot::xBottom);
	if(modifier && modifier->fixRealSpaceYAxisRange())
		_realSpacePlot->setAxisScale(QwtPlot::yLeft, modifier->realSpaceYAxisRangeStart(), modifier->realSpaceYAxisRangeEnd());
	else
		_realSpacePlot->setAxisAutoScale(QwtPlot::yLeft);
	if(modifier && modifier->fixReciprocalSpaceXAxisRange())
		_reciprocalSpacePlot->setAxisScale(QwtPlot::xBottom, modifier->reciprocalSpaceXAxisRangeStart(), modifier->reciprocalSpaceXAxisRangeEnd());
	else
		_reciprocalSpacePlot->setAxisAutoScale(QwtPlot::xBottom);
	if(modifier && modifier->fixReciprocalSpaceYAxisRange())
		_reciprocalSpacePlot->setAxisScale(QwtPlot::yLeft, modifier->reciprocalSpaceYAxisRangeStart(), modifier->reciprocalSpaceYAxisRangeEnd());
	else
		_reciprocalSpacePlot->setAxisAutoScale(QwtPlot::yLeft);

#if 0
	// Obtain the pipeline data produced by the modifier.
	const PipelineFlowState& state = getModifierOutput();

	// Retreive computed values from pipeline.
	const QVariant& mean1 = state.getAttributeValue(QStringLiteral("CorrelationFunction.mean1"), modifierApplication());
	const QVariant& mean2 = state.getAttributeValue(QStringLiteral("CorrelationFunction.mean2"), modifierApplication());
	const QVariant& variance1 = state.getAttributeValue(QStringLiteral("CorrelationFunction.variance1"), modifierApplication());
	const QVariant& variance2 = state.getAttributeValue(QStringLiteral("CorrelationFunction.variance2"), modifierApplication());
	const QVariant& covariance = state.getAttributeValue(QStringLiteral("CorrelationFunction.covariance"), modifierApplication());

	// Determine scaling factor and offset.
	FloatType offset = 0.0;
	FloatType uniformFactor = 1;
	if(modifier && variance1.isValid() && variance2.isValid() && covariance.isValid()) {
		if(modifier->normalizeRealSpace() == CorrelationFunctionModifier::DIFFERENCE_CORRELATION) {
			offset = 0.5 * (variance1.toDouble() + variance2.toDouble());
			uniformFactor = -1;
		}
		if(modifier->normalizeRealSpaceByCovariance() && covariance.toDouble() != 0) {
			uniformFactor /= covariance.toDouble();
		}
	}

	// Display direct neighbor correlation function. 
	DataSeriesObject* neighCorrelation = state.findObject<DataSeriesObject>(QStringLiteral("correlation/neighbor"), modifierApplication()); 
	DataSeriesObject* neighRDF = state.findObject<DataSeriesObject>(QStringLiteral("correlation/neighbor/rdf"), modifierApplication()); 
	if(modifier && modifierApplication() && modifier->doComputeNeighCorrelation() && modApp && neighCorrelation && neighRDF) {
		const auto& yData = neighCorrelation->getYStorage(state);
		const auto& rdfData = neighRDF->getYStorage(state);
		size_t numberOfDataPoints = yData->size();
		QVector<QPointF> plotData(numberOfDataPoints);
		bool normByRDF = modifier->normalizeRealSpaceByRDF();
		for(size_t i = 0; i < numberOfDataPoints; i++) {
			FloatType xValue = neighCorrelation()->getXValue(i);
			FloatType yValue = yData->getFloat(i);
			if(normByRDF)
				yValue = rdfData->getFloat(i) > 1e-12 ? (yValue / rdfData->getFloat(i)) : 0.0;
			yValue = uniformFactor * (yValue - offset);
			plotData[i].rx() = xValue;
			plotData[i].ry() = yValue;
		}
		_neighCurve->setSamples(plotData);
		_neighCurve->show();
	}
	else {
		_neighCurve->hide();
	}

	// Plot real-space correlation function.
	if(modApp && modifier && modApp->realSpaceCorrelation()) {
		auto realSpaceYRange = plotData(modApp->realSpaceCorrelation(), _realSpacePlot, offset, uniformFactor, 
			modifier->normalizeRealSpaceByRDF() ? modApp->realSpaceRDF()->y() : nullptr);

		UndoSuspender noUndo(modifier);
		if(!modifier->fixRealSpaceXAxisRange()) {
			modifier->setRealSpaceXAxisRangeStart(modApp->realSpaceCorrelation()->intervalStart());
			modifier->setRealSpaceXAxisRangeEnd(modApp->realSpaceCorrelation()->intervalEnd());
		}
		if(!modifier->fixRealSpaceYAxisRange()) {
			modifier->setRealSpaceYAxisRangeStart(realSpaceYRange.first);
			modifier->setRealSpaceYAxisRangeEnd(realSpaceYRange.second);
		}
	}
	else {
		_realSpacePlot->setSeries(nullptr);
	}

	// Plot reciprocal-space correlation function.
	if(modifier && modApp && modApp->reciprocalSpaceCorrelation()) {
		FloatType rfac = 1;
		if(modifier->normalizeReciprocalSpace() && modApp->covariance() != 0)
			rfac = 1.0 / modApp->covariance();
		auto reciprocalSpaceYRange = plotData(modApp->reciprocalSpaceCorrelation(), _reciprocalSpacePlot, 0.0, rfac, nullptr);

		UndoSuspender noUndo(modifier);
		if(!modifier->fixReciprocalSpaceXAxisRange()) {
			modifier->setReciprocalSpaceXAxisRangeStart(modApp->reciprocalSpaceCorrelation()->intervalStart());
			modifier->setReciprocalSpaceXAxisRangeEnd(modApp->reciprocalSpaceCorrelation()->intervalEnd());
		}
		if(!modifier->fixReciprocalSpaceYAxisRange()) {
			modifier->setReciprocalSpaceYAxisRangeStart(reciprocalSpaceYRange.first);
			modifier->setReciprocalSpaceYAxisRangeEnd(reciprocalSpaceYRange.second);
		}
	}
	else {
		_reciprocalSpacePlot->setSeries(nullptr);
	}
#endif
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
