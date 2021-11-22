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
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/app/PluginManager.h>
#include "SliceModifier.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(SliceModifierDelegate);
IMPLEMENT_OVITO_CLASS(SliceModifier);
DEFINE_REFERENCE_FIELD(SliceModifier, normalController);
DEFINE_REFERENCE_FIELD(SliceModifier, distanceController);
DEFINE_REFERENCE_FIELD(SliceModifier, widthController);
DEFINE_PROPERTY_FIELD(SliceModifier, createSelection);
DEFINE_PROPERTY_FIELD(SliceModifier, inverse);
DEFINE_PROPERTY_FIELD(SliceModifier, applyToSelection);
DEFINE_PROPERTY_FIELD(SliceModifier, enablePlaneVisualization);
DEFINE_PROPERTY_FIELD(SliceModifier, reducedCoordinates);
DEFINE_REFERENCE_FIELD(SliceModifier, planeVis);
SET_PROPERTY_FIELD_LABEL(SliceModifier, normalController, "Normal");
SET_PROPERTY_FIELD_LABEL(SliceModifier, distanceController, "Distance");
SET_PROPERTY_FIELD_LABEL(SliceModifier, widthController, "Slab width");
SET_PROPERTY_FIELD_LABEL(SliceModifier, createSelection, "Create selection (do not delete)");
SET_PROPERTY_FIELD_LABEL(SliceModifier, inverse, "Reverse orientation");
SET_PROPERTY_FIELD_LABEL(SliceModifier, applyToSelection, "Apply to selection only");
SET_PROPERTY_FIELD_LABEL(SliceModifier, enablePlaneVisualization, "Visualize plane");
SET_PROPERTY_FIELD_LABEL(SliceModifier, reducedCoordinates, "Reduced cell coordinates");
SET_PROPERTY_FIELD_LABEL(SliceModifier, planeVis, "Plane");
SET_PROPERTY_FIELD_UNITS(SliceModifier, normalController, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SliceModifier, distanceController, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SliceModifier, widthController, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SliceModifier::SliceModifier(DataSet* dataset) : MultiDelegatingModifier(dataset),
	_createSelection(false),
	_inverse(false),
	_applyToSelection(false),
	_enablePlaneVisualization(false),
	_reducedCoordinates(false)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void SliceModifier::initializeObject(ExecutionContext executionContext)
{
	setNormalController(ControllerManager::createVector3Controller(dataset(), executionContext));
	setDistanceController(ControllerManager::createFloatController(dataset(), executionContext));
	setWidthController(ControllerManager::createFloatController(dataset(), executionContext));
	if(normalController()) normalController()->setVector3Value(0, Vector3(1,0,0));

	// Generate the list of delegate objects.
	createModifierDelegates(SliceModifierDelegate::OOClass(), executionContext);

	MultiDelegatingModifier::initializeObject(executionContext);

	// Create the vis element for the plane.
	setPlaneVis(OORef<TriMeshVis>::create(dataset(), executionContext));
	planeVis()->setTitle(tr("Plane"));
	planeVis()->setHighlightEdges(true);
	planeVis()->setTransparency(0.5);
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval SliceModifier::validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const
{
	TimeInterval iv = MultiDelegatingModifier::validityInterval(request, modApp);
	if(normalController()) iv.intersect(normalController()->validityInterval(request.time()));
	if(distanceController()) iv.intersect(distanceController()->validityInterval(request.time()));
	if(widthController()) iv.intersect(widthController()->validityInterval(request.time()));
	return iv;
}

/******************************************************************************
* Returns the slicing plane and the slab width.
******************************************************************************/
std::tuple<Plane3, FloatType> SliceModifier::slicingPlane(TimePoint time, TimeInterval& validityInterval, const PipelineFlowState& state)
{
	Plane3 plane;

	if(normalController())
		normalController()->getVector3Value(time, plane.normal, validityInterval);

	if(plane.normal == Vector3::Zero())
		plane.normal = Vector3(0,0,1);

	if(distanceController())
		plane.dist = distanceController()->getFloatValue(time, validityInterval);

	if(inverse())
		plane = -plane;

	if(reducedCoordinates()) {
		if(const SimulationCellObject* cell = state.getObject<SimulationCellObject>()) {
			plane.normal /= plane.normal.squaredLength();
			plane = cell->cellMatrix() * plane;
		}
		else {
			throwException(tr("Slicing plane was specified in reduced cell coordinates but there is no simulation cell."));
		}
	}
	else {
		plane.normal.normalize();
	}

	FloatType slabWidth = 0;
	if(widthController())
		slabWidth = widthController()->getFloatValue(time, validityInterval);

	return std::make_tuple(plane, slabWidth);
}

/******************************************************************************
* Lets the modifier render itself into the viewport.
******************************************************************************/
void SliceModifier::renderModifierVisual(TimePoint time, PipelineSceneNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay)
{
	if(!renderOverlay && isObjectBeingEdited() && renderer->isInteractive() && !renderer->isPicking()) {
		const PipelineFlowState& state = modApp->evaluateInputSynchronous(time);
		renderVisual(time, contextNode, renderer, state);
	}
}

/******************************************************************************
* Renders the modifier's visual representation and computes its bounding box.
******************************************************************************/
void SliceModifier::renderVisual(TimePoint time, PipelineSceneNode* contextNode, SceneRenderer* renderer, const PipelineFlowState& state)
{
	TimeInterval interval;

	Box3 bb = contextNode->localBoundingBox(time, interval);
	if(bb.isEmpty())
		return;

	// Obtain modifier parameter values.
	Plane3 plane;
	FloatType slabWidth;
	std::tie(plane, slabWidth) = slicingPlane(time, interval, state);
	if(plane.normal.isZero())
		return;

	ColorA color(0.8, 0.3, 0.3);
	if(slabWidth <= 0) {
		renderPlane(renderer, plane, bb, color);
	}
	else {
		plane.dist += slabWidth / 2;
		renderPlane(renderer, plane, bb, color);
		plane.dist -= slabWidth;
		renderPlane(renderer, plane, bb, color);
	}
}

/******************************************************************************
* Renders the plane in the viewports.
******************************************************************************/
void SliceModifier::renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& bb, const ColorA& color) const
{
	// Compute intersection lines of slicing plane and bounding box.
	Point3 corners[8];
	for(size_t i = 0; i < 8; i++)
		corners[i] = bb[i];

	std::vector<Point3> vertices;
	vertices.reserve(8);
	planeQuadIntersection(corners, {{0, 1, 5, 4}}, plane, vertices);
	planeQuadIntersection(corners, {{1, 3, 7, 5}}, plane, vertices);
	planeQuadIntersection(corners, {{3, 2, 6, 7}}, plane, vertices);
	planeQuadIntersection(corners, {{2, 0, 4, 6}}, plane, vertices);
	planeQuadIntersection(corners, {{4, 5, 7, 6}}, plane, vertices);
	planeQuadIntersection(corners, {{0, 2, 3, 1}}, plane, vertices);

	// If there is not intersection with the simulation box then
	// project the simulation box onto the plane.
	if(vertices.empty()) {
		const static int edges[12][2] = {
				{0,1},{1,3},{3,2},{2,0},
				{4,5},{5,7},{7,6},{6,4},
				{0,4},{1,5},{3,7},{2,6}
		};
		vertices.reserve(24);
		for(int edge = 0; edge < 12; edge++) {
			vertices.push_back(plane.projectPoint(corners[edges[edge][0]]));
			vertices.push_back(plane.projectPoint(corners[edges[edge][1]]));
		}
	}

	// Render plane-box intersection lines.
	if(renderer->isBoundingBoxPass()) {
		Box3 vertexBoundingBox;
		vertexBoundingBox.addPoints(vertices);
		renderer->addToLocalBoundingBox(vertexBoundingBox);
	}
	else {
		DataBufferAccessAndRef<Point3> positions = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, vertices.size(), DataBuffer::Float, 3, 0, false);
		boost::range::copy(vertices, positions.begin());
		std::shared_ptr<LinePrimitive> buffer = renderer->createLinePrimitive();
		buffer->setPositions(positions.take());
		buffer->setUniformColor(color);
		renderer->renderLines(buffer);
	}
}

