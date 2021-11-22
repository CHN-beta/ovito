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
#include <ovito/particles/export/xyz/XYZExporter.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include "XYZExporterEditor.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(XYZExporterEditor);
SET_OVITO_OBJECT_EDITOR(XYZExporter, XYZExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void XYZExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("XYZ File"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1,1);
	layout->setColumnStretch(4,1);
	layout->setColumnMinimumWidth(2,10);
	layout->addWidget(new QLabel(tr("XYZ format style:")), 0, 0);

	VariantComboBoxParameterUI* subFormatUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(XYZExporter::subFormat));
	subFormatUI->comboBox()->addItem("Extended (default)", QVariant::fromValue((int)XYZExporter::ExtendedFormat));
	subFormatUI->comboBox()->addItem("Parcas", QVariant::fromValue((int)XYZExporter::ParcasFormat));
	layout->addWidget(subFormatUI->comboBox(), 0, 1);

	IntegerParameterUI* precisionUI = new IntegerParameterUI(this, PROPERTY_FIELD(FileExporter::floatOutputPrecision));
	layout->addWidget(precisionUI->label(), 0, 3);
	layout->addLayout(precisionUI->createFieldLayout(), 0, 4);

	FileColumnParticleExporterEditor::createUI(rolloutParams.before(rollout));
}

}	// End of namespace
