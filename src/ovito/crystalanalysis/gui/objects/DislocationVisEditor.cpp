////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 OVITO GmbH, Germany
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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/DislocationVis.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerRadioButtonParameterUI.h>
#include "DislocationVisEditor.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationVisEditor);
SET_OVITO_OBJECT_EDITOR(DislocationVis, DislocationVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Dislocation display"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* linesGroupBox = new QGroupBox(tr("Dislocation lines"));
	QGridLayout* sublayout = new QGridLayout(linesGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(linesGroupBox);

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(DislocationVis::shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), QVariant::fromValue((int)CylinderPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), QVariant::fromValue((int)CylinderPrimitive::FlatShading));
	sublayout->addWidget(new QLabel(tr("Shading mode:")), 0, 0);
	sublayout->addWidget(shadingModeUI->comboBox(), 0, 1);

	// Line width parameter.
	FloatParameterUI* lineWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationVis::lineWidth));
	sublayout->addWidget(lineWidthUI->label(), 1, 0);
	sublayout->addLayout(lineWidthUI->createFieldLayout(), 1, 1);

	// Show line directions.
	BooleanParameterUI* showLineDirectionsUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationVis::showLineDirections));
	sublayout->addWidget(showLineDirectionsUI->checkBox(), 2, 0, 1, 2);

	// Show Burgers vectors.
	BooleanGroupBoxParameterUI* showBurgersVectorsGroupUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(DislocationVis::showBurgersVectors));
	sublayout = new QGridLayout(showBurgersVectorsGroupUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(showBurgersVectorsGroupUI->groupBox());

	// Arrow scaling.
	FloatParameterUI* burgersVectorScalingUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationVis::burgersVectorScaling));
	sublayout->addWidget(new QLabel(tr("Scaling factor:")), 0, 0);
	sublayout->addLayout(burgersVectorScalingUI->createFieldLayout(), 0, 1);

	// Arrow width.
	FloatParameterUI* burgersVectorWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationVis::burgersVectorWidth));
	sublayout->addWidget(new QLabel(tr("Width:")), 1, 0);
	sublayout->addLayout(burgersVectorWidthUI->createFieldLayout(), 1, 1);

	// Arrow color.
	ColorParameterUI* burgersVectorColorUI = new ColorParameterUI(this, PROPERTY_FIELD(DislocationVis::burgersVectorColor));
	sublayout->addWidget(new QLabel(tr("Color:")), 2, 0);
	sublayout->addWidget(burgersVectorColorUI->colorPicker(), 2, 1);

	// Coloring mode.
	QGroupBox* coloringGroupBox = new QGroupBox(tr("Color lines by"));
	sublayout = new QGridLayout(coloringGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(coloringGroupBox);

	IntegerRadioButtonParameterUI* coloringModeUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(DislocationVis::lineColoringMode));
	sublayout->addWidget(coloringModeUI->addRadioButton(DislocationVis::ColorByDislocationType, tr("Dislocation type")), 0, 0, 1, 2);
	sublayout->addWidget(coloringModeUI->addRadioButton(DislocationVis::ColorByBurgersVector, tr("Burgers vector")), 1, 0, 1, 2);
	sublayout->addWidget(coloringModeUI->addRadioButton(DislocationVis::ColorByCharacter, tr("Local character")), 2, 0);
	sublayout->addWidget(new QLabel(tr("<p> (<font color=\"#FF0000\">screw</font>/<font color=\"#0000FF\">edge</font>)</p>")), 2, 1);
}

}	// End of namespace
}	// End of namespace
