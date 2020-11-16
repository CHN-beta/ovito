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
#include <ovito/core/dataset/data/camera/AbstractCameraObject.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/rendering/LinePrimitive.h>

namespace Ovito { namespace StdObj {

/**
 * The standard camera object.
 */
class OVITO_STDOBJ_EXPORT CameraObject : public AbstractCameraObject
{
	Q_OBJECT
	OVITO_CLASS(CameraObject)
	Q_CLASSINFO("DisplayName", "Camera");

public:

	/// Constructor.
	Q_INVOKABLE CameraObject(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(Application::ExecutionContext executionContext) override;
	
	/// Returns whether this camera is a target camera directory at a target object.
	bool isTargetCamera() const;

	/// Changes the type of the camera to a target camera or a free camera.
	void setIsTargetCamera(bool enable);

	/// With a target camera, indicates the distance between the camera and its target.
	FloatType targetDistance() const;

	/// \brief Returns a structure describing the camera's projection.
	/// \param[in] time The animation time for which the camera's projection parameters should be determined.
	/// \param[in,out] projParams The structure that is to be filled with the projection parameters.
	///     The following fields of the ViewProjectionParameters structure are already filled in when the method is called:
	///   - ViewProjectionParameters::aspectRatio (The aspect ratio (height/width) of the viewport)
	///   - ViewProjectionParameters::viewMatrix (The world to view space transformation)
	///   - ViewProjectionParameters::boundingBox (The bounding box of the scene in world space coordinates)
	virtual void projectionParameters(TimePoint time, ViewProjectionParameters& projParams) const override;

	/// \brief Returns whether this camera uses a perspective projection.
	virtual bool isPerspectiveCamera() const override { return isPerspective(); }

	/// \brief Sets whether this camera uses a perspective projection.
	virtual void setPerspectiveCamera(bool perspective) override { setIsPerspective(perspective); }

	/// \brief Returns the field of view of the camera.
	virtual FloatType fieldOfView(TimePoint time, TimeInterval& validityInterval) const override;

	/// \brief Changes the field of view of the camera.
	virtual void setFieldOfView(TimePoint time, FloatType newFOV) override;

	/// Asks the object for its validity interval at the given time.
	virtual TimeInterval objectValidity(TimePoint time) override;

	/// Returns whether this data object wants to be shown in the pipeline editor
	/// under the data source section.
	virtual bool showInPipelineEditor() const override { return true; }

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

/**
 * \brief A visual element for rendering camera objects in the interactive viewports.
 */
class OVITO_STDOBJ_EXPORT CameraVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(CameraVis)
	Q_CLASSINFO("DisplayName", "Camera icon");

public:

	/// \brief Constructor.
	Q_INVOKABLE CameraVis(DataSet* dataset) : DataVis(dataset) {}

	/// \brief Lets the vis element render a camera object.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;
};

}	// End of namespace
}	// End of namespace
