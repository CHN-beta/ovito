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

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportManager.h>
#include <core/dataset/DataSetManager.h>
#include <core/scene/ObjectNode.h>
#include <core/animation/AnimManager.h>
#include <core/animation/controller/StandardControllers.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/gui/actions/ActionManager.h>
#include <core/gui/mainwin/MainWindow.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/Vector3ParameterUI.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/rendering/viewport/ViewportSceneRenderer.h>
#include <viz/data/SimulationCell.h>
#include "SliceModifier.h"

namespace Viz {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Viz, SliceModifier, ParticleModifier)
IMPLEMENT_OVITO_OBJECT(Viz, SliceModifierEditor, ParticleModifierEditor)
SET_OVITO_OBJECT_EDITOR(SliceModifier, SliceModifierEditor)
DEFINE_REFERENCE_FIELD(SliceModifier, _normalCtrl, "PlaneNormal", VectorController)
DEFINE_REFERENCE_FIELD(SliceModifier, _distanceCtrl, "PlaneDistance", FloatController)
DEFINE_REFERENCE_FIELD(SliceModifier, _widthCtrl, "SliceWidth", FloatController)
DEFINE_PROPERTY_FIELD(SliceModifier, _createSelection, "CreateSelection")
DEFINE_PROPERTY_FIELD(SliceModifier, _inverse, "Inverse")
DEFINE_PROPERTY_FIELD(SliceModifier, _applyToSelection, "ApplyToSelection")
SET_PROPERTY_FIELD_LABEL(SliceModifier, _normalCtrl, "Normal")
SET_PROPERTY_FIELD_LABEL(SliceModifier, _distanceCtrl, "Distance")
SET_PROPERTY_FIELD_LABEL(SliceModifier, _widthCtrl, "Slice width")
SET_PROPERTY_FIELD_LABEL(SliceModifier, _createSelection, "Select particles (do not delete)")
SET_PROPERTY_FIELD_LABEL(SliceModifier, _inverse, "Invert")
SET_PROPERTY_FIELD_LABEL(SliceModifier, _applyToSelection, "Apply to selected particles only")
SET_PROPERTY_FIELD_UNITS(SliceModifier, _normalCtrl, WorldParameterUnit)
SET_PROPERTY_FIELD_UNITS(SliceModifier, _distanceCtrl, WorldParameterUnit)
SET_PROPERTY_FIELD_UNITS(SliceModifier, _widthCtrl, WorldParameterUnit)

/******************************************************************************
* Constructs the modifier object.
******************************************************qDebug() << "Rendering color scale " << renderer->viewport();************************/
SliceModifier::SliceModifier() :
	_createSelection(false),
	_inverse(false),
	_applyToSelection(false)
{
	INIT_PROPERTY_FIELD(SliceModifier::_normalCtrl);
	INIT_PROPERTY_FIELD(SliceModifier::_distanceCtrl);
	INIT_PROPERTY_FIELD(SliceModifier::_widthCtrl);
	INIT_PROPERTY_FIELD(SliceModifier::_createSelection);
	INIT_PROPERTY_FIELD(SliceModifier::_inverse);
	INIT_PROPERTY_FIELD(SliceModifier::_applyToSelection);

	_normalCtrl = ControllerManager::instance().createDefaultController<VectorController>();
	_distanceCtrl = ControllerManager::instance().createDefaultController<FloatController>();
	_widthCtrl = ControllerManager::instance().createDefaultController<FloatController>();
	setNormal(Vector3(1,0,0));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval SliceModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = Modifier::modifierValidity(time);
	interval.intersect(_normalCtrl->validityInterval(time));
	interval.intersect(_distanceCtrl->validityInterval(time));
	interval.intersect(_widthCtrl->validityInterval(time));
	return interval;
}

/******************************************************************************
* Returns the slicing plane.
******************************************************************************/
Plane3 SliceModifier::slicingPlane(TimePoint time, TimeInterval& validityInterval)
{
	Plane3 plane;
	_normalCtrl->getValue(time, plane.normal, validityInterval);
	if(plane.normal == Vector3::Zero()) plane.normal = Vector3(0,0,1);
	else plane.normal.normalize();
	_distanceCtrl->getValue(time, plane.dist, validityInterval);
	if(inverse())
		return -plane;
	else
		return plane;
}

