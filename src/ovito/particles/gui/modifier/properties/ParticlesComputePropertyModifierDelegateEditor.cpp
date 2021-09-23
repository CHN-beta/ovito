////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 OVITO GmbH, Germany
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
#include <ovito/particles/modifier/properties/ParticlesComputePropertyModifierDelegate.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteLineEdit.h>
#include <ovito/gui/desktop/widgets/general/AutocompleteTextEdit.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "ParticlesComputePropertyModifierDelegateEditor.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesComputePropertyModifierDelegateEditor);
SET_OVITO_OBJECT_EDITOR(ParticlesComputePropertyModifierDelegate, ParticlesComputePropertyModifierDelegateEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ParticlesComputePropertyModifierDelegateEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
    // Neighbor mode panel.
	QWidget* neighorRollout = createRollout(tr("Neighbor particles"), rolloutParams, "manual:particles.modifiers.compute_property");

	QVBoxLayout* mainLayout = new QVBoxLayout(neighorRollout);
	mainLayout->setContentsMargins(4,4,4,4);

	QGroupBox* rangeGroupBox = new QGroupBox(tr("Evaluation range"));
	mainLayout->addWidget(rangeGroupBox);
	QGridLayout* rangeGroupBoxLayout = new QGridLayout(rangeGroupBox);
	rangeGroupBoxLayout->setContentsMargins(4,4,4,4);
	rangeGroupBoxLayout->setSpacing(1);
	rangeGroupBoxLayout->setColumnStretch(1,1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate::cutoff));
	rangeGroupBoxLayout->addWidget(cutoffRadiusUI->label(), 0, 0);
	rangeGroupBoxLayout->addLayout(cutoffRadiusUI->createFieldLayout(), 0, 1);

	neighborExpressionsGroupBox = new QGroupBox(tr("Neighbor expression"));
	mainLayout->addWidget(neighborExpressionsGroupBox);
	neighborExpressionsLayout = new QGridLayout(neighborExpressionsGroupBox);
	neighborExpressionsLayout->setContentsMargins(4,4,4,4);
	neighborExpressionsLayout->setSpacing(1);
	neighborExpressionsLayout->setRowMinimumHeight(1, 4);
	neighborExpressionsLayout->setColumnStretch(1, 1);

	// Show multiline fields.
	BooleanParameterUI* multilineFieldsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticlesComputePropertyModifierDelegate::useMultilineFields));
	neighborExpressionsLayout->addWidget(multilineFieldsUI->checkBox(), 0, 1, Qt::AlignRight | Qt::AlignBottom);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &ParticlesComputePropertyModifierDelegateEditor::contentsReplaced, this, &ParticlesComputePropertyModifierDelegateEditor::updateExpressionFields);
	connect(this, &ParticlesComputePropertyModifierDelegateEditor::contentsReplaced, this, &ParticlesComputePropertyModifierDelegateEditor::updateVariablesList);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ParticlesComputePropertyModifierDelegateEditor::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject()) {
		if(event.type() == ReferenceEvent::TargetChanged) {
			updateExpressionFieldsLater(this);
		}
		else if(event.type() == ReferenceEvent::ObjectStatusChanged) {
			updateVariablesListLater(this);
		}
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the editor's display of the available expression variables.
******************************************************************************/
void ParticlesComputePropertyModifierDelegateEditor::updateVariablesList()
{
	if(ComputePropertyModifierApplication* modApp = dynamic_object_cast<ComputePropertyModifierApplication>(modifierApplication())) {
		const QStringList& inputVariableNames = modApp->delegateInputVariableNames();
		for(AutocompleteLineEdit* box : neighborExpressionLineEdits)
			box->setWordList(inputVariableNames);
		for(AutocompleteTextEdit* box : neighborExpressionTextEdits)
			box->setWordList(inputVariableNames);
	}
}

/******************************************************************************
* Updates the editor's input fields for the expressions.
******************************************************************************/
void ParticlesComputePropertyModifierDelegateEditor::updateExpressionFields()
{
	ParticlesComputePropertyModifierDelegate* delegate = static_object_cast<ParticlesComputePropertyModifierDelegate>(editObject());
	if(!delegate) return;

	const QStringList& neighExpr = delegate->neighborExpressions();
	neighborExpressionsGroupBox->setTitle((neighExpr.size() <= 1) ? tr("Neighbor expression") : tr("Neighbor expressions"));
	while(neighExpr.size() > neighborExpressionLineEdits.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* lineEdit = new AutocompleteLineEdit();
		AutocompleteTextEdit* textEdit = new AutocompleteTextEdit();
		neighborExpressionsLayout->addWidget(label, neighborExpressionLineEdits.size()+2, 0);
		neighborExpressionsLayout->addWidget(lineEdit, neighborExpressionLineEdits.size()+2, 1);
		neighborExpressionsLayout->addWidget(textEdit, neighborExpressionTextEdits.size()+2, 1);
		neighborExpressionLineEdits.push_back(lineEdit);
		neighborExpressionTextEdits.push_back(textEdit);
		neighborExpressionLabels.push_back(label);
		connect(lineEdit, &AutocompleteLineEdit::editingFinished, this, &ParticlesComputePropertyModifierDelegateEditor::onExpressionEditingFinished);
		connect(textEdit, &AutocompleteTextEdit::editingFinished, this, &ParticlesComputePropertyModifierDelegateEditor::onExpressionEditingFinished);
	}
	while(neighExpr.size() < neighborExpressionLineEdits.size()) {
		delete neighborExpressionLineEdits.takeLast();
		delete neighborExpressionTextEdits.takeLast();
		delete neighborExpressionLabels.takeLast();
	}
	OVITO_ASSERT(neighborExpressionLineEdits.size() == neighExpr.size());
	OVITO_ASSERT(neighborExpressionTextEdits.size() == neighExpr.size());
	OVITO_ASSERT(neighborExpressionLabels.size() == neighExpr.size());
	if(delegate->useMultilineFields()) {
		for(AutocompleteLineEdit* lineEdit : neighborExpressionLineEdits) lineEdit->setVisible(false);
		for(AutocompleteTextEdit* textEdit : neighborExpressionTextEdits) textEdit->setVisible(true);
	}
	else {
		for(AutocompleteLineEdit* lineEdit : neighborExpressionLineEdits) lineEdit->setVisible(true);
		for(AutocompleteTextEdit* textEdit : neighborExpressionTextEdits) textEdit->setVisible(false);
	}

	QStringList standardPropertyComponentNames;
	if(ComputePropertyModifier* modifier = dynamic_object_cast<ComputePropertyModifier>(delegate->modifier())) {
		if(!modifier->outputProperty().isNull() && modifier->outputProperty().type() != PropertyObject::GenericUserProperty) {
			standardPropertyComponentNames = modifier->outputProperty().containerClass()->standardPropertyComponentNames(modifier->outputProperty().type());
		}
	}

	for(int i = 0; i < neighExpr.size(); i++) {
		neighborExpressionLineEdits[i]->setText(neighExpr[i]);
		neighborExpressionTextEdits[i]->setPlainText(neighExpr[i]);
		if(neighExpr.size() == 1)
			neighborExpressionLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				neighborExpressionLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				neighborExpressionLabels[i]->setText(tr("%1:").arg(i+1));
			neighborExpressionLabels[i]->show();
		}
	}
}

/******************************************************************************
* Is called when the user has typed in an expression.
******************************************************************************/
void ParticlesComputePropertyModifierDelegateEditor::onExpressionEditingFinished()
{
	ParticlesComputePropertyModifierDelegate* delegate = static_object_cast<ParticlesComputePropertyModifierDelegate>(editObject());
	int index;
	QString expr;
	if(AutocompleteLineEdit* edit = dynamic_cast<AutocompleteLineEdit*>(sender())) {
		index = neighborExpressionLineEdits.indexOf(edit);
		expr = edit->text();
	}
	else if(AutocompleteTextEdit* edit = dynamic_cast<AutocompleteTextEdit*>(sender())) {
		index = neighborExpressionTextEdits.indexOf(edit);
		expr = edit->toPlainText();
	}
	else return;
	OVITO_ASSERT(index >= 0);
	undoableTransaction(tr("Change neighbor expression"), [delegate, expr, index]() {
		QStringList expressions = delegate->neighborExpressions();
		expressions[index] = expr;
		delegate->setNeighborExpressions(expressions);
	});
}

}	// End of namespace
}	// End of namespace
