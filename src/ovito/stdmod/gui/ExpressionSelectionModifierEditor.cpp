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

#include <ovito/stdmod/gui/StdModGui.h>
#include <ovito/stdmod/modifiers/ExpressionSelectionModifier.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include <ovito/gui/desktop/properties/ModifierDelegateParameterUI.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteTextEdit.h>
#include "ExpressionSelectionModifierEditor.h"

namespace Ovito { namespace StdMod {

IMPLEMENT_OVITO_CLASS(ExpressionSelectionModifierEditor);
SET_OVITO_OBJECT_EDITOR(ExpressionSelectionModifier, ExpressionSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ExpressionSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Expression selection"), rolloutParams, "manual:particles.modifiers.expression_select");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(0);

	ModifierDelegateParameterUI* delegateUI = new ModifierDelegateParameterUI(this, ExpressionSelectionModifierDelegate::OOClass());
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(delegateUI->comboBox());

	layout->addWidget(new QLabel(tr("Boolean expression:")));
	StringParameterUI* expressionUI = new StringParameterUI(this, PROPERTY_FIELD(ExpressionSelectionModifier::expression));
	expressionEdit = new AutocompleteTextEdit();
	expressionUI->setTextBox(expressionEdit);
	layout->addWidget(expressionUI->textBox());

	// Status label.
	layout->addSpacing(12);
	layout->addWidget((new ObjectStatusDisplay(this))->statusWidget());

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "manual:particles.modifiers.expression_select");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
	variableNamesList = new QLabel();
	variableNamesList->setWordWrap(true);
	variableNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesList, 1);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &ExpressionSelectionModifierEditor::contentsReplaced, this, &ExpressionSelectionModifierEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ExpressionSelectionModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::ObjectStatusChanged) {
		updateEditorFields();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void ExpressionSelectionModifierEditor::updateEditorFields()
{
	ExpressionSelectionModifier* mod = static_object_cast<ExpressionSelectionModifier>(editObject());
	if(!mod) return;

	variableNamesList->setText(mod->inputVariableTable() + QStringLiteral("<p></p>"));
	container()->updateRolloutsLater();
	expressionEdit->setWordList(mod->inputVariableNames());
}

}	// End of namespace
}	// End of namespace