/******************************************************************************
* Modifies the particle object.
******************************************************************************/
ObjectStatus SliceModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	QString statusMessage = tr("%n input particles", 0, inputParticleCount());

	// Compute filter mask.
	std::vector<bool> mask(inputParticleCount());
	size_t numRejected = filterParticles(mask, time, validityInterval);
	size_t numKept = inputParticleCount() - numRejected;

	if(createSelection() == false) {

		statusMessage += tr("\n%n particles deleted", 0, numRejected);
		statusMessage += tr("\n%n particles remaining", 0, numKept);
		if(numRejected == 0)
			return ObjectStatus(ObjectStatus::Success, statusMessage);

		// Delete the rejected particles.
		deleteParticles(mask, numRejected);
	}
	else {
		statusMessage += tr("\n%n particles selected", 0, numRejected);
		statusMessage += tr("\n%n particles unselected", 0, numKept);

		ParticlePropertyObject* selProperty = outputStandardProperty(ParticleProperty::SelectionProperty);
		OVITO_ASSERT(mask.size() == selProperty->size());
		auto m = mask.begin();
		for(int& s : selProperty->intRange())
			s = *m++;
		selProperty->changed();
	}
	return ObjectStatus(ObjectStatus::Success, statusMessage);
}

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
size_t SliceModifier::filterParticles(std::vector<bool>& mask, TimePoint time, TimeInterval& validityInterval)
{
	// Get the required input properties.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	ParticlePropertyObject* selProperty = applyToSelection() ? inputStandardProperty(ParticleProperty::SelectionProperty) : nullptr;
	OVITO_ASSERT(posProperty->size() == mask.size());

	FloatType sliceWidth = 0;
	if(_widthCtrl) _widthCtrl->getValue(time, sliceWidth, validityInterval);
	sliceWidth *= 0.5;

	Plane3 plane = slicingPlane(time, validityInterval);

	size_t na = 0;
	auto m = mask.begin();
	const Point3* p = posProperty->constDataPoint3();
	const Point3* p_end = p + posProperty->size();
	const int* s = nullptr;
	if(selProperty) {
		OVITO_ASSERT(selProperty->size() == mask.size());
		s = selProperty->constDataInt();
	}

	if(sliceWidth <= 0) {
		for(; p != p_end; ++p, ++s, ++m) {
			if(plane.pointDistance(*p) > 0) {
				if(selProperty && !*s) continue;
				*m = true;
				na++;
			}
		}
	}
	else {
		for(; p != p_end; ++p, ++s, ++m) {
			if(inverse() == (plane.classifyPoint(*p, sliceWidth) == 0)) {
				if(selProperty && !*s) continue;
				*m = true;
				na++;
			}
		}
	}
	return na;
}

/******************************************************************************
* Lets the modifier render itself into the viewport.
******************************************************************************/
void SliceModifier::render(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay)
{
	if(!renderOverlay && isBeingEdited() && renderer->isInteractive())
		renderVisual(time, contextNode, renderer);
}

/******************************************************************************
* Computes the bounding box of the visual representation of the modifier.
******************************************************************************/
Box3 SliceModifier::boundingBox(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp)
{
	if(isBeingEdited())
		return renderVisual(time, contextNode, nullptr);
	else
		return Box3();
}

/******************************************************************************
* Renders the modifier's visual representation and computes its bounding box.
******************************************************************************/
Box3 SliceModifier::renderVisual(TimePoint time, ObjectNode* contextNode, SceneRenderer* renderer)
{
	TimeInterval interval;

	Box3 bb = contextNode->localBoundingBox(time);
	if(bb.isEmpty())
		return Box3();

	Plane3 plane = slicingPlane(time, interval);

	FloatType sliceWidth = 0;
	if(_widthCtrl) _widthCtrl->getValue(time, sliceWidth, interval);

	ColorA color(0.8f, 0.3f, 0.3f);
	if(sliceWidth <= 0) {
		return renderPlane(renderer, plane, bb, color);
	}
	else {
		plane.dist += sliceWidth / 2;
		Box3 box = renderPlane(renderer, plane, bb, color);
		plane.dist -= sliceWidth;
		box.addBox(renderPlane(renderer, plane, bb, color));
		return box;
	}
}

