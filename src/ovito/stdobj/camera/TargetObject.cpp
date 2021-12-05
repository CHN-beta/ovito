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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include "TargetObject.h"

namespace Ovito::StdObj {


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
void TargetObject::initializeObject(ObjectInitializationHints hints)
{
	if(!visElement() && !hints.testFlag(WithoutVisElement))
		setVisElement(OORef<TargetVis>::create(dataset(), hints));

	DataObject::initializeObject(hints);
}

/******************************************************************************
* Lets the vis element render a data object.
******************************************************************************/
PipelineStatus TargetVis::render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Target objects are only visible in the viewports.
	if(renderer->isInteractive() == false || renderer->viewport() == nullptr)
		return {};

	// Setup transformation matrix to always show the icon at the same size.
	Point3 objectPos = Point3::Origin() + renderer->worldTransform().translation();
	FloatType scaling = FloatType(0.2) * renderer->viewport()->nonScalingSize(objectPos);
	renderer->setWorldTransform(renderer->worldTransform() * AffineTransformation::scaling(scaling));

	if(!renderer->isBoundingBoxPass()) {

		// Cache the line vertices for the icon.
		RendererResourceKey<struct WireframeCube, DataSet*> cacheKey{ renderer->dataset() };
		auto& vertexPositions = dataset()->visCache().get<ConstDataBufferPtr>(std::move(cacheKey));

		// Initialize geometry of wireframe cube.
		if(!vertexPositions) {
			const Point3 linePoints[] = {
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
			DataBufferAccessAndRef<Point3> vertices = DataBufferPtr::create(renderer->dataset(), sizeof(linePoints) / sizeof(Point3), DataBuffer::Float, 3, 0, false);
			boost::copy(linePoints, vertices.begin());
			vertexPositions = vertices.take();
		}

		// Create line rendering primitive.
		LinePrimitive iconPrimitive;
		iconPrimitive.setUniformColor(ViewportSettings::getSettings().viewportColor(contextNode->isSelected() ? ViewportSettings::COLOR_SELECTION : ViewportSettings::COLOR_CAMERAS));
		iconPrimitive.setPositions(vertexPositions);
		if(renderer->isPicking())
			iconPrimitive.setLineWidth(renderer->defaultLinePickingWidth());

		// Render the lines.
		renderer->beginPickObject(contextNode);
		renderer->renderLines(iconPrimitive);
		renderer->endPickObject();
	}
	else {
		// Add target symbol to bounding box.
		renderer->addToLocalBoundingBox(Box3(Point3::Origin(), scaling));
	}

	return {};
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TargetVis::boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// This is not a physical object. It is point-like and doesn't have any size.
	return Box3(Point3::Origin(), Point3::Origin());
}

}	// End of namespace
