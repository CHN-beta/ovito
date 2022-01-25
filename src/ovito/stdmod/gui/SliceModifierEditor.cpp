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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/ViewportWindowInterface.h>
#include <ovito/core/rendering/MarkerPrimitive.h>
#include <ovito/core/rendering/LinePrimitive.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/data/mesh/TriMeshObject.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/widgets/general/ViewportModeButton.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/Vector3ParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/ModifierDelegateFixedListParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include <ovito/gui/base/actions/ViewportModeAction.h>
#include "SliceModifierEditor.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(SliceModifierEditor);
SET_OVITO_OBJECT_EDITOR(SliceModifier, SliceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
*******************************************************************************/
void SliceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Slice"), rolloutParams, "manual:particles.modifiers.slice");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);

	_reducedCoordinatesPUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(SliceModifier::reducedCoordinates));
	_reducedCoordinatesPUI->buttonFalse()->setText(tr("Cartesian coordinates"));
	_reducedCoordinatesPUI->buttonTrue()->setText(tr("Miller indices"));
	sublayout->addWidget(_reducedCoordinatesPUI->buttonFalse(), 1);
	sublayout->addWidget(_reducedCoordinatesPUI->buttonTrue(), 1);
	layout->addLayout(sublayout);
	connect(_reducedCoordinatesPUI, &BooleanRadioButtonParameterUI::valueEntered, this, &SliceModifierEditor::onCoordinateTypeChanged);
#ifdef OVITO_BUILD_BASIC
	_reducedCoordinatesPUI->setEnabled(false);
	_reducedCoordinatesPUI->buttonFalse()->setText(tr("Cartesian"));
	_reducedCoordinatesPUI->buttonTrue()->setText(_reducedCoordinatesPUI->buttonTrue()->text() + tr(" (OVITO Pro)"));
#endif

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	// Distance parameter.
	_distancePUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::distanceController));
	gridlayout->addWidget(_distancePUI->label(), 2, 0);
	gridlayout->addLayout(_distancePUI->createFieldLayout(), 2, 1);

	// Normal parameter.
	for(int i = 0; i < 3; i++) {
		_normalPUI[i] = new Vector3ParameterUI(this, PROPERTY_FIELD(SliceModifier::normalController), i);
		_normalPUI[i]->label()->setTextFormat(Qt::RichText);
		_normalPUI[i]->label()->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		connect(_normalPUI[i]->label(), &QLabel::linkActivated, this, &SliceModifierEditor::onAlignNormalWithAxis);
		gridlayout->addWidget(_normalPUI[i]->label(), i + 3, 0);
		gridlayout->addLayout(_normalPUI[i]->createFieldLayout(), i + 3, 1);
	}
	connect(_reducedCoordinatesPUI->buttonFalse(), &QAbstractButton::toggled, this, &SliceModifierEditor::updateCoordinateLabels);
	updateCoordinateLabels();

	// Slice width parameter.
	FloatParameterUI* widthPUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::widthController));
	gridlayout->addWidget(widthPUI->label(), 6, 0);
	gridlayout->addLayout(widthPUI->createFieldLayout(), 6, 1);

	layout->addLayout(gridlayout);
	layout->addSpacing(8);

	// Invert parameter.
	BooleanParameterUI* invertPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::inverse));
	layout->addWidget(invertPUI->checkBox());

	// Create selection parameter.
	BooleanParameterUI* createSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::createSelection));
	layout->addWidget(createSelectionPUI->checkBox());

	// Apply to selection only parameter.
	BooleanParameterUI* applyToSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::applyToSelection));
	layout->addWidget(applyToSelectionPUI->checkBox());

	// Visualize plane.
	BooleanParameterUI* visualizePlanePUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::enablePlaneVisualization));
	layout->addWidget(visualizePlanePUI->checkBox());

	layout->addSpacing(8);
	QPushButton* centerPlaneBtn = new QPushButton(tr("Center in simulation cell"), rollout);
	connect(centerPlaneBtn, &QPushButton::clicked, this, &SliceModifierEditor::onCenterOfBox);
	layout->addWidget(centerPlaneBtn);

	// Add buttons for view alignment functions.
	QPushButton* alignViewToPlaneBtn = new QPushButton(tr("Align view to plane"), rollout);
	connect(alignViewToPlaneBtn, &QPushButton::clicked, this, &SliceModifierEditor::onAlignViewToPlane);
	layout->addWidget(alignViewToPlaneBtn);
	QPushButton* alignPlaneToViewBtn = new QPushButton(tr("Align plane to view"), rollout);
	connect(alignPlaneToViewBtn, &QPushButton::clicked, this, &SliceModifierEditor::onAlignPlaneToView);
	layout->addWidget(alignPlaneToViewBtn);

	_pickPlanePointsInputMode = new PickPlanePointsInputMode(this);
	connect(this, &QObject::destroyed, _pickPlanePointsInputMode, &ViewportInputMode::removeMode);
	_pickPlanePointsInputModeAction = new ViewportModeAction(mainWindow(), tr("Pick three points"), this, _pickPlanePointsInputMode);
	layout->addWidget(new ViewportModeButton(_pickPlanePointsInputModeAction));

	// Deactivate input mode when editor is reset.
	connect(this, &PropertiesEditor::contentsReplaced, _pickPlanePointsInputModeAction, &ViewportModeAction::deactivateMode);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget((new ObjectStatusDisplay(this))->statusWidget());

	// Create a second rollout.
	rollout = createRollout(tr("Operate on"), rolloutParams.after(rollout), "manual:particles.modifiers.slice");

	// Create the rollout contents.
	layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ModifierDelegateFixedListParameterUI* delegatesPUI = new ModifierDelegateFixedListParameterUI(this, rolloutParams.after(rollout));
	layout->addWidget(delegatesPUI->listWidget());
}

