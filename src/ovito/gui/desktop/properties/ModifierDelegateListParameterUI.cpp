////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/gui/desktop/properties/ModifierPropertiesEditor.h>
#include "ModifierDelegateListParameterUI.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ModifierDelegateListParameterUI);

/******************************************************************************
* The constructor.
******************************************************************************/
ModifierDelegateListParameterUI::ModifierDelegateListParameterUI(QObject* parentEditor, const RolloutInsertionParameters& rolloutParams, OvitoClassPtr defaultEditorClass)
	: RefTargetListParameterUI(parentEditor, PROPERTY_FIELD(MultiDelegatingModifier::delegates), rolloutParams, defaultEditorClass)
{
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant ModifierDelegateListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	if(role == Qt::DisplayRole) {
		if(index.column() == 0 && target) {
			return target->objectTitle();
		}
	}
	else if(role == Qt::CheckStateRole) {
		if(index.column() == 0) {
			if(ModifierDelegate* delegate = dynamic_object_cast<ModifierDelegate>(target))
				return delegate->isEnabled() ? Qt::Checked : Qt::Unchecked;
		}
	}
	return {};
}

/******************************************************************************
* Sets the role data for the item at index to value.
******************************************************************************/
bool ModifierDelegateListParameterUI::setItemData(RefTarget* target, const QModelIndex& index, const QVariant& value, int role)
{
	if(index.column() == 0 && role == Qt::CheckStateRole) {
		if(ModifierDelegate* delegate = dynamic_object_cast<ModifierDelegate>(target)) {
			bool enabled = (value.toInt() == Qt::Checked);
			undoableTransaction(tr("Enable/disable data element"), [delegate, enabled]() {
				delegate->setEnabled(enabled);
			});
			return true;
		}
	}

	return RefTargetListParameterUI::setItemData(target, index, value, role);
}

/******************************************************************************
* Returns the model/view item flags for the given entry.
******************************************************************************/
Qt::ItemFlags ModifierDelegateListParameterUI::getItemFlags(RefTarget* target, const QModelIndex& index)
{
	Qt::ItemFlags flags = RefTargetListParameterUI::getItemFlags(target, index);
	if(index.column() == 0) {
		if(ModifierDelegate* delegate = dynamic_object_cast<ModifierDelegate>(target)) {
			if(ModifierPropertiesEditor* editor = dynamic_object_cast<ModifierPropertiesEditor>(this->editor())) {
				const PipelineFlowState& input = editor->getModifierInput();
				if(delegate->getOOMetaClass().getApplicableObjects(input).empty()) {
					flags &= ~Qt::ItemIsEnabled;
				}
			}
		}
		return flags | Qt::ItemIsUserCheckable;
	}
	return flags;
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ModifierDelegateListParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::ModifierInputChanged) {
//		updateColumns(0, 0);
	}
	return RefTargetListParameterUI::referenceEvent(source, event);
}

}	// End of namespace