/******************************************************************************
* Renders the plane in the viewports.
******************************************************************************/
Box3 SliceModifier::renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& bb, const ColorA& color) const
{
	// Compute intersection lines of slicing plane and bounding box.
	QVector<Point3> vertices;
	Point3 corners[8];
	for(int i = 0; i < 8; i++)
		corners[i] = bb[i];

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
		for(int edge = 0; edge < 12; edge++) {
			vertices.push_back(plane.projectPoint(corners[edges[edge][0]]));
			vertices.push_back(plane.projectPoint(corners[edges[edge][1]]));
		}
	}

	if(renderer) {
		// Render plane-box intersection lines.
		OORef<LineGeometryBuffer> buffer = renderer->createLineGeometryBuffer();
		buffer->setSize(vertices.size());
		buffer->setVertexPositions(vertices.constData());
		buffer->setVertexColor(color);
		buffer->render(renderer);
	}

	// Compute bounding box.
	Box3 vertexBoundingBox;
	vertexBoundingBox.addPoints(vertices.constData(), vertices.size());
	return vertexBoundingBox;
}

/******************************************************************************
* Computes the intersection lines of a plane and a quad.
******************************************************************************/
void SliceModifier::planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, QVector<Point3>& vertices) const
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
void SliceModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Get the input simulation cell to initially place the slicing plane in
	// the center of the cell.
	PipelineFlowState input = pipeline->evaluatePipeline(AnimManager::instance().time(), modApp, false);
	SimulationCell* cell = input.findObject<SimulationCell>();
	if(cell) {
		Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
		FloatType centerDistance = normal().dot(centerPoint - Point3::Origin());
		if(fabs(centerDistance) > FLOATTYPE_EPSILON)
			setDistance(centerDistance);
	}
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SliceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Slice"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	// Distance parameter.
	FloatParameterUI* distancePUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::_distanceCtrl));
	gridlayout->addWidget(distancePUI->label(), 0, 0);
	gridlayout->addLayout(distancePUI->createFieldLayout(), 0, 1);

	// Normal parameter.
	for(int i = 0; i < 3; i++) {
		Vector3ParameterUI* normalPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SliceModifier::_normalCtrl), i);
		normalPUI->label()->setTextFormat(Qt::RichText);
		normalPUI->label()->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		normalPUI->label()->setText(tr("<a href=\"%1\">%2</a>").arg(i).arg(normalPUI->label()->text()));
		connect(normalPUI->label(), SIGNAL(linkActivated(const QString&)), this, SLOT(onXYZNormal(const QString&)));
		gridlayout->addWidget(normalPUI->label(), i+1, 0);
		gridlayout->addLayout(normalPUI->createFieldLayout(), i+1, 1);
	}

	// Slice width parameter.
	FloatParameterUI* widthPUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::_widthCtrl));
	gridlayout->addWidget(widthPUI->label(), 4, 0);
	gridlayout->addLayout(widthPUI->createFieldLayout(), 4, 1);
	widthPUI->setMinValue(0);

	layout->addLayout(gridlayout);
	layout->addSpacing(8);

	// Invert parameter.
	BooleanParameterUI* invertPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::_inverse));
	layout->addWidget(invertPUI->checkBox());

	// Create selection parameter.
	BooleanParameterUI* createSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::_createSelection));
	layout->addWidget(createSelectionPUI->checkBox());

	// Apply to selection only parameter.
	BooleanParameterUI* applyToSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::_applyToSelection));
	layout->addWidget(applyToSelectionPUI->checkBox());

	layout->addSpacing(8);
	QPushButton* centerPlaneBtn = new QPushButton(tr("Move plane to simulation box center"), rollout);
	connect(centerPlaneBtn, SIGNAL(clicked(bool)), this, SLOT(onCenterOfBox()));
	layout->addWidget(centerPlaneBtn);

	// Add buttons for view alignment functions.
	QPushButton* alignViewToPlaneBtn = new QPushButton(tr("Align view direction to plane normal"), rollout);
	connect(alignViewToPlaneBtn, SIGNAL(clicked(bool)), this, SLOT(onAlignViewToPlane()));
	layout->addWidget(alignViewToPlaneBtn);
	QPushButton* alignPlaneToViewBtn = new QPushButton(tr("Align plane normal to view direction"), rollout);
	connect(alignPlaneToViewBtn, SIGNAL(clicked(bool)), this, SLOT(onAlignPlaneToView()));
	layout->addWidget(alignPlaneToViewBtn);

	_pickParticlePlaneInputMode = new PickParticlePlaneInputMode(this);
	_pickParticlePlaneInputModeAction = new ViewportModeAction(tr("Pick three particles"), this, _pickParticlePlaneInputMode);
	layout->addWidget(_pickParticlePlaneInputModeAction->createPushButton());

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Aligns the normal of the slicing plane with the X, Y, or Z axis.
******************************************************************************/
void SliceModifierEditor::onXYZNormal(const QString& link)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;

	UndoableTransaction::handleExceptions(tr("Set plane normal"), [mod, &link]() {
		if(link == "0")
			mod->setNormal(Vector3(1,0,0));
		else if(link == "1")
			mod->setNormal(Vector3(0,1,0));
		else if(link == "2")
			mod->setNormal(Vector3(0,0,1));
	});
}

