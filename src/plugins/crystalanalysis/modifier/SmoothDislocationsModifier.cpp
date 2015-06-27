///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/data/DislocationNetwork.h>
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/BooleanGroupBoxParameterUI.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "SmoothDislocationsModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, SmoothDislocationsModifier, Modifier);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, SmoothDislocationsModifierEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(SmoothDislocationsModifier, SmoothDislocationsModifierEditor);
DEFINE_FLAGS_PROPERTY_FIELD(SmoothDislocationsModifier, _smoothingEnabled, "SmoothingEnabled", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(SmoothDislocationsModifier, _smoothingLevel, "SmoothingLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(SmoothDislocationsModifier, _coarseningEnabled, "CoarseningEnabled", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(SmoothDislocationsModifier, _linePointInterval, "LinePointInterval", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(SmoothDislocationsModifier, _smoothingEnabled, "Enable smoothing");
SET_PROPERTY_FIELD_LABEL(SmoothDislocationsModifier, _smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(SmoothDislocationsModifier, _coarseningEnabled, "Enable coarsening");
SET_PROPERTY_FIELD_LABEL(SmoothDislocationsModifier, _linePointInterval, "Point separation");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SmoothDislocationsModifier::SmoothDislocationsModifier(DataSet* dataset) : Modifier(dataset),
	_smoothingEnabled(true), _coarseningEnabled(true),
	_smoothingLevel(4), _linePointInterval(3)
{
	INIT_PROPERTY_FIELD(SmoothDislocationsModifier::_smoothingEnabled);
	INIT_PROPERTY_FIELD(SmoothDislocationsModifier::_smoothingLevel);
	INIT_PROPERTY_FIELD(SmoothDislocationsModifier::_coarseningEnabled);
	INIT_PROPERTY_FIELD(SmoothDislocationsModifier::_linePointInterval);
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool SmoothDislocationsModifier::isApplicableTo(const PipelineFlowState& input)
{
	return (input.findObject<DislocationNetworkObject>() != nullptr);
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus SmoothDislocationsModifier::modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	DislocationNetworkObject* inputDislocations = state.findObject<DislocationNetworkObject>();
	if(!inputDislocations)
		return PipelineStatus::Success;	// Nothing to smooth in the modifier's input.

	if(_coarseningEnabled || _smoothingEnabled) {
		CloneHelper cloneHelper;
		OORef<DislocationNetworkObject> outputDislocations = cloneHelper.cloneObject(inputDislocations, false);
		smoothDislocationLines(outputDislocations);
		state.replaceObject(inputDislocations, outputDislocations);
	}
	return PipelineStatus::Success;
}

/******************************************************************************
* This applies the modifier an an object.
******************************************************************************/
void SmoothDislocationsModifier::smoothDislocationLines(DislocationNetworkObject* dislocationsObj)
{
	if(_coarseningEnabled || _smoothingEnabled) {
		for(DislocationSegment* segment : dislocationsObj->modifiableSegments()) {
			std::deque<Point3> line;
			std::deque<int> coreSize;
			coarsenDislocationLine(_coarseningEnabled.value() ? _linePointInterval.value() : 0, segment->line, segment->coreSize, line, coreSize, segment->isClosedLoop(), segment->isInfiniteLine());
			smoothDislocationLine(_smoothingEnabled.value() ? _smoothingLevel.value() : 0, line, segment->isClosedLoop());
			segment->line = std::move(line);
			segment->coreSize = std::move(coreSize);
		}
		dislocationsObj->changed();
	}
}

/******************************************************************************
* Removes some of the sampling points from a dislocation line.
******************************************************************************/
void SmoothDislocationsModifier::coarsenDislocationLine(FloatType linePointInterval, const std::deque<Point3>& input, const std::deque<int>& coreSize, std::deque<Point3>& output, std::deque<int>& outputCoreSize, bool isClosedLoop, bool isInfiniteLine)
{
	OVITO_ASSERT(input.size() >= 2);
	OVITO_ASSERT(input.size() == coreSize.size());

	if(linePointInterval <= 0) {
		output = input;
		outputCoreSize = coreSize;
		return;
	}

	// Special handling for infinite lines.
	if(isInfiniteLine && input.size() >= 3) {
		int coreSizeSum = std::accumulate(coreSize.cbegin(), coreSize.cend() - 1, 0);
		int count = input.size() - 1;
		if(coreSizeSum * linePointInterval > count * count) {
			// Make it a straight line.
			Vector3 com = Vector3::Zero();
			for(auto p = input.cbegin(); p != input.cend() - 1; ++p)
				com += *p - input.front();
			output.push_back(input.front() + com / count);
			outputCoreSize.push_back(coreSizeSum / count);
			output.push_back(input.back() + com / count);
			outputCoreSize.push_back(coreSizeSum / count);
			return;
		}
	}

	// Special handling for very short segments.
	if(input.size() < 4) {
		output = input;
		outputCoreSize = coreSize;
		return;
	}

	// Always keep the end points of linear segments to not break junctions.
	if(!isClosedLoop) {
		output.push_back(input.front());
		outputCoreSize.push_back(coreSize.front());
	}

	auto inputPtr = input.cbegin();
	auto inputCoreSizePtr = coreSize.cbegin();

	int sum = 0;
	int count = 0;

	// Average over a half interval, starting from the beginning of the segment.
	Vector3 com = Vector3::Zero();
	do {
		sum += *inputCoreSizePtr;
		com += *inputPtr - input.front();
		count++;
		++inputPtr;
		++inputCoreSizePtr;
	}
	while(2*count*count < (int)(linePointInterval * sum) && count < input.size()/4);

	// Average over a half interval, starting from the end of the segment.
	auto inputPtrEnd = input.cend() - 1;
	auto inputCoreSizePtrEnd = coreSize.cend() - 1;
	while(count*count < (int)(linePointInterval * sum) && count < input.size()/2) {
		sum += *inputCoreSizePtrEnd;
		com += *inputPtrEnd - input.back();
		count++;
		--inputPtrEnd;
		--inputCoreSizePtrEnd;
	}
	OVITO_ASSERT(inputPtr < inputPtrEnd);

	if(isClosedLoop) {
		output.push_back(input.front() + com / count);
		outputCoreSize.push_back(sum / count);
	}

	while(inputPtr < inputPtrEnd)
	{
		int sum = 0;
		int count = 0;
		Vector3 com = Vector3::Zero();
		do {
			sum += *inputCoreSizePtr++;
			com.x() += inputPtr->x();
			com.y() += inputPtr->y();
			com.z() += inputPtr->z();
			count++;
			++inputPtr;
		}
		while(count*count < (int)(linePointInterval * sum) && count < input.size()/2 && inputPtr != inputPtrEnd);
		output.push_back(Point3::Origin() + com / count);
		outputCoreSize.push_back(sum / count);
	}

	if(!isClosedLoop) {
		// Always keep the end points of linear segments to not break junctions.
		output.push_back(input.back());
		outputCoreSize.push_back(coreSize.back());
	}
	else {
		output.push_back(input.back() + com / count);
		outputCoreSize.push_back(sum / count);
	}

	OVITO_ASSERT(output.size() >= 2);
	OVITO_ASSERT(!isClosedLoop || isInfiniteLine || output.size() >= 3);
}

/******************************************************************************
* Smoothes the sampling points of a dislocation line.
******************************************************************************/
void SmoothDislocationsModifier::smoothDislocationLine(int smoothingLevel, std::deque<Point3>& line, bool isLoop)
{
	if(smoothingLevel <= 0)
		return;	// Nothing to do.

	if(line.size() <= 2)
		return;	// Nothing to do.

	// This is the 2d implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

	FloatType k_PB = 0.1f;
	FloatType lambda = 0.5f;
	FloatType mu = 1.0f / (k_PB - 1.0f/lambda);
	const FloatType prefactors[2] = { lambda, mu };

	std::vector<Vector3> laplacians(line.size());
	for(int iteration = 0; iteration < smoothingLevel; iteration++) {

		for(int pass = 0; pass <= 1; pass++) {
			// Compute discrete Laplacian for each point.
			auto l = laplacians.begin();
			if(isLoop == false)
				(*l++).setZero();
			else
				(*l++) = ((*(line.end()-2) - *(line.end()-3)) + (*(line.begin()+1) - line.front())) * FloatType(0.5);

			auto p1 = line.cbegin();
			auto p2 = line.cbegin() + 1;
			for(;;) {
				auto p0 = p1;
				++p1;
				++p2;
				if(p2 == line.cend())
					break;
				*l++ = ((*p0 - *p1) + (*p2 - *p1)) * FloatType(0.5);
			}

			*l++ = laplacians.front();
			OVITO_ASSERT(l == laplacians.end());

			auto lc = laplacians.cbegin();
			for(Point3& p : line) {
				p += prefactors[pass] * (*lc++);
			}
		}
	}
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SmoothDislocationsModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the first rollout.
	QWidget* rollout = createRollout(tr("Smooth dislocations"), rolloutParams);

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);

	BooleanGroupBoxParameterUI* smoothingEnabledUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_smoothingEnabled));
	smoothingEnabledUI->groupBox()->setTitle(tr("Line smoothing"));
    QGridLayout* sublayout = new QGridLayout(smoothingEnabledUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(smoothingEnabledUI->groupBox());

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_smoothingLevel));
	sublayout->addWidget(smoothingLevelUI->label(), 0, 0);
	sublayout->addLayout(smoothingLevelUI->createFieldLayout(), 0, 1);
	smoothingLevelUI->setMinValue(0);

	BooleanGroupBoxParameterUI* coarseningEnabledUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_coarseningEnabled));
	coarseningEnabledUI->groupBox()->setTitle(tr("Line coarsening"));
    sublayout = new QGridLayout(coarseningEnabledUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(coarseningEnabledUI->groupBox());

	FloatParameterUI* linePointIntervalUI = new FloatParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_linePointInterval));
	sublayout->addWidget(linePointIntervalUI->label(), 0, 0);
	sublayout->addLayout(linePointIntervalUI->createFieldLayout(), 0, 1);
	linePointIntervalUI->setMinValue(0);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
