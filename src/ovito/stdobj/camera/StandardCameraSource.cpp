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
#include <ovito/core/app/Application.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/DataCollection.h>
#include <ovito/core/dataset/pipeline/StaticSource.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/stdobj/camera/TargetObject.h>
#include "StandardCameraSource.h"
#include "StandardCameraObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(StandardCameraSource);
DEFINE_PROPERTY_FIELD(StandardCameraSource, isPerspective);
DEFINE_REFERENCE_FIELD(StandardCameraSource, fovController);
DEFINE_REFERENCE_FIELD(StandardCameraSource, zoomController);
SET_PROPERTY_FIELD_LABEL(StandardCameraSource, isPerspective, "Perspective projection");
SET_PROPERTY_FIELD_LABEL(StandardCameraSource, fovController, "FOV angle");
SET_PROPERTY_FIELD_LABEL(StandardCameraSource, zoomController, "FOV size");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(StandardCameraSource, fovController, AngleParameterUnit, FloatType(1e-3), FLOATTYPE_PI - FloatType(1e-2));
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(StandardCameraSource, zoomController, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a camera object.
******************************************************************************/
StandardCameraSource::StandardCameraSource(DataSet* dataset) : PipelineObject(dataset), _isPerspective(true)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void StandardCameraSource::initializeObject(ExecutionContext executionContext)
{
	setFovController(ControllerManager::createFloatController(dataset(), executionContext));
	fovController()->setFloatValue(0, FLOATTYPE_PI/4);

	setZoomController(ControllerManager::createFloatController(dataset(), executionContext));
	zoomController()->setFloatValue(0, 200);

	// Adopt the view parameters from the currently active Viewport.
	if(executionContext == ExecutionContext::Interactive) {
		if(Viewport* vp = dataset()->viewportConfig()->activeViewport()) {
			setIsPerspective(vp->isPerspectiveProjection());
			if(vp->isPerspectiveProjection())
				fovController()->setFloatValue(0, vp->fieldOfView());
			else
				zoomController()->setFloatValue(0, vp->fieldOfView());
		}
	}

	PipelineObject::initializeObject(executionContext);
}

/******************************************************************************
* Asks the object for its validity interval at the given time.
******************************************************************************/
TimeInterval StandardCameraSource::validityInterval(const PipelineEvaluationRequest& request) const
{
	TimeInterval interval = PipelineObject::validityInterval(request);
	if(isPerspective() && fovController()) 
		interval.intersect(fovController()->validityInterval(request.time()));
	if(!isPerspective() && zoomController()) 
		interval.intersect(zoomController()->validityInterval(request.time()));
	return interval;
}

/******************************************************************************
* Asks the pipeline stage to compute the results.
******************************************************************************/
PipelineFlowState StandardCameraSource::evaluateSynchronous(TimePoint time)
{
	// Create a new DataCollection.
	DataOORef<DataCollection> data = DataOORef<DataCollection>::create(dataset(), Application::instance()->executionContext());

	// Set up the camera data object.
	DataOORef<StandardCameraObject> camera = DataOORef<StandardCameraObject>::create(dataset(), Application::instance()->executionContext());
	camera->setDataSource(this);
	TimeInterval stateValidity = TimeInterval::infinite();
	camera->setIsPerspective(isPerspective());
	if(fovController()) camera->setFov(fovController()->getFloatValue(time, stateValidity));
	if(zoomController()) camera->setZoom(zoomController()->getFloatValue(time, stateValidity));
	data->addObject(std::move(camera));

	// Wrap the DataCollection in a PipelineFlowState.
	return PipelineFlowState(std::move(data), PipelineStatus::Success, stateValidity);
}

/******************************************************************************
* Returns whether this camera is a target camera directed at a target object.
******************************************************************************/
bool StandardCameraSource::isTargetCamera() const
{
	for(PipelineSceneNode* pipeline : pipelines(true)) {
		if(pipeline->lookatTargetNode() != nullptr)
			return true;
	}
	return false;
}

/******************************************************************************
* For a target camera, queries the distance between the camera and its target.
******************************************************************************/
FloatType StandardCameraSource::targetDistance() const
{
	for(PipelineSceneNode* node : pipelines(true)) {
		if(node->lookatTargetNode() != nullptr) {
			return StandardCameraObject::getTargetDistance(dataset()->animationSettings()->time(), node);
		}
	}

	return StandardCameraObject::getTargetDistance(dataset()->animationSettings()->time(), nullptr);
}

/******************************************************************************
* Changes the type of the camera to a target camera or a free camera.
******************************************************************************/
void StandardCameraSource::setIsTargetCamera(bool enable)
{
	dataset()->undoStack().pushIfRecording<TargetChangedUndoOperation>(this);

	for(PipelineSceneNode* node : pipelines(true)) {
		if(node->lookatTargetNode() == nullptr && enable) {
			if(SceneNode* parentNode = node->parentNode()) {
				AnimationSuspender noAnim(this);
				DataOORef<DataCollection> dataCollection = DataOORef<DataCollection>::create(dataset(), Application::instance()->executionContext());
				dataCollection->addObject(DataOORef<TargetObject>::create(dataset(), Application::instance()->executionContext()));
				OORef<StaticSource> targetSource = OORef<StaticSource>::create(dataset(), Application::instance()->executionContext(), dataCollection);
				OORef<PipelineSceneNode> targetNode = OORef<PipelineSceneNode>::create(dataset(), Application::instance()->executionContext());
				targetNode->setDataProvider(targetSource);
				targetNode->setNodeName(tr("%1.target").arg(node->nodeName()));
				parentNode->addChildNode(targetNode);
				// Position the new target to match the current orientation of the camera.
				TimeInterval iv;
				const AffineTransformation& cameraTM = node->getWorldTransform(dataset()->animationSettings()->time(), iv);
				Vector3 cameraPos = cameraTM.translation();
				Vector3 cameraDir = cameraTM.column(2).normalized();
				Vector3 targetPos = cameraPos - targetDistance() * cameraDir;
				targetNode->transformationController()->translate(0, targetPos, AffineTransformation::Identity());
				node->setLookatTargetNode(targetNode);
			}
		}
		else if(node->lookatTargetNode() != nullptr && !enable) {
			node->lookatTargetNode()->deleteNode();
			OVITO_ASSERT(node->lookatTargetNode() == nullptr);
		}
	}

	dataset()->undoStack().pushIfRecording<TargetChangedRedoOperation>(this);
	notifyTargetChanged();
}

}	// End of namespace
}	// End of namespace