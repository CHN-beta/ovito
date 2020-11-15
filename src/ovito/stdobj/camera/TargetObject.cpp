////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include "TargetObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(TargetObject);
IMPLEMENT_OVITO_CLASS(TargetVis);

/******************************************************************************
* Constructs a target object.
******************************************************************************/
TargetObject::TargetObject(DataSet* dataset) : DataObject(dataset)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void TargetObject::initializeObject(Application::ExecutionContext executionContext)
{
	addVisElement(OORef<TargetVis>::create(dataset(), executionContext));

	DataObject::initializeObject(executionContext);
}

/******************************************************************************
* Lets the vis element render a data object.
******************************************************************************/
void TargetVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Target objects are only visible in the viewports.
	if(renderer->isInteractive() == false || renderer->viewport() == nullptr)
		return;

	// Setup transformation matrix to always show the icon at the same size.
	Point3 objectPos = Point3::Origin() + renderer->worldTransform().translation();
	FloatType scaling = FloatType(0.2) * renderer->viewport()->nonScalingSize(objectPos);
	renderer->setWorldTransform(renderer->worldTransform() * AffineTransformation::scaling(scaling));

	if(!renderer->isBoundingBoxPass()) {

		// The key type used for caching the geometry primitive:
		using CacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			Color						// Display color
		>;

		// The values stored in the vis cache.
		struct CacheValue {
			std::shared_ptr<LinePrimitive> icon;
			std::shared_ptr<LinePrimitive> pickIcon;
		};

		// Determine icon color depending on selection state.
		Color color = ViewportSettings::getSettings().viewportColor(contextNode->isSelected() ? ViewportSettings::COLOR_SELECTION : ViewportSettings::COLOR_CAMERAS);

		// Lookup the rendering primitive in the vis cache.
		auto& targetPrimitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, color));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!targetPrimitives.icon || !targetPrimitives.pickIcon
				|| !targetPrimitives.icon->isValid(renderer)
				|| !targetPrimitives.pickIcon->isValid(renderer)) {

			targetPrimitives.icon = renderer->createLinePrimitive();
			targetPrimitives.pickIcon = renderer->createLinePrimitive();

			// Initialize lines.
			static const Point3 linePoints[] = {
					{-1, -1, -1}, { 1,-1,-1},
					{-1, -1,  1}, { 1,-1, 1},
					{-1, -1, -1}, {-1,-1, 1},
					{ 1, -1, -1}, { 1,-1, 1},
					{-1,  1, -1}, { 1, 1,-1},
					{-1,  1,  1}, { 1, 1, 1},
					{-1,  1, -1}, {-1, 1, 1},
					{ 1,  1, -1}, { 1, 1, 1},
					{-1, -1, -1}, {-1, 1,-1},
					{ 1, -1, -1}, { 1, 1,-1},
					{ 1, -1,  1}, { 1, 1, 1},
					{-1, -1,  1}, {-1, 1, 1}
			};

			targetPrimitives.icon->setVertexCount(24);
			targetPrimitives.icon->setVertexPositions(linePoints);
			targetPrimitives.icon->setLineColor(ColorA(color));

			targetPrimitives.pickIcon->setVertexCount(24, renderer->defaultLinePickingWidth());
			targetPrimitives.pickIcon->setVertexPositions(linePoints);
			targetPrimitives.pickIcon->setLineColor(ColorA(color));
		}

		renderer->beginPickObject(contextNode);
		if(!renderer->isPicking())
			targetPrimitives.icon->render(renderer);
		else
			targetPrimitives.pickIcon->render(renderer);
		renderer->endPickObject();
	}
	else {
		// Add target symbol to bounding box.
		renderer->addToLocalBoundingBox(Box3(Point3::Origin(), scaling));
	}
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TargetVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// This is not a physical object. It doesn't have a size.
	return Box3(Point3::Origin(), Point3::Origin());
}

}	// End of namespace
}	// End of namespace