/******************************************************************************
* Computes the intersection lines of a plane and a quad.
******************************************************************************/
void SliceModifier::planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, std::vector<Point3>& vertices) const
{
	Point3 p1;
	bool hasP1 = false;
	for(int i = 0; i < 4; i++) {
		Ray3 edge(corners[quadVerts[i]], corners[quadVerts[(i+1)%4]]);
		FloatType t = plane.intersectionT(edge, FLOATTYPE_EPSILON);
		if(t < 0 || t > 1) continue;
		if(!hasP1) {
			p1 = edge.point(t);
			hasP1 = true;
		}
		else {
			Point3 p2 = edge.point(t);
			if(!p2.equals(p1)) {
				vertices.push_back(p1);
				vertices.push_back(p2);
				return;
			}
		}
	}
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a PipelineObject.
******************************************************************************/
void SliceModifier::initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext)
{
	MultiDelegatingModifier::initializeModifier(time, modApp, executionContext);

	// Get the input simulation cell to initially place the cutting plane in
	// the center of the cell.
	const PipelineFlowState& input = modApp->evaluateInputSynchronous(time);
	if(const SimulationCellObject* cell = input.getObject<SimulationCellObject>()) {
		TimeInterval iv;
		if(distanceController() && distanceController()->getFloatValue(0, iv) == 0) {
			Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
			FloatType centerDistance = normal().dot(centerPoint - Point3::Origin());
			if(std::abs(centerDistance) > FLOATTYPE_EPSILON && distanceController())
				distanceController()->setFloatValue(0, centerDistance);
		}
	}
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void SliceModifier::evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	MultiDelegatingModifier::evaluateSynchronous(time, modApp, state);

	if(enablePlaneVisualization()) {

		Plane3 plane;
		FloatType slabWidth;
		TimeInterval interval;
		std::tie(plane, slabWidth) = slicingPlane(time, interval, state);
		if(plane.normal.isZero())
			return;

		// Compute intersection polygon of slicing plane with simulation cell.
		const SimulationCellObject* cellObj = state.expectObject<SimulationCellObject>();
		const AffineTransformation& cellMatrix = cellObj->cellMatrix();

		// Compute intersection lines of slicing plane and simulation cell.
		TriMeshPtr mesh = std::make_shared<TriMesh>();
		auto createIntersectionPolygon = [&](const Plane3& plane) {
			QVector<Point3> vertices;
			auto planeEdgeIntersection = [&](const Vector3& b, const Vector3& d) {
				Ray3 edge(Point3::Origin() + b, d);
				FloatType t = plane.intersectionT(edge, FLOATTYPE_EPSILON);
				if(t >= -FLOATTYPE_EPSILON && t <= 1 + FLOATTYPE_EPSILON)
					vertices.push_back(edge.point(t));
			};
			planeEdgeIntersection(cellMatrix.translation(), cellMatrix.column(0));
			planeEdgeIntersection(cellMatrix.translation(), cellMatrix.column(1));
			planeEdgeIntersection(cellMatrix.translation(), cellMatrix.column(2));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(0), cellMatrix.column(1));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(0), cellMatrix.column(2));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(1), cellMatrix.column(0));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(1), cellMatrix.column(2));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(2), cellMatrix.column(0));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(2), cellMatrix.column(1));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(0) + cellMatrix.column(1), cellMatrix.column(2));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(1) + cellMatrix.column(2), cellMatrix.column(0));
			planeEdgeIntersection(cellMatrix.translation() + cellMatrix.column(2) + cellMatrix.column(0), cellMatrix.column(1));
			if(vertices.size() < 3) return;
			vertices.erase(std::remove_if(vertices.begin() + 1, vertices.end(), 
				[p = vertices.front()](const Point3& p2) { return p2.equals(p); }), vertices.end());
			if(vertices.size() < 3) return;
			std::sort(vertices.begin() + 1, vertices.end(), [&](const Point3& a, const Point3& b) {
				return (a - vertices.front()).cross(b - vertices.front()).dot(plane.normal) < 0;
			});
			int baseVertexCount = mesh->vertexCount();
			mesh->setVertexCount(baseVertexCount + vertices.size());
			std::copy(vertices.begin(), vertices.end(), mesh->vertices().begin() + baseVertexCount);
			for(int f = 2; f < vertices.size(); f++) {
				TriMeshFace& face = mesh->addFace();
				face.setVertices(baseVertexCount, baseVertexCount+f-1, baseVertexCount+f);
				face.setEdgeVisibility(f == 2, true, f == vertices.size()-1);
			}
		};
		if(slabWidth <= 0) {
			createIntersectionPolygon(plane);
		}
		else {
			plane.dist += slabWidth / 2;
			createIntersectionPolygon(plane);
			plane.dist -= slabWidth;
			createIntersectionPolygon(plane);
		}

		// Create an output mesh for visualizing the cutting plane.
		TriMeshObject* meshObj = state.createObject<TriMeshObject>(QStringLiteral("plane"), modApp, ExecutionContext::Scripting);
		meshObj->setMesh(std::move(mesh));
		meshObj->setVisElement(planeVis());
	}
}

/******************************************************************************
* Moves the plane along its current normal vector to position in the center of the simulation cell. 
******************************************************************************/
void SliceModifier::centerPlaneInSimulationCell(ModifierApplication* modApp)
{
	if(!modApp) return;

	// Get the simulation cell from the input object to center the slicing plane in
	// the center of the simulation cell.
	const PipelineFlowState& input = modApp->evaluateSynchronous(dataset()->animationSettings()->time());
	if(const SimulationCellObject* cell = input.getObject<SimulationCellObject>()) {

		FloatType centerDistance;
		if(!reducedCoordinates()) {
			Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
			centerDistance = normal().safelyNormalized().dot(centerPoint - Point3::Origin());
		}
		else {
			if(!normal().isZero())
				centerDistance = normal().dot(Vector3(0.5, 0.5, 0.5));
			else
				centerDistance = distance();
		}

		setDistance(centerDistance);
	}
}

}	// End of namespace
