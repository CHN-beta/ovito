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
#include <ovito/particles/modifier/analysis/centrosymmetry/CentroSymmetryModifier.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <qwt/qwt_plot_zoneitem.h>
#include "CentroSymmetryModifierEditor.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(CentroSymmetryModifierEditor);
SET_OVITO_OBJECT_EDITOR(CentroSymmetryModifier, CentroSymmetryModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CentroSymmetryModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Centrosymmetry parameter"), rolloutParams, "manual:particles.modifiers.centrosymmetry");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(6);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	// Num neighbors parameter.
	IntegerParameterUI* numNeighborsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CentroSymmetryModifier::numNeighbors));
	layout2->addWidget(numNeighborsPUI->label(), 0, 0);
	layout2->addLayout(numNeighborsPUI->createFieldLayout(), 0, 1);

    // Info label
	//QLabel* infoLabel = new QLabel(tr("This parameter specifies the number of nearest neighbors in the underlying lattice of atoms. For FCC and BCC lattices, set this to 12 and 8 respectively. More generally, it must be a positive, even integer."));
	//infoLabel->setWordWrap(true);
	//layout2->addWidget(infoLabel);

	QGridLayout* layout3 = new QGridLayout();
	layout3->setContentsMargins(0,0,0,0);
	layout3->setColumnMinimumWidth(0, 20);
	layout1->addLayout(layout3);

	// Method selector.
	IntegerRadioButtonParameterUI* modeUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CentroSymmetryModifier::mode));
	QRadioButton* conventionalModeBtn = modeUI->addRadioButton(CentroSymmetryModifier::ConventionalMode, tr("Conventional CSP"));
	QRadioButton* matchingModeBtn = modeUI->addRadioButton(CentroSymmetryModifier::MatchingMode, tr("Minimum-weight matching CSP"));
	layout3->addWidget(new QLabel(tr("Algorithm:")), 0, 0, 1, 2);
	layout3->addWidget(conventionalModeBtn, 1, 1);
	layout3->addWidget(matchingModeBtn, 2, 1);

	// Use only selected particles.
	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(CentroSymmetryModifier::onlySelectedParticles));
	layout1->addSpacing(6);
	layout1->addWidget(onlySelectedParticlesUI->checkBox());

	// Create plot widget for CSP distribution.
	_cspPlotWidget = new DataTablePlotWidget();
	_cspPlotWidget->setMinimumHeight(200);
	_cspPlotWidget->setMaximumHeight(200);
	layout1->addSpacing(10);
	layout1->addWidget(_cspPlotWidget);

	// Update data plot whenever the modifier has calculated new results.
	connect(this, &PropertiesEditor::pipelineOutputChanged, this, &CentroSymmetryModifierEditor::plotHistogram);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget((new ObjectStatusDisplay(this))->statusWidget());
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void CentroSymmetryModifierEditor::plotHistogram()
{
	// Request the modifier's pipeline output.
	const PipelineFlowState& state = getPipelineOutput();

	if(state) {
		// Look up the data table in the modifier's pipeline output.
		_cspPlotWidget->setTable(state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("csp-centrosymmetry")));
	}
	else {
		_cspPlotWidget->reset();
	}
}

}	// End of namespace
