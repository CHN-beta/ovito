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
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerCheckBoxParameterUI.h>
#include "BondsVisEditor.h"

namespace Ovito::Particles {

SET_OVITO_OBJECT_EDITOR(BondsVis, BondsVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BondsVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bonds display"), rolloutParams, "manual:visual_elements.bonds");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(2, 1);
	layout->setColumnMinimumWidth(0, 24);

	// Bond width.
	FloatParameterUI* bondWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(BondsVis::bondWidth));
	layout->addWidget(bondWidthUI->label(), 0, 0, 1, 2);
	layout->addLayout(bondWidthUI->createFieldLayout(), 0, 2);

	// Shading mode.
	IntegerCheckBoxParameterUI* shadingModeUI = new IntegerCheckBoxParameterUI(this, PROPERTY_FIELD(BondsVis::shadingMode), BondsVis::NormalShading, BondsVis::FlatShading);
	shadingModeUI->checkBox()->setText(tr("Flat shading"));
	layout->addWidget(shadingModeUI->checkBox(), 1, 2);

	// Coloring mode.
	layout->addWidget(new QLabel(tr("Coloring mode:")), 2, 0, 1, 3);
	_coloringModeUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(BondsVis::coloringMode));
	layout->addWidget(_coloringModeUI->addRadioButton(BondsVis::UniformColoring, tr("Uniform:")), 3, 1);
	
	// Uniform color.
	_bondColorUI = new ColorParameterUI(this, PROPERTY_FIELD(BondsVis::bondColor));
	layout->addWidget(_bondColorUI->colorPicker(), 3, 2);

	// By bond type coloring mode.
	layout->addWidget(_coloringModeUI->addRadioButton(BondsVis::ByTypeColoring, tr("Bond types")), 4, 1, 1, 2);

	// By particle coloring mode.
	layout->addWidget(_coloringModeUI->addRadioButton(BondsVis::ParticleBasedColoring, tr("Use particle colors")), 5, 1, 1, 2);

	// Whenever the pipeline input of the vis element changes, update the list of available coloring options.
	connect(this, &PropertiesEditor::pipelineInputChanged, this, &BondsVisEditor::updateColoringOptions);

	// Update the coloring controls when a parameter of the vis element has been changed.
	connect(this, &PropertiesEditor::contentsChanged, this, &BondsVisEditor::updateColoringOptions);	
}

/******************************************************************************
* Updates the coloring controls shown in the UI.
******************************************************************************/
void BondsVisEditor::updateColoringOptions()
{
	// Retrieve the BondsObject this vis element is associated with.
	DataOORef<const BondsObject> bonds = dynamic_object_cast<const BondsObject>(getVisDataObject());

	// Do the bonds have explicit RGB colors assigned ("Color" property exists)?
	bool hasExplicitColors = (bonds && bonds->getProperty(BondsObject::ColorProperty));

	BondsVis::ColoringMode coloringMode = editObject() ? static_object_cast<BondsVis>(editObject())->coloringMode() : BondsVis::UniformColoring;

	_bondColorUI->setEnabled(editObject() && !hasExplicitColors && coloringMode == BondsVis::UniformColoring);

	_coloringModeUI->buttonGroup()->button(BondsVis::UniformColoring)->setEnabled(editObject() && !hasExplicitColors);
	_coloringModeUI->buttonGroup()->button(BondsVis::ByTypeColoring)->setEnabled(bonds && !hasExplicitColors && bonds->getProperty(BondsObject::TypeProperty));
	_coloringModeUI->buttonGroup()->button(BondsVis::ParticleBasedColoring)->setEnabled(!hasExplicitColors);
}

}	// End of namespace