/******************************************************************************
* Aligns the slicing plane to the viewing direction.
******************************************************************************/
void SliceModifierEditor::onAlignPlaneToView()
{
	TimeInterval interval;

	Viewport* vp = ViewportManager::instance().activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	ObjectNode* node = dynamic_object_cast<ObjectNode>(DataSetManager::instance().currentSet()->selection()->firstNode());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(AnimManager::instance().time(), interval);

	// Get the base point of the current slicing plane in local coordinates.
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;
	Plane3 oldPlaneLocal = mod->slicingPlane(AnimManager::instance().time(), interval);
	Point3 basePoint = Point3::Origin() + oldPlaneLocal.normal * oldPlaneLocal.dist;

	// Get the orientation of the projection plane of the current viewport.
	Vector3 dirWorld = -vp->cameraDirection();
	Plane3 newPlaneLocal(basePoint, nodeTM.inverse() * dirWorld);
	if(std::abs(newPlaneLocal.normal.x()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.x() = 0;
	if(std::abs(newPlaneLocal.normal.y()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.y() = 0;
	if(std::abs(newPlaneLocal.normal.z()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.z() = 0;

	UndoableTransaction::handleExceptions(tr("Align plane to view"), [mod, &newPlaneLocal]() {
		mod->setNormal(newPlaneLocal.normal.normalized());
		mod->setDistance(newPlaneLocal.dist);
	});
}

/******************************************************************************
* Aligns the current viewing direction to the slicing plane.
******************************************************************************/
void SliceModifierEditor::onAlignViewToPlane()
{
	TimeInterval interval;

	Viewport* vp = ViewportManager::instance().activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	ObjectNode* node = dynamic_object_cast<ObjectNode>(DataSetManager::instance().currentSet()->selection()->firstNode());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(AnimManager::instance().time(), interval);

	// Transform the current slicing plane to the world coordinate system.
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;
	Plane3 planeLocal = mod->slicingPlane(AnimManager::instance().time(), interval);
	Plane3 planeWorld = nodeTM * planeLocal;

	// Calculate the intersection point of the current viewing direction with the current slicing plane.
	Ray3 viewportRay(vp->cameraPosition(), vp->cameraDirection());
	FloatType t = planeWorld.intersectionT(viewportRay);
	Point3 intersectionPoint;
	if(t != FLOATTYPE_MAX)
		intersectionPoint = viewportRay.point(t);
	else
		intersectionPoint = Point3::Origin() + nodeTM.translation();

	if(vp->isPerspectiveProjection()) {
		FloatType distance = (vp->cameraPosition() - intersectionPoint).length();
		vp->setViewType(Viewport::VIEW_PERSPECTIVE);
		vp->setCameraDirection(-planeWorld.normal);
		vp->setCameraPosition(intersectionPoint + planeWorld.normal * distance);
	}
	else {
		vp->setViewType(Viewport::VIEW_ORTHO);
		vp->setCameraDirection(-planeWorld.normal);
	}
	ActionManager::instance().invokeAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS);
}

/******************************************************************************
* Moves the plane to the center of the simulation box.
******************************************************************************/
void SliceModifierEditor::onCenterOfBox()
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;

	// Get the simulation cell from the input object to center the slicing plane in
	// the center of the simulation cell.
	PipelineFlowState input = mod->getModifierInput();
	SimulationCell* cell = input.findObject<SimulationCell>();
	if(!cell) return;

	Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
	FloatType centerDistance = mod->normal().dot(centerPoint - Point3::Origin());

	UndoableTransaction::handleExceptions(tr("Set plane position"), [mod, centerDistance]() {
		mod->setDistance(centerDistance);
	});
}

/******************************************************************************
* This is called by the system after the input handler has become the active handler.
******************************************************************************/
void PickParticlePlaneInputMode::activated()
{
	MainWindow::instance().statusBar()->showMessage(tr("Pick three particles to define a new slicing plane."));
}

/******************************************************************************
* This is called by the system after the input handler is no longer the active handler.
******************************************************************************/
void PickParticlePlaneInputMode::deactivated()
{
	_pickedParticles.clear();
	MainWindow::instance().statusBar()->clearMessage();
}

/******************************************************************************
* Handles the mouse events for a Viewport.
******************************************************************************/
void PickParticlePlaneInputMode::mouseReleaseEvent(Viewport* vp, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton && temporaryNavigationMode() == nullptr) {

		if(_pickedParticles.size() >= 3) {
			_pickedParticles.clear();
			ViewportManager::instance().updateViewports();
		}

		PickResult pickResult;
		if(pickParticle(vp, event->pos(), pickResult)) {

			// Do not select the same particle twice.
			bool ignore = false;
			if(_pickedParticles.size() >= 1 && _pickedParticles[0].worldPos.equals(pickResult.worldPos, FLOATTYPE_EPSILON)) ignore = true;
			if(_pickedParticles.size() >= 2 && _pickedParticles[1].worldPos.equals(pickResult.worldPos, FLOATTYPE_EPSILON)) ignore = true;

			if(!ignore) {
				_pickedParticles.push_back(pickResult);
				ViewportManager::instance().updateViewports();

				if(_pickedParticles.size() == 3) {

					// Get the slice modifier that is currently being edited.
					SliceModifier* mod = dynamic_object_cast<SliceModifier>(_editor->editObject());
					if(mod)
						alignPlane(mod);
					_pickedParticles.clear();
				}
			}
		}
	}

	ViewportInputHandler::mouseReleaseEvent(vp, event);
}

/******************************************************************************
* Aligns the modifier's slicing plane to the three selected particles.
******************************************************************************/
void PickParticlePlaneInputMode::alignPlane(SliceModifier* mod)
{
	OVITO_ASSERT(_pickedParticles.size() == 3);

	try {
		Plane3 worldPlane(_pickedParticles[0].worldPos, _pickedParticles[1].worldPos, _pickedParticles[2].worldPos, true);
		if(worldPlane.normal.equals(Vector3::Zero(), FLOATTYPE_EPSILON))
			throw Exception(tr("Cannot set the new slicing plane. The three selected particle are colinear."));

		// Get the object to world transformation for the currently selected node.
		ObjectNode* node = _pickedParticles[0].objNode.get();
		TimeInterval interval;
		const AffineTransformation& nodeTM = node->getWorldTransform(AnimManager::instance().time(), interval);

		// Transform new plane from world to object space.
		Plane3 localPlane = nodeTM.inverse() * worldPlane;

		// Flip new plane orientation if necessary to align it with old orientation.
		if(localPlane.normal.dot(mod->normal()) < 0)
			localPlane = -localPlane;

		localPlane.normalizePlane();
		UndoableTransaction::handleExceptions(tr("Align plane to particles"), [mod, &localPlane]() {
			mod->setNormal(localPlane.normal);
			mod->setDistance(localPlane.dist);
		});
	}
	catch(const Exception& ex) {
		ex.showError();
	}
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void PickParticlePlaneInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer, bool isActive)
{
	ViewportInputHandler::renderOverlay3D(vp, renderer, isActive);

	Q_FOREACH(const PickResult& pa, _pickedParticles) {
		renderSelectionMarker(vp, renderer, pa);
	}
}

};	// End of namespace
