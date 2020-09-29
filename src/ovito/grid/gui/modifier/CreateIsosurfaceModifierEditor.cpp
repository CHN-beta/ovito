////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include <ovito/stdobj/gui/widgets/PropertyReferenceParameterUI.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/IntegerParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/SubObjectParameterUI.h>
#include <ovito/grid/modifier/CreateIsosurfaceModifier.h>
#include "CreateIsosurfaceModifierEditor.h"

#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_picker_machine.h>

namespace Ovito { namespace Grid {

IMPLEMENT_OVITO_CLASS(CreateIsosurfaceModifierEditor);
SET_OVITO_OBJECT_EDITOR(CreateIsosurfaceModifier, CreateIsosurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CreateIsosurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Create isosurface"), rolloutParams, "particles.modifiers.create_isosurface.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(4);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::subject));
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return VoxelGrid::OOClass().isMember(container);
	});
	layout2->addWidget(new QLabel(tr("Operate on:")), 0, 0);
	layout2->addWidget(pclassUI->comboBox(), 0, 1);

	PropertyReferenceParameterUI* fieldQuantityUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::sourceProperty), nullptr);
	layout2->addWidget(new QLabel(tr("Field quantity:")), 1, 0);
	layout2->addWidget(fieldQuantityUI->comboBox(), 1, 1);
	connect(this, &PropertiesEditor::contentsChanged, this, [fieldQuantityUI](RefTarget* editObject) {
		if(CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(editObject)) {
			fieldQuantityUI->setContainerRef(modifier->subject());
		}
		else
			fieldQuantityUI->setContainerRef(nullptr);
	});

	// Isolevel parameter.
	FloatParameterUI* isolevelPUI = new FloatParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::isolevelController));
	layout2->addWidget(isolevelPUI->label(), 2, 0);
	layout2->addLayout(isolevelPUI->createFieldLayout(), 2, 1);

	// Transfer field values.
	BooleanParameterUI* transferFieldValuesUI = new BooleanParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::transferFieldValues));
	layout2->addWidget(transferFieldValuesUI->checkBox(), 3, 0, 1, 2);

	_plotWidget = new StdObj::DataTablePlotWidget();
	_plotWidget->setMinimumHeight(200);
	_plotWidget->setMaximumHeight(200);
	_isoLevelIndicator = new QwtPlotMarker();
	_isoLevelIndicator->setLineStyle(QwtPlotMarker::VLine);
	_isoLevelIndicator->setLinePen(Qt::blue, 1, Qt::DashLine);
	_isoLevelIndicator->setZ(1);
	_isoLevelIndicator->attach(_plotWidget);
	_isoLevelIndicator->hide();
	QwtPlotPicker* picker = new QwtPlotPicker(_plotWidget->canvas());
	OVITO_ASSERT(picker->isEnabled());
	picker->setTrackerMode(QwtPlotPicker::AlwaysOff);
	picker->setStateMachine(new QwtPickerDragPointMachine());
	connect(picker, qOverload<const QPointF&>(&QwtPlotPicker::appended), this, &CreateIsosurfaceModifierEditor::onPickerPoint);
	connect(picker, qOverload<const QPointF&>(&QwtPlotPicker::moved), this, &CreateIsosurfaceModifierEditor::onPickerPoint);
	connect(picker, &QwtPicker::activated, this, &CreateIsosurfaceModifierEditor::onPickerActivated);
	connect(this, &PropertiesEditor::contentsReplaced, this, [this]() { onPickerActivated(false); });

	layout2->addWidget(new QLabel(tr("Histogram:")), 4, 0, 1, 2);
	layout2->addWidget(_plotWidget, 5, 0, 1, 2);

	// Status label.
	layout1->addSpacing(8);
	layout1->addWidget(statusLabel());

	// Open a sub-editor for the mesh vis element.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::surfaceMeshVis), rolloutParams.after(rollout));

	// Update data plot whenever the modifier has calculated new results.
	connect(this, &ModifierPropertiesEditor::contentsReplaced, this, &CreateIsosurfaceModifierEditor::plotHistogram);
	connect(this, &ModifierPropertiesEditor::modifierEvaluated, this, [this]() {
		plotHistogramLater(this);
	});
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void CreateIsosurfaceModifierEditor::plotHistogram()
{
	CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(editObject());

	if(modifier && modifierApplication()) {
		_isoLevelIndicator->setXValue(modifier->isolevel());
		_isoLevelIndicator->show();

		// Request the modifier's pipeline output.
		const PipelineFlowState& state = getModifierOutput();

		// Look up the generated data table in the modifier's pipeline output.
		const DataTable* table = state.getObjectBy<DataTable>(modifierApplication(), QStringLiteral("isosurface-histogram"));
		_plotWidget->setTable(table);
	}
	else {
		_isoLevelIndicator->hide();
		_plotWidget->reset();
	}
}

/******************************************************************************
* Is called when the user starts or stops picking a location in the plot widget.
******************************************************************************/
void CreateIsosurfaceModifierEditor::onPickerActivated(bool on)
{
	if(on) {
		if(CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(editObject())) {
			dataset()->undoStack().beginCompoundOperation(tr("Change iso-value"));
			_interactionInProgress = true;
		}
	}
	else {
		if(_interactionInProgress) {
			dataset()->undoStack().endCompoundOperation(true);
			_interactionInProgress = false;
		}
	}
}

/******************************************************************************
* Is called when the user picks a location in the plot widget.
******************************************************************************/
void CreateIsosurfaceModifierEditor::onPickerPoint(const QPointF& pt)
{
	if(CreateIsosurfaceModifier* modifier = static_object_cast<CreateIsosurfaceModifier>(editObject())) {
		OVITO_ASSERT(_interactionInProgress);
		dataset()->undoStack().resetCurrentCompoundOperation();
		modifier->setIsolevel(pt.x());
	}
}

}	// End of namespace
}	// End of namespace
