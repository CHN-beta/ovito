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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/coloring/AmbientOcclusionModifier.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include "AmbientOcclusionModifierEditor.h"

namespace Ovito::Particles {

SET_OVITO_OBJECT_EDITOR(AmbientOcclusionModifier, AmbientOcclusionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AmbientOcclusionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Ambient occlusion"), rolloutParams, "manual:particles.modifiers.ambient_occlusion");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(4);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	// Intensity parameter.
	FloatParameterUI* intensityPUI = new FloatParameterUI(this, PROPERTY_FIELD(AmbientOcclusionModifier::intensity));
	layout2->addWidget(intensityPUI->label(), 0, 0);
	layout2->addLayout(intensityPUI->createFieldLayout(), 0, 1);

	// Sampling level parameter.
	IntegerParameterUI* samplingCountPUI = new IntegerParameterUI(this, PROPERTY_FIELD(AmbientOcclusionModifier::samplingCount));
	layout2->addWidget(samplingCountPUI->label(), 1, 0);
	layout2->addLayout(samplingCountPUI->createFieldLayout(), 1, 1);

	// Buffer resolution parameter.
	IntegerParameterUI* bufferResPUI = new IntegerParameterUI(this, PROPERTY_FIELD(AmbientOcclusionModifier::bufferResolution));
	layout2->addWidget(bufferResPUI->label(), 2, 0);
	layout2->addLayout(bufferResPUI->createFieldLayout(), 2, 1);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget((new ObjectStatusDisplay(this))->statusWidget());
}

}	// End of namespace
