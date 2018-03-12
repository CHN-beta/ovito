///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/rendering/SceneRenderer.h>
#include "TrajectoryVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryVis);	
DEFINE_PROPERTY_FIELD(TrajectoryVis, lineWidth);
DEFINE_PROPERTY_FIELD(TrajectoryVis, lineColor);
DEFINE_PROPERTY_FIELD(TrajectoryVis, shadingMode);
DEFINE_PROPERTY_FIELD(TrajectoryVis, showUpToCurrentTime);
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, lineColor, "Line color");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(TrajectoryVis, showUpToCurrentTime, "Show up to current time only");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TrajectoryVis, lineWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
TrajectoryVis::TrajectoryVis(DataSet* dataset) : DataVis(dataset),
	_lineWidth(0.2), 
	_lineColor(0.6, 0.6, 0.6),
	_shadingMode(ArrowPrimitive::FlatShading), 
	_showUpToCurrentTime(false)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TrajectoryVis::boundingBox(TimePoint time, DataObject* dataObject, PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	TrajectoryObject* trajObj = dynamic_object_cast<TrajectoryObject>(dataObject);

	// Detect if the input data has changed since the last time we computed the bounding box.
	if(_boundingBoxCacheHelper.updateState(trajObj, lineWidth())) {
		// Compute bounding box.
		_cachedBoundingBox.setEmpty();
		if(trajObj) {
			_cachedBoundingBox.addPoints(trajObj->points().constData(), trajObj->points().size());
		}
	}
	return _cachedBoundingBox;
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void TrajectoryVis::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, PipelineSceneNode* contextNode)
{
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, dataObject, contextNode, flowState, validityInterval));
		return;
	}
	
	TrajectoryObject* trajObj = dynamic_object_cast<TrajectoryObject>(dataObject);

	// Do we have to re-create the geometry buffers from scratch?
	bool recreateBuffers = !_segmentBuffer || !_segmentBuffer->isValid(renderer)
						|| !_cornerBuffer || !_cornerBuffer->isValid(renderer);

	// Set up shading mode.
	ParticlePrimitive::ShadingMode cornerShadingMode = (shadingMode() == ArrowPrimitive::NormalShading)
			? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading;
	if(!recreateBuffers) {
		recreateBuffers |= !_segmentBuffer->setShadingMode(shadingMode());
		recreateBuffers |= !_cornerBuffer->setShadingMode(cornerShadingMode);
	}

	TimePoint endTime = showUpToCurrentTime() ? time : TimePositiveInfinity();

	// Do we have to update contents of the geometry buffers?
	bool updateContents = _geometryCacheHelper.updateState(trajObj, lineWidth(), lineColor(), endTime) || recreateBuffers;

	// Re-create the geometry buffers if necessary.
	if(recreateBuffers) {
		_segmentBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), ArrowPrimitive::HighQuality);
		_cornerBuffer = renderer->createParticlePrimitive(cornerShadingMode, ParticlePrimitive::HighQuality);
	}

	if(updateContents) {
		FloatType lineRadius = lineWidth() / 2;
		if(trajObj && lineRadius > 0) {
			int timeSamples = std::upper_bound(trajObj->sampleTimes().cbegin(), trajObj->sampleTimes().cend(), endTime) - trajObj->sampleTimes().cbegin();

			int lineSegmentCount = std::max(0, timeSamples - 1) * trajObj->trajectoryCount();

			_segmentBuffer->startSetElements(lineSegmentCount);
			int lineSegmentIndex = 0;
			for(int pindex = 0; pindex < trajObj->trajectoryCount(); pindex++) {
				for(int tindex = 0; tindex < timeSamples - 1; tindex++) {
					const Point3& p1 = trajObj->points()[tindex * trajObj->trajectoryCount() + pindex];
					const Point3& p2 = trajObj->points()[(tindex+1) * trajObj->trajectoryCount() + pindex];
					_segmentBuffer->setElement(lineSegmentIndex++, p1, p2 - p1, ColorA(lineColor()), lineRadius);
				}
			}
			_segmentBuffer->endSetElements();

			int pointCount = std::max(0, timeSamples - 2) * trajObj->trajectoryCount();
			_cornerBuffer->setSize(pointCount);
			if(pointCount)
				_cornerBuffer->setParticlePositions(trajObj->points().constData() + trajObj->trajectoryCount());
			_cornerBuffer->setParticleColor(ColorA(lineColor()));
			_cornerBuffer->setParticleRadius(lineRadius);
		}
		else {
			_segmentBuffer.reset();
			_cornerBuffer.reset();
		}
	}

	if(!_segmentBuffer)
		return;

	renderer->beginPickObject(contextNode);
	_segmentBuffer->render(renderer);
	_cornerBuffer->render(renderer);
	renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace