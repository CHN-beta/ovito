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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include "ClearSelectionModifierEditor.h"

namespace Ovito::StdMod {

SET_OVITO_OBJECT_EDITOR(ClearSelectionModifier, ClearSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ClearSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Clear selection"), rolloutParams, "manual:particles.modifiers.clear_selection");

	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(8,8,8,8);
	layout->setSpacing(4);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::subject));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());

	// List only property containers that support element selection.
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return container->getOOMetaClass().isValidStandardPropertyId(PropertyObject::GenericSelectionProperty);
	});
}

}	// End of namespace
