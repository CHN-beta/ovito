////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 Alexander Stukowski
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
#include <ovito/particles/import/cif/mmCIFImporter.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include "mmCIFImporterEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(mmCIFImporterEditor);
SET_OVITO_OBJECT_EDITOR(mmCIFImporter, mmCIFImporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void mmCIFImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("mmCIF/PDBx reader"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* optionsBox = new QGroupBox(tr("Options"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(optionsBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(optionsBox);

	// Center simulation cell.
	BooleanParameterUI* recenterCellUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::recenterCell));
	sublayout->addWidget(recenterCellUI->checkBox());
	
	// Generate bonds
	BooleanParameterUI* generateBondsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::generateBonds));
	sublayout->addWidget(generateBondsUI->checkBox());
}

}	// End of namespace
}	// End of namespace
