////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 OVITO GmbH, Germany
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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/modify/LoadTrajectoryModifier.h>
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include "LoadTrajectoryModifierEditor.h"

namespace Ovito::Particles {

SET_OVITO_OBJECT_EDITOR(LoadTrajectoryModifier, LoadTrajectoryModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LoadTrajectoryModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Load Trajectory"), rolloutParams, "manual:particles.modifiers.load_trajectory");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	// Status label.
	layout->addWidget((new ObjectStatusDisplay(this))->statusWidget());

	// Open a sub-editor for the source object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(LoadTrajectoryModifier::trajectorySource), RolloutInsertionParameters().setTitle(tr("Trajectory Source: %1")));
}

}	// End of namespace
