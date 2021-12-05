////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 OVITO GmbH, Germany
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/dataset/data/mesh/TriMeshObject.h>
#include "VRSceneRenderer.h"
#include "VRSettingsObject.h"

#include <OpenVR/openvr.h>

namespace VRPlugin {

using namespace Ovito;

class VRRenderingWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:

	/// Constructor.
    VRRenderingWidget(QWidget* parent, DataSet* dataset);

	/// Destructor.
    ~VRRenderingWidget();

	/// Returns the dataset that is being shown in the VR window.
	DataSet* dataset() const { return _sceneRenderer->dataset(); }

	/// Returns the settings object.
	VRSettingsObject* settings() const { return _settings; }

	// From QWidget:
	QSize minimumSizeHint() const override { return QSize(50, 50); }
    QSize sizeHint() const override { return QSize(500, 500); }

public Q_SLOTS:

	/// Called when the GL context is destroyed.
	void cleanup();

protected:

	/// Called when the GL context is initialized.
    void initializeGL() override;

	/// Called when the VR window contents are rendered.
    void paintGL() override;

	/// Called when the VR display window is resized.
    void resizeGL(int width, int height) override;

private:

	/// Computes the projection and transformation matrices for each of the two eyes.
	ViewProjectionParameters projectionParameters(int eye, FloatType aspectRatio, const AffineTransformation& bodyToWorldTM, const Box3& sceneBoundingBox = Box3());

	/// Converts a transformation matrix from the OpenVR format to OVITO's internal format.
	static AffineTransformation fromOpenVRMatrix(const vr::HmdMatrix34_t& tm) {
		AffineTransformation tm_out;
		for(size_t r = 0; r < 3; ++r)
			for(size_t c = 0; c < 4; ++c)
				tm_out(r,c) = static_cast<FloatType>(tm.m[r][c]);
		return tm_out;
	}

	/// Converts a transformation matrix from the OpenVR format to OVITO's internal format.
	static Matrix4 fromOpenVRMatrix(const vr::HmdMatrix44_t& tm) {
		Matrix4 tm_out;
		for(size_t r = 0; r < 4; ++r)
			for(size_t c = 0; c < 4; ++c)
				tm_out(r,c) = static_cast<FloatType>(tm.m[r][c]);
		return tm_out;
	}

	/// This is the OpenGL scene renderer.
	OORef<VRSceneRenderer> _sceneRenderer;

	/// VR Headset interface.
	vr::IVRSystem* _hmd = nullptr;
    vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

	/// Corner positions of the VR play area.
	vr::HmdQuad_t _playAreaRect;
	TriMesh _playAreaMesh;

	/// Floor geometry.
	std::shared_ptr<MeshPrimitive> _floorMesh;

	/// Controller geometry.
	std::shared_ptr<CylinderPrimitive> _controllerGeometry;
	FloatType _controllerSize = 0.2f;

	// Current size of VR monitor window.
	int _windowWidth;
	int _windowHeight;

	// Preferred rendering resolution of the VR headset.
	uint32_t _hmdRenderWidth = 800;
	uint32_t _hmdRenderHeight = 600;

	/// Rendering resolution.
	QSize _renderResolution;

	// GL framebuffer into which the each eye's view is rendered.
	std::unique_ptr<QOpenGLFramebufferObject> _eyeBuffer;

	/// Used for time measurements.
	QTime _time;

	/// For smooth acceleration.
	FloatType _currentSpeed = 0;

	/// The settings object.
	OORef<VRSettingsObject> _settings;
};

}	// End of namespace
