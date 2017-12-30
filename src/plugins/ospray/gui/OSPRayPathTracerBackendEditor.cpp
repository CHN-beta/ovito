///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <gui/GUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <plugins/ospray/renderer/OSPRayBackend.h>
#include "OSPRayPathTracerBackendEditor.h"

namespace Ovito { namespace OSPRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_CLASS(OSPRayPathTracerBackendEditor);
SET_OVITO_OBJECT_EDITOR(OSPRayPathTracerBackend, OSPRayPathTracerBackendEditor);

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void OSPRayPathTracerBackendEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Path tracer settings"), rolloutParams);

	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	
	// Roulette depth.
	IntegerParameterUI* rouletteDepthUI = new IntegerParameterUI(this, PROPERTY_FIELD(OSPRayPathTracerBackend::rouletteDepth));
	layout->addWidget(rouletteDepthUI->label(), 0, 0);
	layout->addLayout(rouletteDepthUI->createFieldLayout(), 0, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace