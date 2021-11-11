////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include <ovito/gui/desktop/properties/NumericalParameterUI.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/utilities/units/UnitsManager.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(NumericalParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
NumericalParameterUI::NumericalParameterUI(PropertiesEditor* parentEditor, const char* propertyName, const QMetaObject* defaultParameterUnitType, const QString& labelText) :
	PropertyParameterUI(parentEditor, propertyName), _parameterUnitType(defaultParameterUnitType)
{
	initUIControls(labelText);
}

/******************************************************************************
* Constructor for a PropertyField or ReferenceField property.
******************************************************************************/
NumericalParameterUI::NumericalParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField, const QMetaObject* defaultParameterUnitType) :
	PropertyParameterUI(parentEditor, propField), _parameterUnitType(defaultParameterUnitType)
{
	// Look up the ParameterUnit type for this parameter.
	if(propField->numericalParameterInfo() && propField->numericalParameterInfo()->unitType)
		_parameterUnitType = propField->numericalParameterInfo()->unitType;

	initUIControls(propField->displayName() + ":");
}

/******************************************************************************
* Creates the widgets for this property UI.
******************************************************************************/
void NumericalParameterUI::initUIControls(const QString& labelText)
{
	// Create UI widgets.
	_label = new QLabel(labelText);
	_textBox = new QLineEdit();
	_spinner = new SpinnerWidget();
	connect(spinner(), &SpinnerWidget::spinnerValueChanged, this, &NumericalParameterUI::onSpinnerValueChanged);
	connect(spinner(), &SpinnerWidget::spinnerDragStart, this, &NumericalParameterUI::onSpinnerDragStart);
	connect(spinner(), &SpinnerWidget::spinnerDragStop, this, &NumericalParameterUI::onSpinnerDragStop);
	connect(spinner(), &SpinnerWidget::spinnerDragAbort, this, &NumericalParameterUI::onSpinnerDragAbort);
	spinner()->setTextBox(_textBox);
	if(propertyField()->numericalParameterInfo() != nullptr) {
		spinner()->setMinValue(propertyField()->numericalParameterInfo()->minValue);
		spinner()->setMaxValue(propertyField()->numericalParameterInfo()->maxValue);
	}

	// Create animate button if parameter is animation (i.e. it's a reference to a Controller object).
	if(isReferenceFieldUI() && propertyField()->targetClass()->isDerivedFrom(Controller::OOClass())) {
		_animateButton = new QToolButton();
		_animateButton->setText(tr("A"));
		_animateButton->setFocusPolicy(Qt::NoFocus);
		static_cast<QToolButton*>(_animateButton.data())->setAutoRaise(true);
		static_cast<QToolButton*>(_animateButton.data())->setToolButtonStyle(Qt::ToolButtonTextOnly);
		_animateButton->setToolTip(tr("Animate this parameter..."));
		_animateButton->setEnabled(false);
		connect(_animateButton.data(), &QAbstractButton::clicked, this, &NumericalParameterUI::openAnimationKeyEditor);
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
NumericalParameterUI::~NumericalParameterUI()
{
	// Release widgets managed by this class.
	delete label();
	delete spinner();
	delete textBox();
	delete animateButton();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to.
******************************************************************************/
void NumericalParameterUI::resetUI()
{
	if(spinner()) {
		spinner()->setEnabled(editObject() && isEnabled());
		if(editObject()) {
			ParameterUnit* unit = nullptr;
			if(parameterUnitType())
				unit = dataset()->unitsManager().getUnit(parameterUnitType());
			spinner()->setUnit(unit);
		}
		else {
			spinner()->setUnit(nullptr);
			spinner()->setFloatValue(0);
		}
	}

	if(isReferenceFieldUI() && editObject()) {
		// Update the displayed value when the animation time has changed.
		connect(dataset()->container(), &DataSetContainer::timeChanged, this, &NumericalParameterUI::updateUI, Qt::UniqueConnection);
	}

	PropertyParameterUI::resetUI();

	if(animateButton())
		animateButton()->setEnabled(editObject() && parameterObject() && isEnabled());
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void NumericalParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(spinner()) {
		if(isReferenceFieldUI()) {
			spinner()->setEnabled(parameterObject() && isEnabled());
		}
		else {
			spinner()->setEnabled(editObject() && isEnabled());
		}
	}
	if(animateButton())
		animateButton()->setEnabled(editObject() && parameterObject() && isEnabled());
}

/******************************************************************************
* Is called when the spinner value has changed.
******************************************************************************/
void NumericalParameterUI::onSpinnerValueChanged()
{
	ViewportSuspender noVPUpdate(dataset()->viewportConfig());
	if(!_isDraggingSpinner) {
		UndoableTransaction transaction(dataset()->undoStack(), tr("Change parameter"));
		updatePropertyValue();
		transaction.commit();
	}
	else {
		dataset()->undoStack().resetCurrentCompoundOperation();
		updatePropertyValue();
	}
}

/******************************************************************************
* Is called when the user begins dragging the spinner interactively.
******************************************************************************/
void NumericalParameterUI::onSpinnerDragStart()
{
	OVITO_ASSERT(!_isDraggingSpinner);
	dataset()->undoStack().beginCompoundOperation(tr("Change parameter"));
	_isDraggingSpinner = true;
}

/******************************************************************************
* Is called when the user stops dragging the spinner interactively.
******************************************************************************/
void NumericalParameterUI::onSpinnerDragStop()
{
	OVITO_ASSERT(_isDraggingSpinner);
	dataset()->undoStack().endCompoundOperation();
	_isDraggingSpinner = false;
}

/******************************************************************************
* Is called when the user aborts dragging the spinner interactively.
******************************************************************************/
void NumericalParameterUI::onSpinnerDragAbort()
{
	OVITO_ASSERT(_isDraggingSpinner);
	dataset()->undoStack().endCompoundOperation(false);
	_isDraggingSpinner = false;
}

/******************************************************************************
* Creates a QLayout that contains the text box and the spinner widget.
******************************************************************************/
QLayout* NumericalParameterUI::createFieldLayout() const
{
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	layout->addWidget(textBox());
	layout->addWidget(spinner());
	if(animateButton())
		layout->addWidget(animateButton());
	return layout;
}

}	// End of namespace
