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
#include <ovito/stdmod/modifiers/ComputePropertyModifier.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/gui/desktop/properties/ModifierDelegateParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include <ovito/gui/desktop/properties/VariantComboBoxParameterUI.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteLineEdit.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteTextEdit.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include "ComputePropertyModifierEditor.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(ComputePropertyModifierEditor);
SET_OVITO_OBJECT_EDITOR(ComputePropertyModifier, ComputePropertyModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ComputePropertyModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Compute property"), rolloutParams, "manual:particles.modifiers.compute_property");

    // Create the rollout contents.
	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	QGroupBox* operateOnGroup = new QGroupBox(tr("Operate on"));
	QVBoxLayout* sublayout = new QVBoxLayout(operateOnGroup);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	mainLayout->addWidget(operateOnGroup);

	ModifierDelegateParameterUI* delegateUI = new ModifierDelegateParameterUI(this, ComputePropertyModifierDelegate::OOClass());
	sublayout->addWidget(delegateUI->comboBox());

	QGroupBox* propertiesGroupBox = new QGroupBox(tr("Output property"), rollout);
	mainLayout->addWidget(propertiesGroupBox);
	QVBoxLayout* propertiesLayout = new QVBoxLayout(propertiesGroupBox);
	propertiesLayout->setContentsMargins(6,6,6,6);
	propertiesLayout->setSpacing(4);

	// Output property
	PropertyReferenceParameterUI* outputPropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::outputProperty), nullptr, PropertyReferenceParameterUI::ShowNoComponents, false);
	propertiesLayout->addWidget(outputPropertyUI->comboBox());
	connect(this, &PropertiesEditor::contentsChanged, this, [outputPropertyUI](RefTarget* editObject) {
		ComputePropertyModifier* modifier = static_object_cast<ComputePropertyModifier>(editObject);
		if(modifier && modifier->delegate())
			outputPropertyUI->setContainerRef(modifier->delegate()->inputContainerRef());
		else
			outputPropertyUI->setContainerRef({});
	});
	connect(outputPropertyUI, &PropertyReferenceParameterUI::valueEntered, this, [this]() {
		if(ComputePropertyModifier* modifier = static_object_cast<ComputePropertyModifier>(editObject())) {
			modifier->adjustPropertyComponentCount();
		}
	});

	// Create the check box for the selection flag.
	BooleanParameterUI* selectionFlagUI = new BooleanParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::onlySelectedElements));
	propertiesLayout->addWidget(selectionFlagUI->checkBox());

	expressionsGroupBox = new QGroupBox(tr("Expression"));
	mainLayout->addWidget(expressionsGroupBox);
	expressionsLayout = new QGridLayout(expressionsGroupBox);
	expressionsLayout->setContentsMargins(4,4,4,4);
	expressionsLayout->setSpacing(1);
	expressionsLayout->setRowMinimumHeight(1,4);
	expressionsLayout->setColumnStretch(1,1);

	// Show multiline fields.
	BooleanParameterUI* multilineFieldsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::useMultilineFields));
	expressionsLayout->addWidget(multilineFieldsUI->checkBox(), 0, 1, Qt::AlignRight | Qt::AlignBottom);

	// Status label.
	mainLayout->addWidget((new ObjectStatusDisplay(this))->statusWidget());

	// List of available input variables.
	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "manual:particles.modifiers.compute_property");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
    variableNamesDisplay = new QLabel();
	variableNamesDisplay->setWordWrap(true);
	variableNamesDisplay->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesDisplay);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &ComputePropertyModifierEditor::contentsReplaced, this, &ComputePropertyModifierEditor::updateExpressionFields);
	connect(this, &ComputePropertyModifierEditor::contentsReplaced, this, &ComputePropertyModifierEditor::updateVariablesList);

	// Show settings editor of modifier delegate.
	new SubObjectParameterUI(this, PROPERTY_FIELD(AsynchronousDelegatingModifier::delegate), rolloutParams.before(variablesRollout));
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ComputePropertyModifierEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject() && event.type() == ReferenceEvent::TargetChanged) {
		updateExpressionFieldsLater(this);
	}
	else if(source == editObject() && event.type() == ReferenceEvent::ObjectStatusChanged) {
		updateVariablesListLater(this);
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the editor's display of the available expression variables.
******************************************************************************/
void ComputePropertyModifierEditor::updateVariablesList()
{
	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	if(!mod) return;

	if(ComputePropertyModifierApplication* modApp = dynamic_object_cast<ComputePropertyModifierApplication>(modifierApplication())) {
		const QStringList& inputVariableNames = modApp->inputVariableNames();
		for(AutocompleteLineEdit* box : expressionLineEdits)
			box->setWordList(inputVariableNames);
		for(AutocompleteTextEdit* box : expressionTextEdits)
			box->setWordList(inputVariableNames);
		variableNamesDisplay->setText(modApp->inputVariableTable() + QStringLiteral("<p></p>"));
	}

	container()->updateRolloutsLater();
}

/******************************************************************************
* Updates the editor's input fields for the expressions.
******************************************************************************/
void ComputePropertyModifierEditor::updateExpressionFields()
{
	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	if(!mod) return;

	const QStringList& expr = mod->expressions();
	expressionsGroupBox->setTitle((expr.size() <= 1) ? tr("Expression") : tr("Expressions"));
	while(expr.size() > expressionLineEdits.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* lineEdit = new AutocompleteLineEdit();
		AutocompleteTextEdit* textEdit = new AutocompleteTextEdit();
		expressionsLayout->addWidget(label, expressionTextEdits.size()+2, 0);
		expressionsLayout->addWidget(lineEdit, expressionLineEdits.size()+2, 1);
		expressionsLayout->addWidget(textEdit, expressionTextEdits.size()+2, 1);
		expressionLineEdits.push_back(lineEdit);
		expressionTextEdits.push_back(textEdit);
		expressionLabels.push_back(label);
		connect(lineEdit, &AutocompleteLineEdit::editingFinished, this, &ComputePropertyModifierEditor::onExpressionEditingFinished);
		connect(textEdit, &AutocompleteTextEdit::editingFinished, this, &ComputePropertyModifierEditor::onExpressionEditingFinished);
	}
	while(expr.size() < expressionLineEdits.size()) {
		delete expressionLineEdits.takeLast();
		delete expressionTextEdits.takeLast();
		delete expressionLabels.takeLast();
	}
	OVITO_ASSERT(expressionLineEdits.size() == expr.size());
	OVITO_ASSERT(expressionTextEdits.size() == expr.size());
	OVITO_ASSERT(expressionLabels.size() == expr.size());
	if(mod->useMultilineFields()) {
		for(AutocompleteLineEdit* lineEdit : expressionLineEdits) lineEdit->setVisible(false);
		for(AutocompleteTextEdit* textEdit : expressionTextEdits) textEdit->setVisible(true);
	}
	else {
		for(AutocompleteLineEdit* lineEdit : expressionLineEdits) lineEdit->setVisible(true);
		for(AutocompleteTextEdit* textEdit : expressionTextEdits) textEdit->setVisible(false);
	}

	QStringList standardPropertyComponentNames;
	if(!mod->outputProperty().isNull() && mod->outputProperty().type() != PropertyObject::GenericUserProperty) {
		standardPropertyComponentNames = mod->outputProperty().containerClass()->standardPropertyComponentNames(mod->outputProperty().type());
	}

	for(int i = 0; i < expr.size(); i++) {
		expressionLineEdits[i]->setText(expr[i]);
		expressionTextEdits[i]->setPlainText(expr[i]);
		if(expr.size() == 1)
			expressionLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				expressionLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				expressionLabels[i]->setText(tr("%1:").arg(i+1));
			expressionLabels[i]->show();
		}
	}
	container()->updateRolloutsLater();
}

/******************************************************************************
* Is called when the user has typed in an expression.
******************************************************************************/
void ComputePropertyModifierEditor::onExpressionEditingFinished()
{
	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	int index;
	QString expr;
	if(mod->useMultilineFields()) {
		AutocompleteTextEdit* edit = static_cast<AutocompleteTextEdit*>(sender());
		index = expressionTextEdits.indexOf(edit);
		expr = edit->toPlainText();
	}
	else {
		AutocompleteLineEdit* edit = static_cast<AutocompleteLineEdit*>(sender());
		index = expressionLineEdits.indexOf(edit);
		expr = edit->text();
	}
	OVITO_ASSERT(index >= 0);
	undoableTransaction(tr("Change expression"), [mod, expr, index]() {
		QStringList expressions = mod->expressions();
		expressions[index] = expr;
		mod->setExpressions(expressions);
	});
}

}	// End of namespace
