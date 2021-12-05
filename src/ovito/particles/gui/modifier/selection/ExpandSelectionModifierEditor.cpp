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
#include <ovito/particles/modifier/selection/ExpandSelectionModifier.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include "ExpandSelectionModifierEditor.h"

namespace Ovito::Particles {

SET_OVITO_OBJECT_EDITOR(ExpandSelectionModifier, ExpandSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ExpandSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Expand selection"), rolloutParams, "manual:particles.modifiers.expand_selection");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QLabel* label = new QLabel(tr("Expand current selection to include particles that are..."));
	label->setWordWrap(true);
	layout->addWidget(label);

	IntegerRadioButtonParameterUI* modePUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::mode));
	QRadioButton* cutoffModeBtn = modePUI->addRadioButton(ExpandSelectionModifier::CutoffRange, tr("... within the range:"));
	layout->addSpacing(10);
	layout->addWidget(cutoffModeBtn);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::cutoffRange));
	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->addSpacing(20);
	sublayout->addWidget(cutoffRadiusPUI->label());
	sublayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 1);
	layout->addLayout(sublayout);
	cutoffRadiusPUI->setEnabled(false);
	connect(cutoffModeBtn, &QRadioButton::toggled, cutoffRadiusPUI, &FloatParameterUI::setEnabled);

	QRadioButton* nearestNeighborsModeBtn = modePUI->addRadioButton(ExpandSelectionModifier::NearestNeighbors, tr("... among the N nearest neighbors:"));
	layout->addSpacing(10);
	layout->addWidget(nearestNeighborsModeBtn);

	// Number of nearest neighbors.
	IntegerParameterUI* numNearestNeighborsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::numNearestNeighbors));
	sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->addSpacing(20);
	sublayout->addWidget(numNearestNeighborsPUI->label());
	sublayout->addLayout(numNearestNeighborsPUI->createFieldLayout(), 1);
	layout->addLayout(sublayout);
	numNearestNeighborsPUI->setEnabled(false);
	connect(nearestNeighborsModeBtn, &QRadioButton::toggled, numNearestNeighborsPUI, &FloatParameterUI::setEnabled);

	QRadioButton* bondModeBtn = modePUI->addRadioButton(ExpandSelectionModifier::BondedNeighbors, tr("... bonded to a selected particle."));
	layout->addSpacing(10);
	layout->addWidget(bondModeBtn);

	layout->addSpacing(10);
	IntegerParameterUI* numIterationsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::numberOfIterations));
	sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->addWidget(numIterationsPUI->label());
	sublayout->addLayout(numIterationsPUI->createFieldLayout(), 1);
	layout->addLayout(sublayout);

	// Status label.
	layout->addSpacing(10);
	layout->addWidget((new ObjectStatusDisplay(this))->statusWidget());
}

}	// End of namespace
