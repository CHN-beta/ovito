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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/LinePrimitive.h>
#include <ovito/core/rendering/CylinderPrimitive.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include "SimulationCellVis.h"
#include "SimulationCellObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(SimulationCellVis);
DEFINE_PROPERTY_FIELD(SimulationCellVis, cellLineWidth);
DEFINE_PROPERTY_FIELD(SimulationCellVis, defaultCellLineWidth);
DEFINE_PROPERTY_FIELD(SimulationCellVis, renderCellEnabled);
DEFINE_PROPERTY_FIELD(SimulationCellVis, cellColor);
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, cellLineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, defaultCellLineWidth, "Default line width");
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, renderCellEnabled, "Visible in rendered images");
SET_PROPERTY_FIELD_LABEL(SimulationCellVis, cellColor, "Line color");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimulationCellVis, cellLineWidth, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimulationCellVis, defaultCellLineWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
SimulationCellVis::SimulationCellVis(DataSet* dataset) : DataVis(dataset),
	_renderCellEnabled(true),
	_cellLineWidth(0.0),
	_defaultCellLineWidth(0.0),
	_cellColor(0, 0, 0)
{
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 SimulationCellVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const SimulationCellObject* cellObject = dynamic_object_cast<SimulationCellObject>(objectStack.back());
	OVITO_CHECK_OBJECT_POINTER(cellObject);

	AffineTransformation matrix = cellObject->cellMatrix();
	if(cellObject->is2D()) {
		matrix.column(2).setZero();
		matrix.translation().z() = 0;
	}

	return Box3(Point3(0), Point3(1)).transformed(matrix);
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void SimulationCellVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	const SimulationCellObject* cell = dynamic_object_cast<SimulationCellObject>(objectStack.back());
	OVITO_CHECK_OBJECT_POINTER(cell);
	if(!cell) return;

	if(renderer->isInteractive() && !renderer->viewport()->renderPreviewMode()) {
		if(!renderer->isBoundingBoxPass()) {
			renderWireframe(time, cell, flowState, renderer, contextNode);
		}
		else {
			TimeInterval validityInterval;
			renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		}
	}
	else {
		if(!renderCellEnabled())
			return;		// Do nothing if rendering has been disabled by the user.

		if(!renderer->isBoundingBoxPass()) {
			renderSolid(time, cell, flowState, renderer, contextNode);
		}
		else {
			TimeInterval validityInterval;
			Box3 bb = boundingBox(time, objectStack, contextNode, flowState, validityInterval);
			renderer->addToLocalBoundingBox(bb.padBox(cellLineWidth()));
		}
	}
}

/******************************************************************************
* Renders the given simulation cell using lines.
******************************************************************************/
void SimulationCellVis::renderWireframe(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	OVITO_ASSERT(!renderer->isBoundingBoxPass());

	// The key type used for caching the geometry primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,// Scene renderer
		bool,					// 2D cell
		ColorA					// Wireframe color
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<LinePrimitive> linesRendering;
		std::shared_ptr<LinePrimitive> linesPicking;
	};

	// Cell wireframe color.
	ColorA color = ViewportSettings::getSettings().viewportColor(contextNode->isSelected() ? ViewportSettings::COLOR_SELECTION : ViewportSettings::COLOR_UNSELECTED);

	// Lookup the rendering primitive in the vis cache.
	auto& wireframePrimitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, cell->is2D(), color));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!wireframePrimitives.linesRendering || !wireframePrimitives.linesPicking) {

		wireframePrimitives.linesRendering = renderer->createLinePrimitive();
		wireframePrimitives.linesPicking = renderer->createLinePrimitive();
		wireframePrimitives.linesRendering->setUniformColor(color);
		wireframePrimitives.linesPicking->setUniformColor(color);
		wireframePrimitives.linesPicking->setLineWidth(renderer->defaultLinePickingWidth());

		// Depending on whether this cell is 3D or 2D, create a wireframe unit cube or unit square.
		DataBufferAccessAndRef<Point3> corners = DataBufferPtr::create(renderer->dataset(), ExecutionContext::Scripting, cell->is2D() ? 8 : 24, DataBuffer::Float, 3, 0, false);
		corners[0] = Point3(0,0,0);
		corners[1] = Point3(1,0,0);
		corners[2] = Point3(1,0,0);
		corners[3] = Point3(1,1,0);
		corners[4] = Point3(1,1,0);
		corners[5] = Point3(0,1,0);
		corners[6] = Point3(0,1,0);
		corners[7] = Point3(0,0,0);
		if(!cell->is2D()) {
			corners[8]  = Point3(0,0,1);
			corners[9]  = Point3(1,0,1);
			corners[10] = Point3(1,0,1);
			corners[11] = Point3(1,1,1);
			corners[12] = Point3(1,1,1);
			corners[13] = Point3(0,1,1);
			corners[14] = Point3(0,1,1);
			corners[15] = Point3(0,0,1);
			corners[16] = Point3(0,0,0);
			corners[17] = Point3(0,0,1);
			corners[18] = Point3(1,0,0);
			corners[19] = Point3(1,0,1);
			corners[20] = Point3(1,1,0);
			corners[21] = Point3(1,1,1);
			corners[22] = Point3(0,1,0);
			corners[23] = Point3(0,1,1);
		}
		wireframePrimitives.linesRendering->setPositions(corners.take());
		wireframePrimitives.linesPicking->setPositions(wireframePrimitives.linesRendering->positions());
	}

	const AffineTransformation oldTM = renderer->worldTransform();
	AffineTransformation cellMatrix = cell->cellMatrix();
	if(cell->is2D()) 
		cellMatrix(2,3) = 0; // For 2D cells, implicitly set z-coordinate of origin to zero.	
	renderer->setWorldTransform(oldTM * cellMatrix);
	if(renderer->isPicking())
		renderer->beginPickObject(contextNode);
	renderer->renderLines(renderer->isPicking() ? wireframePrimitives.linesPicking : wireframePrimitives.linesRendering);
	if(renderer->isPicking())
		renderer->endPickObject();
	renderer->setWorldTransform(oldTM);
}

