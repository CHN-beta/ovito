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

#pragma once


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/dataset/animation/controller/Controller.h>

namespace Ovito { namespace StdObj {

/**
 * A pipeline source generating a StandardCameraObject.
 */
class OVITO_STDOBJ_EXPORT StandardCameraSource : public PipelineObject
{
	Q_OBJECT
	OVITO_CLASS(StandardCameraSource)
	Q_CLASSINFO("DisplayName", "Camera");

public:

	/// Constructor.
	Q_INVOKABLE StandardCameraSource(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;
	
	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request) const override;

	/// Asks the pipeline stage to compute the results.
	virtual SharedFuture<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request) override {
		return evaluateSynchronous(request.time());
	}

	/// Asks the pipeline stage to compute the preliminary results in a synchronous fashion.
	virtual PipelineFlowState evaluateSynchronous(TimePoint time) override;
	
	/// Returns whether this camera is a target camera directed at a target object.
	bool isTargetCamera() const;

	/// Changes the type of the camera to a target camera or a free camera.
	void setIsTargetCamera(bool enable);

	/// For a target camera, queries the distance between the camera and its target.
	FloatType targetDistance() const;

public:

	Q_PROPERTY(bool isTargetCamera READ isTargetCamera WRITE setIsTargetCamera);
	Q_PROPERTY(bool isPerspective READ isPerspective WRITE setIsPerspective);

private:

	/// Determines if this camera uses a perspective projection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isPerspective, setIsPerspective);

	/// This controller stores the field of view of the camera if it uses a perspective projection.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<Controller>, fovController, setFovController);

	/// This controller stores the field of view of the camera if it uses an orthogonal projection.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<Controller>, zoomController, setZoomController);
};

}	// End of namespace
}	// End of namespace
