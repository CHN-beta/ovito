////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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


#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/stdmod/modifiers/SliceModifier.h>

namespace Ovito::StdMod {

class PickPlanePointsInputMode;	// Defined below

/**
 * A properties editor for the SliceModifier class.
 */
class SliceModifierEditor : public PropertiesEditor
{
	OVITO_CLASS(SliceModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE SliceModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Aligns the slicing plane to the viewing direction.
	void onAlignPlaneToView();

	/// Aligns the current viewing direction to the slicing plane.
	void onAlignViewToPlane();

	/// Aligns the normal of the slicing plane with one of the coordinate axes.
	void onAlignNormalWithAxis(const QString& link);

	/// Moves the plane to the center of the simulation box.
	void onCenterOfBox();

	/// Is called when the user switches between Cartesian and reduced cell coordinates.
	void onCoordinateTypeChanged();

	/// Is called when the selected type of plane normal coordinates have changed.
	void updateCoordinateLabels();

private:

	PickPlanePointsInputMode* _pickPlanePointsInputMode;
	ViewportModeAction* _pickPlanePointsInputModeAction;
	BooleanRadioButtonParameterUI* _reducedCoordinatesPUI;
	Vector3ParameterUI* _normalPUI[3];
	FloatParameterUI* _distancePUI;
};

/******************************************************************************
* The viewport input mode that lets the user select three points in space
* to define the slicing plane.
******************************************************************************/
class PickPlanePointsInputMode : public ViewportInputMode, public ViewportGizmo
{
public:

	/// Constructor.
	PickPlanePointsInputMode(SliceModifierEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// Handles mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// Lets the input mode render its overlay content in a viewport.
	virtual void renderOverlay3D(Viewport* vp, SceneRenderer* renderer) override;

protected:

	/// This is called by the system after the input handler has become the active handler.
	virtual void activated(bool temporary) override;

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated(bool temporary) override;

private:

	/// Aligns the modifier's slicing plane to the three selected points.
	void alignPlane(SliceModifier* mod);

	/// The list of spatial points picked by the user so far.
	Point3 _pickedPoints[3];

	/// The number of points picked so far.
	int _numPickedPoints = 0;

	/// Indicates whether a preliminary point is available.
	bool _hasPreliminaryPoint = false;

	/// The properties editor of the SliceModifier.
	SliceModifierEditor* _editor;
};

}	// End of namespace
