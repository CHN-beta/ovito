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

#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "HistogramModifier.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(HistogramModifier);
DEFINE_PROPERTY_FIELD(HistogramModifier, numberOfBins);
DEFINE_PROPERTY_FIELD(HistogramModifier, selectInRange);
DEFINE_PROPERTY_FIELD(HistogramModifier, selectionRangeStart);
DEFINE_PROPERTY_FIELD(HistogramModifier, selectionRangeEnd);
DEFINE_PROPERTY_FIELD(HistogramModifier, fixXAxisRange);
DEFINE_PROPERTY_FIELD(HistogramModifier, xAxisRangeStart);
DEFINE_PROPERTY_FIELD(HistogramModifier, xAxisRangeEnd);
DEFINE_PROPERTY_FIELD(HistogramModifier, fixYAxisRange);
DEFINE_PROPERTY_FIELD(HistogramModifier, yAxisRangeStart);
DEFINE_PROPERTY_FIELD(HistogramModifier, yAxisRangeEnd);
DEFINE_PROPERTY_FIELD(HistogramModifier, sourceProperty);
DEFINE_PROPERTY_FIELD(HistogramModifier, onlySelectedElements);
SET_PROPERTY_FIELD_LABEL(HistogramModifier, numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, selectInRange, "Select value range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, selectionRangeStart, "Selection range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, selectionRangeEnd, "Selection range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, fixXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, xAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, xAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, fixYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, yAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, yAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, onlySelectedElements, "Use only selected elements");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(HistogramModifier, numberOfBins, IntegerParameterUnit, 1, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
HistogramModifier::HistogramModifier(ObjectCreationParams params) : GenericPropertyModifier(params),
	_numberOfBins(200),
	_selectInRange(false),
	_selectionRangeStart(0),
	_selectionRangeEnd(1),
	_fixXAxisRange(false),
	_xAxisRangeStart(0),
	_xAxisRangeEnd(0),
	_fixYAxisRange(false),
	_yAxisRangeStart(0),
	_yAxisRangeEnd(0),
	_onlySelectedElements(false)
{
	// Operate on particle properties by default.
	setDefaultSubject(QStringLiteral("Particles"), QStringLiteral("ParticlesObject"));
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void HistogramModifier::initializeModifier(const ModifierInitializationRequest& request)
{
	GenericPropertyModifier::initializeModifier(request);

	// Use the first available property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull() && subject() && ExecutionContext::isInteractive()) {
		const PipelineFlowState& input = request.modApp()->evaluateInputSynchronous(request);
		if(const PropertyContainer* container = input.getLeafObject(subject())) {
			PropertyReference bestProperty;
			for(const PropertyObject* property : container->properties()) {
				bestProperty = PropertyReference(subject().dataClass(), property, (property->componentCount() > 1) ? 0 : -1);
			}
			if(!bestProperty.isNull()) {
				setSourceProperty(bestProperty);
			}
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void HistogramModifier::propertyChanged(const PropertyFieldDescriptor* field)
{
	// Whenever the selected property class of this modifier changes, update the source property reference accordingly.
	if(field == PROPERTY_FIELD(GenericPropertyModifier::subject) && !isBeingLoaded() && !isAboutToBeDeleted() && !dataset()->undoStack().isUndoingOrRedoing()) {
		setSourceProperty(sourceProperty().convertToContainerClass(subject().dataClass()));
	}
	GenericPropertyModifier::propertyChanged(field);
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void HistogramModifier::evaluateSynchronous(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	if(!subject())
		throwException(tr("No data element type set."));
	if(sourceProperty().isNull())
		throwException(tr("No input property selected."));

	// Check if the source property is the right kind of property.
	if(sourceProperty().containerClass() != subject().dataClass())
		throwException(tr("Modifier was set to operate on '%1', but the selected input is a '%2' property.")
			.arg(subject().dataClass()->pythonName()).arg(sourceProperty().containerClass()->propertyClassDisplayName()));

	// Look up the property container object.
	const PropertyContainer* container = state.expectLeafObject(subject());
	container->verifyIntegrity();

	// Get the input property.
	const PropertyObject* property = sourceProperty().findInContainer(container);
	if(!property)
		throwException(tr("The selected input property '%1' is not present.").arg(sourceProperty().name()));

	size_t vecComponent = std::max(0, sourceProperty().vectorComponent());
	if(vecComponent >= property->componentCount())
		throwException(tr("The selected vector component is out of range. The property '%1' has only %2 components per element.").arg(property->name()).arg(property->componentCount()));
	size_t vecComponentCount = property->componentCount();

	// Get the input selection if filtering was enabled by the user.
	ConstPropertyAccess<int> inputSelection;
	if(onlySelectedElements() && container->getOOMetaClass().isValidStandardPropertyId(PropertyObject::GenericSelectionProperty)) {
		inputSelection = container->expectProperty(PropertyObject::GenericSelectionProperty);
	}

	// Create storage for output selection.
	PropertyAccess<int> outputSelection;
	if(selectInRange() && container->getOOMetaClass().isValidStandardPropertyId(PropertyObject::GenericSelectionProperty)) {
		// First make sure we can safely modify the property container.
		PropertyContainer* mutableContainer = state.expectMutableLeafObject(subject());
		// Add the selection property to the output container.
		outputSelection = mutableContainer->createProperty(PropertyObject::GenericSelectionProperty);
	}

	// Create selection property for output.
	FloatType selectionRangeStart = this->selectionRangeStart();
	FloatType selectionRangeEnd = this->selectionRangeEnd();
	if(selectionRangeStart > selectionRangeEnd) std::swap(selectionRangeStart, selectionRangeEnd);
	size_t numSelected = 0;

	FloatType intervalStart = xAxisRangeStart();
	FloatType intervalEnd = xAxisRangeEnd();

	// Allocate output data array.
	PropertyAccessAndRef<qlonglong> histogram = DataTable::OOClass().createUserProperty(dataset(), std::max(1, numberOfBins()), PropertyObject::Int64, 1, tr("Count"), DataBuffer::InitializeMemory);
	qlonglong* histogramData = histogram.begin();
	int histogramSizeMin1 = histogram.size() - 1;

	if(property->size() > 0) {
		if(property->dataType() == PropertyObject::Float) {
			ConstPropertyAccess<FloatType,true> array(property);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<FloatType>::max();
				intervalEnd = std::numeric_limits<FloatType>::lowest();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(FloatType v : array.componentRange(vecComponent)) {
					if(sel && !*sel++) continue;
					if(v < intervalStart) intervalStart = v;
					if(v > intervalEnd) intervalEnd = v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / histogram.size();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(FloatType v : array.componentRange(vecComponent)) {
					if(sel && !*sel++) continue;
					if(v < intervalStart || v > intervalEnd) continue;
					int binIndex = (v - intervalStart) / binSize;
					histogramData[std::max(0, std::min(binIndex, histogramSizeMin1))]++;
				}
			}
			else {
				if(!inputSelection)
					histogramData[0] = property->size();
				else
					histogramData[0] = property->size() - boost::count(inputSelection, 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection.size() == property->size());
				int* s = outputSelection.begin();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(FloatType v : array.componentRange(vecComponent)) {
					if((!sel || *sel++) && v >= selectionRangeStart && v <= selectionRangeEnd) {
						*s++ = 1;
						numSelected++;
					}
					else *s++ = 0;
				}
			}
		}
		else if(property->dataType() == PropertyObject::Int) {
			ConstPropertyAccess<int,true> array(property);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<FloatType>::max();
				intervalEnd = std::numeric_limits<FloatType>::lowest();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(int v : array.componentRange(vecComponent)) {
					if(sel && !*sel++) continue;
					if(v < intervalStart) intervalStart = v;
					if(v > intervalEnd) intervalEnd = v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / histogram.size();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(int v : array.componentRange(vecComponent)) {
					if(sel && !*sel++) continue;
					if(v < intervalStart || v > intervalEnd) continue;
					int binIndex = ((FloatType)v - intervalStart) / binSize;
					histogramData[std::max(0, std::min(binIndex, histogramSizeMin1))]++;
				}
			}
			else {
				if(!inputSelection)
					histogramData[0] = property->size();
				else
					histogramData[0] = property->size() - boost::count(inputSelection, 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection.size() == property->size());
				int* s = outputSelection.begin();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(int v : array.componentRange(vecComponent)) {
					if((!sel || *sel++) && v >= selectionRangeStart && v <= selectionRangeEnd) {
						*s++ = 1;
						numSelected++;
					}
					else *s++ = 0;
				}
			}
		}
		else if(property->dataType() == PropertyObject::Int64) {
			ConstPropertyAccess<qlonglong,true> array(property);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<FloatType>::max();
				intervalEnd = std::numeric_limits<FloatType>::lowest();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(qlonglong v : array.componentRange(vecComponent)) {
					if(sel && !*sel++) continue;
					if(v < intervalStart) intervalStart = v;
					if(v > intervalEnd) intervalEnd = v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / histogram.size();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(qlonglong v : array.componentRange(vecComponent)) {
					if(sel && !*sel++) continue;
					if(v < intervalStart || v > intervalEnd) continue;
					int binIndex = ((FloatType)v - intervalStart) / binSize;
					histogramData[std::max(0, std::min(binIndex, histogramSizeMin1))]++;
				}
			}
			else {
				if(!inputSelection)
					histogramData[0] = property->size();
				else
					histogramData[0] = property->size() - boost::count(inputSelection, 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection.size() == property->size());
				int* s = outputSelection.begin();
				const int* sel = inputSelection ? inputSelection.cbegin() : nullptr;
				for(qlonglong v : array.componentRange(vecComponent)) {
					if((!sel || *sel++) && v >= selectionRangeStart && v <= selectionRangeEnd) {
						*s++ = 1;
						numSelected++;
					}
					else *s++ = 0;
				}
			}
		}
		else {
			throwException(tr("The property '%1' has a data type that is not supported by the histogram modifier.").arg(property->name()));
		}
	}
	else {
		intervalStart = intervalEnd = 0;
	}

	// Output a data table with the histogram data.
	DataTable* table = state.createObject<DataTable>(
		QStringLiteral("histogram[%1]").arg(sourceProperty().nameWithComponent()), 
		request.modApp(), DataTable::Histogram, sourceProperty().nameWithComponent(), histogram.take());
	table->setAxisLabelX(sourceProperty().nameWithComponent());
	table->setIntervalStart(intervalStart);
	table->setIntervalEnd(intervalEnd);

	QString statusMessage;
	if(outputSelection) {
		statusMessage = tr("%1 %2 selected (%3%)")
				.arg(numSelected)
				.arg(container->getOOMetaClass().elementDescriptionName())
				.arg((FloatType)numSelected * 100 / std::max((size_t)1,outputSelection.size()), 0, 'f', 1);
	}
	state.setStatus(PipelineStatus(PipelineStatus::Success, std::move(statusMessage)));
}

}	// End of namespace