/******************************************************************************
* Is called when the selected type of plane normal coordinates have changed.
******************************************************************************/
void SliceModifierEditor::updateCoordinateLabels()
{
	static const QString cartesianAxesNames[3] = { QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z") };
	static const QString millerAxesNames[3] = { QStringLiteral("h"), QStringLiteral("k"), QStringLiteral("l") };

	for(int i = 0; i < 3; i++) {
		const QString& axisName = 
			_reducedCoordinatesPUI->buttonFalse()->isChecked() ? 
			cartesianAxesNames[i] : millerAxesNames[i];
		_normalPUI[i]->label()->setText(QStringLiteral("<a href=\"%1\">Normal (%2)</a>").arg(i).arg(axisName));
		_normalPUI[i]->label()->setToolTip(tr("Click here to align plane normal with %1 axis").arg(axisName));
	}

	_distancePUI->label()->setText(_reducedCoordinatesPUI->buttonFalse()->isChecked()
		? tr("Distance:")
		: tr("<html>Distance [d<sub>hkl</sub>]:</html>"));
}

/******************************************************************************
* Is called when the user switches between Cartesian and reduced cell coordinates.
******************************************************************************/
void SliceModifierEditor::onCoordinateTypeChanged()
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;

	const PipelineFlowState& input = getPipelineInput();
	const SimulationCellObject* cell = input.getObject<SimulationCellObject>();
	if(!cell) return;

	// Get the plane info.
	Plane3 plane;
	TimeInterval validityInterval;
	if(mod->normalController())
		mod->normalController()->getVector3Value(mod->dataset()->animationSettings()->time(), plane.normal, validityInterval);
	if(mod->distanceController())
		plane.dist = mod->distanceController()->getFloatValue(mod->dataset()->animationSettings()->time(), validityInterval);

	// Automatically convert current plane equation to/from reduced coordinates.
	if(mod->reducedCoordinates()) {
		plane.normal.normalizeSafely();
		plane = cell->reciprocalCellMatrix() * plane;
	}
	else {
		FloatType lengthSq = plane.normal.squaredLength();
		if(lengthSq != 0)
			plane.normal /= lengthSq;
		plane = cell->cellMatrix() * plane;
	}

	mod->setNormal(plane.normal);
	mod->setDistance(plane.dist);
}

/******************************************************************************
* Aligns the normal of the slicing plane with one of the coordinate axes.
******************************************************************************/
void SliceModifierEditor::onAlignNormalWithAxis(const QString& link)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Set plane normal"), [mod, &link]() {
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

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	PipelineSceneNode* node = dynamic_object_cast<PipelineSceneNode>(dataset()->selection()->firstNode());
	if(!node) return;
	TimePoint time = dataset()->animationSettings()->time();
	const AffineTransformation& nodeTM = node->getWorldTransform(time, interval);

	undoableTransaction(tr("Align plane to view"), [&]() {

		// Get the base point of the current slicing plane in local coordinates.
		SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
		if(!mod) return;

		const PipelineFlowState& input = getPipelineInput();

		Plane3 oldPlaneLocal = std::get<Plane3>(mod->slicingPlane(time, interval, input));
		Point3 basePoint = Point3::Origin() + oldPlaneLocal.normal * oldPlaneLocal.dist;

		// Get the orientation of the projection plane of the current viewport.
		Vector3 dirWorld = -vp->cameraDirection();
		Plane3 newPlaneLocal(basePoint, nodeTM.inverse() * dirWorld);

		// Convert to reduced cell coordinates if requested.
		if(mod->reducedCoordinates()) {
			if(const SimulationCellObject* cell = input.getObject<SimulationCellObject>()) {
				newPlaneLocal = cell->inverseMatrix() * newPlaneLocal;
			}
		}

		// Perform rounding of almost axis-aligned normal vectors.
		if(std::abs(newPlaneLocal.normal.x()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.x() = 0;
		if(std::abs(newPlaneLocal.normal.y()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.y() = 0;
		if(std::abs(newPlaneLocal.normal.z()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.z() = 0;

		mod->setNormal(newPlaneLocal.normal.normalized());
		mod->setDistance(newPlaneLocal.dist);
	});
}

/******************************************************************************
* Aligns the current viewing direction to the slicing plane.
******************************************************************************/
void SliceModifierEditor::onAlignViewToPlane()
{
	try {
		TimeInterval interval;

		Viewport* vp = dataset()->viewportConfig()->activeViewport();
		if(!vp) return;

		// Get the object to world transformation for the currently selected object
		PipelineSceneNode* node = dynamic_object_cast<PipelineSceneNode>(dataset()->selection()->firstNode());
		if(!node) return;
		TimePoint time = dataset()->animationSettings()->time();
		const AffineTransformation& nodeTM = node->getWorldTransform(time, interval);

		// Transform the current slicing plane to the world coordinate system.
		SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
		if(!mod) return;
		Plane3 planeLocal = std::get<Plane3>(mod->slicingPlane(time, interval, getPipelineInput()));
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

		vp->zoomToSelectionExtents();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Moves the plane to the center of the simulation box.
******************************************************************************/
void SliceModifierEditor::onCenterOfBox()
{
	if(SliceModifier* mod = static_object_cast<SliceModifier>(editObject())) {
		undoableTransaction(tr("Center plane in box"), [&]() {
			mod->centerPlaneInSimulationCell(modifierApplication());
		});
	}
}

/******************************************************************************
* This is called by the system after the input handler has become the active handler.
******************************************************************************/
void PickPlanePointsInputMode::activated(bool temporary)
{
	ViewportInputMode::activated(temporary);
	inputManager()->userInterface().showStatusBarMessage(tr("Pick three points to define a new slicing plane."));
	if(!temporary)
		_numPickedPoints = 0;
	inputManager()->addViewportGizmo(this);
}

/******************************************************************************
* This is called by the system after the input handler is no longer the active handler.
******************************************************************************/
void PickPlanePointsInputMode::deactivated(bool temporary)
{
	if(!temporary) {
		_numPickedPoints = 0;
		_hasPreliminaryPoint = false;
	}
	inputManager()->userInterface().clearStatusBarMessage();
	inputManager()->removeViewportGizmo(this);
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse events for a Viewport.
******************************************************************************/
void PickPlanePointsInputMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	ViewportInputMode::mouseMoveEvent(vpwin, event);

	ViewportPickResult pickResult = vpwin->pick(getMousePosition(event));
	setCursor(pickResult.isValid() ? SelectionMode::selectionCursor() : QCursor());
	if(pickResult.isValid() && _numPickedPoints < 3) {
		_pickedPoints[_numPickedPoints] = pickResult.hitLocation();
		_hasPreliminaryPoint = true;
		requestViewportUpdate();
	}
	else {
		if(_hasPreliminaryPoint)
			requestViewportUpdate();
		_hasPreliminaryPoint = false;
	}
}

/******************************************************************************
* Handles the mouse events for a Viewport.
******************************************************************************/
void PickPlanePointsInputMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {

		if(_numPickedPoints >= 3) {
			_numPickedPoints = 0;
			requestViewportUpdate();
		}

		ViewportPickResult pickResult = vpwin->pick(getMousePosition(event));
		if(pickResult.isValid()) {

			// Do not select the same point twice.
			bool ignore = false;
			if(_numPickedPoints >= 1 && _pickedPoints[0].equals(pickResult.hitLocation(), FLOATTYPE_EPSILON)) ignore = true;
			if(_numPickedPoints >= 2 && _pickedPoints[1].equals(pickResult.hitLocation(), FLOATTYPE_EPSILON)) ignore = true;

			if(!ignore) {
				_pickedPoints[_numPickedPoints] = pickResult.hitLocation();
				_numPickedPoints++;
				_hasPreliminaryPoint = false;
				requestViewportUpdate();

				if(_numPickedPoints == 3) {

					// Get the slice modifier that is currently being edited.
					SliceModifier* mod = dynamic_object_cast<SliceModifier>(_editor->editObject());
					if(mod)
						alignPlane(mod);
					_numPickedPoints = 0;
				}
			}
		}
	}

	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Aligns the modifier's slicing plane to the three selected particles.
******************************************************************************/
void PickPlanePointsInputMode::alignPlane(SliceModifier* mod)
{
	OVITO_ASSERT(_numPickedPoints == 3);

	try {
		Plane3 worldPlane(_pickedPoints[0], _pickedPoints[1], _pickedPoints[2], true);
		if(worldPlane.normal.equals(Vector3::Zero(), FLOATTYPE_EPSILON))
			mod->throwException(tr("Cannot set the new slicing plane. The three selected points are colinear."));

		// Get the object-to-world transformation for the currently selected object.
		ModifierApplication* modApp = mod->someModifierApplication();
		if(!modApp) return;
		QSet<PipelineSceneNode*> nodes = modApp->pipelines(true);
		if(nodes.empty()) return;
		PipelineSceneNode* node = *nodes.begin();
		TimeInterval interval;
		const AffineTransformation& nodeTM = node->getWorldTransform(mod->dataset()->animationSettings()->time(), interval);

		// Transform new plane from world to object space.
		Plane3 localPlane = nodeTM.inverse() * worldPlane;

		// Convert to reduced cell coordinates if requested.
		if(mod->reducedCoordinates()) {
			const PipelineFlowState& input = _editor->getPipelineInput();
			if(const SimulationCellObject* cell = input.getObject<SimulationCellObject>()) {
				localPlane = cell->inverseMatrix() * localPlane;
			}
		}
		else {
			localPlane.normalizePlane();
		}

		// Flip new plane orientation if necessary to align it with old orientation.
		if(localPlane.normal.dot(mod->normal()) < 0)
			localPlane = -localPlane;

		UndoableTransaction::handleExceptions(mod->dataset()->undoStack(), tr("Align plane to points"), [mod, &localPlane]() {
			mod->setNormal(localPlane.normal);
			mod->setDistance(localPlane.dist);
		});
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void PickPlanePointsInputMode::renderOverlay3D(Viewport* vp, SceneRenderer* renderer)
{
	if(renderer->isPicking())
		return;

	int npoints = _numPickedPoints;
	if(_hasPreliminaryPoint && npoints < 3) npoints++;
	if(_numPickedPoints == 0)
		return;

	renderer->setWorldTransform(AffineTransformation::Identity());
	if(!renderer->isBoundingBoxPass()) {
		MarkerPrimitive markers(MarkerPrimitive::BoxShape);
		markers.setPositions(vp->dataset(), _pickedPoints, _pickedPoints + npoints);
		markers.setColor(ColorA(1, 1, 1));
		renderer->renderMarkers(markers);

		if(npoints == 2) {
			LinePrimitive lines;
			lines.setPositions(vp->dataset(), _pickedPoints, _pickedPoints + 2);
			lines.setUniformColor(ColorA(1, 1, 1));
			renderer->renderLines(lines);
		}
		else if(npoints == 3) {
			DataOORef<TriMeshObject> tri = DataOORef<TriMeshObject>::create(renderer->dataset(), ObjectCreationParams::WithoutVisElement);
			tri->setVertexCount(3);
			tri->setVertex(0, _pickedPoints[0]);
			tri->setVertex(1, _pickedPoints[1]);
			tri->setVertex(2, _pickedPoints[2]);
			tri->addFace().setVertices(0, 1, 2);
			MeshPrimitive meshPrimitive;
			meshPrimitive.setMesh(std::move(tri), MeshPrimitive::ConvexShapeMode);
			meshPrimitive.setUniformColor(ColorA(0.7, 0.7, 1.0, 0.5));
			renderer->renderMesh(meshPrimitive);

			LinePrimitive lines;
			const Point3 vertices[6] = { _pickedPoints[0], _pickedPoints[1], _pickedPoints[1], _pickedPoints[2], _pickedPoints[2], _pickedPoints[0] };
			lines.setPositions(vp->dataset(), vertices);
			lines.setUniformColor(ColorA(1, 1, 1));
			renderer->renderLines(lines);
		}
	}
	else {
		for(int i = 0; i < npoints; i++)
			renderer->addToLocalBoundingBox(_pickedPoints[i]);
	}
}

}	// End of namespace