/******************************************************************************
* Renders the given simulation cell using solid shading mode.
******************************************************************************/
void SimulationCellVis::renderSolid(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	OVITO_ASSERT(!renderer->isBoundingBoxPass());

	// The key type used for caching the geometry primitive:
	using CacheKey = std::tuple<
		CompatibleRendererGroup,	// Scene renderer
		ConstDataObjectRef,			// Simulation cell
		FloatType, 					// Line width
		Color						// Line color
	>;

	// The values stored in the vis cache.
	struct CacheValue {
		std::shared_ptr<CylinderPrimitive> edges;
		std::shared_ptr<ParticlePrimitive> corners;
	};

	// Lookup the rendering primitive in the vis cache.
	auto& visCache = dataset()->visCache().get<CacheValue>(CacheKey(renderer, cell, cellLineWidth(), cellColor()));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!visCache.edges || !visCache.corners) {

		visCache.edges = renderer->createCylinderPrimitive(CylinderPrimitive::CylinderShape, CylinderPrimitive::NormalShading, CylinderPrimitive::HighQuality);
		visCache.edges->setUniformColor(cellColor());
		visCache.edges->setUniformRadius(cellLineWidth());

		// Create a data buffer for the box corner coordinates.
		DataBufferAccessAndRef<Point3> corners = DataOORef<DataBuffer>::create(dataset(), ExecutionContext::Scripting, cell->is2D() ? 4 : 8, DataBuffer::Float, 3, 0, false);

		// Create a data buffer for the cylinder base points.
		DataBufferAccessAndRef<Point3> basePoints = DataOORef<DataBuffer>::create(dataset(), ExecutionContext::Scripting, cell->is2D() ? 4 : 12, DataBuffer::Float, 3, 0, false);

		// Create a data buffer for the cylinder head points.
		DataBufferAccessAndRef<Point3> headPoints = DataOORef<DataBuffer>::create(dataset(), ExecutionContext::Scripting, cell->is2D() ? 4 : 12, DataBuffer::Float, 3, 0, false);

		corners[0] = cell->cellOrigin();
		if(cell->is2D()) corners[0].z() = 0; // For 2D cells, implicitly set z-coordinate of origin to zero.
		corners[1] = corners[0] + cell->cellVector1();
		corners[2] = corners[1] + cell->cellVector2();
		corners[3] = corners[0] + cell->cellVector2();
		basePoints[0] = corners[0];
		basePoints[1] = corners[1];
		basePoints[2] = corners[2];
		basePoints[3] = corners[3];
		headPoints[0] = corners[1];
		headPoints[1] = corners[2];
		headPoints[2] = corners[3];
		headPoints[3] = corners[0];
		if(cell->is2D() == false) {
			corners[4] = corners[0] + cell->cellVector3();
			corners[5] = corners[1] + cell->cellVector3();
			corners[6] = corners[2] + cell->cellVector3();
			corners[7] = corners[3] + cell->cellVector3();
			basePoints[4] = corners[4];
			basePoints[5] = corners[5];
			basePoints[6] = corners[6];
			basePoints[7] = corners[7];
			basePoints[8] = corners[0];
			basePoints[9] = corners[1];
			basePoints[10] = corners[2];
			basePoints[11] = corners[3];
			headPoints[4] = corners[5];
			headPoints[5] = corners[6];
			headPoints[6] = corners[7];
			headPoints[7] = corners[4];
			headPoints[8] = corners[4];
			headPoints[9] = corners[5];
			headPoints[10] = corners[6];
			headPoints[11] = corners[7];
		}
		visCache.edges->setPositions(basePoints.take(), headPoints.take());

		// Render spheres in the corners of the simulation box.
		visCache.corners  = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality);
		visCache.corners->setPositions(corners.take());
		visCache.corners->setUniformRadius(cellLineWidth());
		visCache.corners->setUniformColor(cellColor());
	}
	renderer->beginPickObject(contextNode);
	renderer->renderCylinders(visCache.edges);
	renderer->renderParticles(visCache.corners);
	renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace
