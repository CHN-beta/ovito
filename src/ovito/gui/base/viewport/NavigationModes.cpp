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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/gui/base/mainwin/MainWindowInterface.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/data/camera/AbstractCameraObject.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSet.h>
#include "ViewportInputManager.h"
#include "NavigationModes.h"

namespace Ovito {

/******************************************************************************
* This is called by the system after the input handler has
* become the active handler.
******************************************************************************/
void NavigationMode::activated(bool temporaryActivation)
{
	_temporaryActivation = temporaryActivation;
	inputManager()->addViewportGizmo(inputManager()->pickOrbitCenterMode());
	ViewportInputMode::activated(temporaryActivation);
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void NavigationMode::deactivated(bool temporary)
{
	if(_viewport) {
		// Restore old settings if view change has not been committed.
		_viewport->setCameraTransformation(_oldCameraTM);
		_viewport->setFieldOfView(_oldFieldOfView);
		_viewport->dataset()->undoStack().endCompoundOperation(false);
		_viewport = nullptr;
	}
	inputManager()->removeViewportGizmo(inputManager()->pickOrbitCenterMode());
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Applies a step-wise change of the view orientation.
******************************************************************************/
void NavigationMode::discreteStep(ViewportWindowInterface* vpwin, QPointF delta)
{
	Viewport* vp = vpwin->viewport();
	if(_viewport == nullptr) {
		std::swap(_viewport, vp);
		_startPoint = QPointF(0,0);
		_oldCameraTM = _viewport->cameraTransformation();
		_oldCameraPosition = _viewport->cameraPosition();
		_oldCameraDirection = _viewport->cameraDirection();
		_oldFieldOfView = _viewport->fieldOfView();
		_oldViewMatrix = _viewport->projectionParams().viewMatrix;
		_oldInverseViewMatrix = _viewport->projectionParams().inverseViewMatrix;
		_currentOrbitCenter = _viewport->orbitCenter();
	}
	modifyView(vpwin, vpwin->viewport(), delta, true);
	std::swap(_viewport, vp);
}

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void NavigationMode::mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::RightButton) {
		ViewportInputMode::mousePressEvent(vpwin, event);
		return;
	}

	if(_viewport == nullptr) {
		_viewport = vpwin->viewport();
		_startPoint = getMousePosition(event);
		_oldCameraTM = _viewport->cameraTransformation();
		_oldCameraPosition = _viewport->cameraPosition();
		_oldCameraDirection = _viewport->cameraDirection();
		_oldFieldOfView = _viewport->fieldOfView();
		_oldViewMatrix = _viewport->projectionParams().viewMatrix;
		_oldInverseViewMatrix = _viewport->projectionParams().inverseViewMatrix;
		_currentOrbitCenter = _viewport->orbitCenter();
		_viewport->dataset()->undoStack().beginCompoundOperation(tr("Modify camera"));
	}
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void NavigationMode::mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(_viewport) {
		// Commit view change.
		_viewport->dataset()->undoStack().endCompoundOperation();
		_viewport = nullptr;

		if(_temporaryActivation)
			inputManager()->removeInputMode(this);
	}
}

/******************************************************************************
* Is called when a viewport looses the input focus.
******************************************************************************/
void NavigationMode::focusOutEvent(ViewportWindowInterface* vpwin, QFocusEvent* event)
{
	if(_viewport) {
		if(_temporaryActivation)
			inputManager()->removeInputMode(this);
	}
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void NavigationMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(_viewport == vpwin->viewport()) {
		QPointF pos = getMousePosition(event);

		_viewport->dataset()->undoStack().resetCurrentCompoundOperation();
		modifyView(vpwin, _viewport, pos - _startPoint, false);

		// Force immediate viewport repaint.
		_viewport->dataset()->viewportConfig()->processViewportUpdates();
	}
}

/******************************************************************************
* Returns the camera object associated with the given viewport.
******************************************************************************/
AbstractCameraObject* NavigationMode::getViewportCamera(Viewport* vp)
{
	if(vp->viewNode() && vp->viewType() == Viewport::VIEW_SCENENODE) {
		return dynamic_object_cast<AbstractCameraObject>(vp->viewNode()->pipelineSource());
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
////////////////////////////////// Pan Mode ///////////////////////////////////

/******************************************************************************
* Computes the new view matrix based on the new mouse position.
******************************************************************************/
void PanMode::modifyView(ViewportWindowInterface* vpwin, Viewport* vp, QPointF delta, bool discreteStep)
{
	FloatType scaling;
	FloatType normalization = discreteStep ? 20.0 : vpwin->viewportWindowDeviceIndependentSize().height();
	if(vp->isPerspectiveProjection())
		scaling = FloatType(10) * vp->nonScalingSize(_currentOrbitCenter) / normalization;
	else
		scaling = FloatType(2) * _oldFieldOfView / normalization;
	FloatType deltaX = -scaling * delta.x();
	FloatType deltaY =  scaling * delta.y();
	Vector3 displacement = _oldInverseViewMatrix * Vector3(deltaX, deltaY, 0);
	if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
		vp->setCameraPosition(_oldCameraPosition + displacement);
	}
	else {
		// Get parent's system.
		TimeInterval iv;
		const AffineTransformation& parentSys =
				vp->viewNode()->parentNode()->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);

		// Move node in parent's system.
		vp->viewNode()->transformationController()->translate(
				vp->dataset()->animationSettings()->time(), displacement, parentSys.inverse());

		// If it's a target camera, move target as well.
		if(vp->viewNode()->lookatTargetNode()) {
			vp->viewNode()->lookatTargetNode()->transformationController()->translate(
					vp->dataset()->animationSettings()->time(), displacement, parentSys.inverse());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Zoom Mode ///////////////////////////////////

/******************************************************************************
* Computes the new view matrix based on the new mouse position.
******************************************************************************/
void ZoomMode::modifyView(ViewportWindowInterface* vpwin, Viewport* vp, QPointF delta, bool discreteStep)
{
	if(vp->isPerspectiveProjection()) {
		FloatType amount =  FloatType(-5) * sceneSizeFactor(vp) * delta.y();
		if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
			vp->setCameraPosition(_oldCameraPosition + _oldCameraDirection.resized(amount));
		}
		else {
			TimeInterval iv;
			const AffineTransformation& sys = vp->viewNode()->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);
			vp->viewNode()->transformationController()->translate(
					vp->dataset()->animationSettings()->time(), Vector3(0,0,-amount), sys);
		}
	}
	else {

		FloatType oldFOV = _oldFieldOfView;
		if(AbstractCameraObject* cameraObj = getViewportCamera(vp)) {
			TimeInterval iv;
			oldFOV = cameraObj->fieldOfView(vp->dataset()->animationSettings()->time(), iv);
		}

		FloatType newFOV = oldFOV * (FloatType)exp(0.003 * delta.y());

		if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
			vp->setFieldOfView(newFOV);
		}
		else if(AbstractCameraObject* cameraObj = getViewportCamera(vp)) {
			cameraObj->setFieldOfView(vp->dataset()->animationSettings()->time(), newFOV);
		}
	}
}

/******************************************************************************
* Computes a scaling factor that depends on the total size of the scene
* which is used to control the zoom sensitivity in perspective mode.
******************************************************************************/
FloatType ZoomMode::sceneSizeFactor(Viewport* vp)
{
	OVITO_CHECK_OBJECT_POINTER(vp);
	Box3 sceneBoundingBox = vp->dataset()->sceneRoot()->worldBoundingBox(vp->dataset()->animationSettings()->time());
	if(!sceneBoundingBox.isEmpty())
		return sceneBoundingBox.size().length() * FloatType(5e-4);
	else
		return FloatType(0.1);
}

/******************************************************************************
* Zooms the viewport in or out.
******************************************************************************/
void ZoomMode::zoom(Viewport* vp, FloatType steps)
{
	if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
		if(vp->isPerspectiveProjection()) {
			vp->setCameraPosition(vp->cameraPosition() + vp->cameraDirection().resized(sceneSizeFactor(vp) * steps));
		}
		else {
			vp->setFieldOfView(vp->fieldOfView() * exp(-steps * FloatType(1e-3)));
		}
	}
	else {
		UndoableTransaction::handleExceptions(vp->dataset()->undoStack(), tr("Zoom viewport"), [this, steps, vp]() {
			if(vp->isPerspectiveProjection()) {
				FloatType amount = sceneSizeFactor(vp) * steps;
				TimeInterval iv;
				const AffineTransformation& sys = vp->viewNode()->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);
				vp->viewNode()->transformationController()->translate(vp->dataset()->animationSettings()->time(), Vector3(0,0,-amount), sys);
			}
			else {
				if(AbstractCameraObject* cameraObj = getViewportCamera(vp)) {
					TimeInterval iv;
					FloatType oldFOV = cameraObj->fieldOfView(vp->dataset()->animationSettings()->time(), iv);
					cameraObj->setFieldOfView(vp->dataset()->animationSettings()->time(), oldFOV * exp(-steps * FloatType(1e-3)));
				}
			}
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////// FOV Mode ///////////////////////////////////

/******************************************************************************
* Computes the new field of view based on the new mouse position.
******************************************************************************/
void FOVMode::modifyView(ViewportWindowInterface* vpwin, Viewport* vp, QPointF delta, bool discreteStep)
{
	FloatType oldFOV = _oldFieldOfView;
	if(AbstractCameraObject* cameraObj = getViewportCamera(vp)) {
		TimeInterval iv;
		oldFOV = cameraObj->fieldOfView(vp->dataset()->animationSettings()->time(), iv);
	}

	FloatType newFOV;
	if(vp->isPerspectiveProjection()) {
		newFOV = oldFOV + (FloatType)delta.y() * FloatType(2e-3);
		newFOV = std::max(newFOV, qDegreesToRadians(FloatType(5.0)));
		newFOV = std::min(newFOV, qDegreesToRadians(FloatType(170.0)));
	}
	else {
		newFOV = oldFOV * (FloatType)exp(FloatType(6e-3) * delta.y());
	}

	if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
		vp->setFieldOfView(newFOV);
	}
	else if(AbstractCameraObject* cameraObj = getViewportCamera(vp)) {
		cameraObj->setFieldOfView(vp->dataset()->animationSettings()->time(), newFOV);
	}
}

///////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Orbit Mode ///////////////////////////////////

/******************************************************************************
* Computes the new view matrix based on the new mouse position.
******************************************************************************/
void OrbitMode::modifyView(ViewportWindowInterface* vpwin, Viewport* vp, QPointF delta, bool discreteStep)
{
	if(vp->viewType() < Viewport::VIEW_ORTHO)
		vp->setViewType(Viewport::VIEW_ORTHO, true);

	FloatType speed = discreteStep ? 0.05 : (5.0 / vp->windowSize().height());
	FloatType deltaTheta = speed * delta.x();
	FloatType deltaPhi = -speed * delta.y();

	Vector3 t1 = _currentOrbitCenter - Point3::Origin();
	Vector3 t2 = (_oldViewMatrix * _currentOrbitCenter) - Point3::Origin();

	if(ViewportSettings::getSettings().constrainCameraRotation()) {
		const Matrix3& coordSys = ViewportSettings::getSettings().coordinateSystemOrientation();
		Vector3 v = _oldViewMatrix * coordSys.column(2);

		FloatType theta, phi;
		if(v.x() == 0 && v.y() == 0)
			theta = FLOATTYPE_PI;
		else
			theta = atan2(v.x(), v.y());
		phi = atan2(sqrt(v.x() * v.x() + v.y() * v.y()), v.z());

		// Restrict rotation to keep the major axis pointing upward (prevent camera from becoming upside down).
		if(phi + deltaPhi < FLOATTYPE_EPSILON)
			deltaPhi = -phi + FLOATTYPE_EPSILON;
		else if(phi + deltaPhi > FLOATTYPE_PI - FLOATTYPE_EPSILON)
			deltaPhi = FLOATTYPE_PI - FLOATTYPE_EPSILON - phi;

		if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
			AffineTransformation newTM =
					AffineTransformation::translation(t1) *
					AffineTransformation::rotation(Rotation(ViewportSettings::getSettings().upVector(), -deltaTheta)) *
					AffineTransformation::translation(-t1) * _oldInverseViewMatrix *
					AffineTransformation::translation(t2) *
					AffineTransformation::rotationX(deltaPhi) *
					AffineTransformation::translation(-t2);
			newTM.orthonormalize();
			vp->setCameraTransformation(newTM);
		}
		else {
			Controller* ctrl = vp->viewNode()->transformationController();
			TimePoint time = vp->dataset()->animationSettings()->time();
			Rotation rotX(Vector3(1,0,0), deltaPhi, false);
			ctrl->rotate(time, rotX, _oldInverseViewMatrix);
			Rotation rotZ(ViewportSettings::getSettings().upVector(), -deltaTheta);
			ctrl->rotate(time, rotZ, AffineTransformation::Identity());
			Vector3 shiftVector = _oldInverseViewMatrix.translation() - (_currentOrbitCenter - Point3::Origin());
			Vector3 translationZ = (Matrix3::rotation(rotZ) * shiftVector) - shiftVector;
			Vector3 translationX = Matrix3::rotation(rotZ) * _oldInverseViewMatrix * ((Matrix3::rotation(rotX) * t2) - t2);
			ctrl->translate(time, translationZ - translationX, AffineTransformation::Identity());
		}
	}
	else {
		if(vp->viewNode() == nullptr || vp->viewType() != Viewport::VIEW_SCENENODE) {
			AffineTransformation newTM = _oldInverseViewMatrix *
					AffineTransformation::translation(t2) *
					AffineTransformation::rotationY(-deltaTheta) *
					AffineTransformation::rotationX(deltaPhi) *
					AffineTransformation::translation(-t2);
			newTM.orthonormalize();
			vp->setCameraTransformation(newTM);
		}
		else {
			Controller* ctrl = vp->viewNode()->transformationController();
			TimePoint time = vp->dataset()->animationSettings()->time();
			Rotation rot = Rotation(Vector3(0,1,0), -deltaTheta, false) * Rotation(Vector3(1,0,0), deltaPhi, false);
			ctrl->rotate(time, rot, _oldInverseViewMatrix);
			Vector3 translation = t2 - (Matrix3::rotation(rot) * t2);
			ctrl->translate(time, translation, _oldInverseViewMatrix);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Pick Orbit Center Mode ////////////////////////////////

/******************************************************************************
* Sets the orbit rotation center to the space location under given mouse coordinates.
******************************************************************************/
bool PickOrbitCenterMode::pickOrbitCenter(ViewportWindowInterface* vpwin, const QPointF& pos)
{
	Point3 p;
	Viewport* vp = vpwin->viewport();
	if(findIntersection(vpwin, pos, p)) {
		vp->dataset()->viewportConfig()->setOrbitCenterMode(ViewportConfiguration::ORBIT_USER_DEFINED);
		vp->dataset()->viewportConfig()->setUserOrbitCenter(p);
		return true;
	}
	else {
		vp->dataset()->viewportConfig()->setOrbitCenterMode(ViewportConfiguration::ORBIT_SELECTION_CENTER);
		vp->dataset()->viewportConfig()->setUserOrbitCenter(Point3::Origin());
		if(MainWindowInterface* mainWindow = vpwin->mainWindow())
			mainWindow->showStatusBarMessage(tr("No object has been picked. Resetting orbit center to default position."), 1200);
		return false;
	}
}

/******************************************************************************
* Handles the mouse down events for a Viewport.
******************************************************************************/
void PickOrbitCenterMode::mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(pickOrbitCenter(vpwin, getMousePosition(event)))
			return;
	}
	ViewportInputMode::mousePressEvent(vpwin, event);
}

/******************************************************************************
* Is called when the user moves the mouse while the operation is not active.
******************************************************************************/
void PickOrbitCenterMode::mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event)
{
	ViewportInputMode::mouseMoveEvent(vpwin, event);

	Point3 p;
	bool isOverObject = findIntersection(vpwin, getMousePosition(event), p);

	if(!isOverObject && _showCursor) {
		_showCursor = false;
		setCursor(QCursor());
	}
	else if(isOverObject && !_showCursor) {
		_showCursor = true;
		setCursor(_hoverCursor);
	}
}

/******************************************************************************
* Finds the closest intersection point between a ray originating from the
* current mouse cursor position and the whole scene.
******************************************************************************/
bool PickOrbitCenterMode::findIntersection(ViewportWindowInterface* vpwin, const QPointF& mousePos, Point3& intersectionPoint)
{
	ViewportPickResult pickResult = vpwin->pick(mousePos);
	if(pickResult.isValid()) {
		intersectionPoint = pickResult.hitLocation();
		return true;
	}
	return false;
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void PickOrbitCenterMode::renderOverlay3D(Viewport* vp, SceneRenderer* renderer)
{
	if(renderer->isPicking())
		return;

	// Render center of rotation.
	Point3 center = vp->dataset()->viewportConfig()->orbitCenter();
	FloatType symbolSize = vp->nonScalingSize(center);
	renderer->setWorldTransform(AffineTransformation::translation(center - Point3::Origin()) * AffineTransformation::scaling(symbolSize));

	if(!renderer->isBoundingBoxPass()) {
		// Create line buffer.
		if(!_orbitCenterMarker) {
			DataBufferAccessAndRef<Point3> basePositions = DataBufferPtr::create(vp->dataset(), ExecutionContext::Scripting, 3, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Point3> headPositions = DataBufferPtr::create(vp->dataset(), ExecutionContext::Scripting, 3, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Color> colors = DataBufferPtr::create(vp->dataset(), ExecutionContext::Scripting, 3, DataBuffer::Float, 3, 0, false);
			basePositions[0] = Point3(-1,0,0); headPositions[0] = Point3(1,0,0); colors[0] = Color(1,0,0);
			basePositions[1] = Point3(0,-1,0); headPositions[1] = Point3(0,1,0); colors[1] = Color(0,1,0);
			basePositions[2] = Point3(0,0,-1); headPositions[2] = Point3(0,0,1); colors[2] = Color(0.4,0.4,1);
			_orbitCenterMarker = renderer->createCylinderPrimitive(CylinderPrimitive::CylinderShape, CylinderPrimitive::NormalShading, CylinderPrimitive::HighQuality);
			_orbitCenterMarker->setUniformRadius(0.05);
			_orbitCenterMarker->setPositions(basePositions.take(), headPositions.take());
			_orbitCenterMarker->setColors(colors.take());
		}
		renderer->renderCylinders(_orbitCenterMarker);
	}
	else {
		// Add marker to bounding box.
		renderer->addToLocalBoundingBox(Box3(Point3::Origin(), symbolSize));
	}
}

}	// End of namespace
