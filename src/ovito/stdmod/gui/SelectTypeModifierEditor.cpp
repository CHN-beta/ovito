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
#include <ovito/stdobj/gui/widgets/PropertyContainerParameterUI.h>
#include <ovito/stdmod/modifiers/SelectTypeModifier.h>
#include <ovito/gui/desktop/properties/ObjectStatusDisplay.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include "SelectTypeModifierEditor.h"

namespace Ovito::StdMod {

IMPLEMENT_OVITO_CLASS(SelectTypeModifierEditor);
SET_OVITO_OBJECT_EDITOR(SelectTypeModifier, SelectTypeModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SelectTypeModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Select type"), rolloutParams, "manual:particles.modifiers.select_particle_type");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	PropertyContainerParameterUI* pclassUI = new PropertyContainerParameterUI(this, PROPERTY_FIELD(GenericPropertyModifier::subject));
	layout->addWidget(new QLabel(tr("Operate on:")));
	layout->addWidget(pclassUI->comboBox());
	pclassUI->setContainerFilter([](const PropertyContainer* container) {
		return std::any_of(container->properties().begin(), container->properties().end(), &isValidInputProperty);
	});

	_sourcePropertyUI = new PropertyReferenceParameterUI(this, PROPERTY_FIELD(SelectTypeModifier::sourceProperty));
	layout->addWidget(new QLabel(tr("Property:")));
	layout->addWidget(_sourcePropertyUI->comboBox());

	// Show only typed properties that have some element types attached to them.
	_sourcePropertyUI->setPropertyFilter(&isValidInputProperty);

	class TableWidget : public QTableView {
	public:
		using QTableView::QTableView;
		virtual QSize sizeHint() const { return QSize(256, 400); }
	};
	_elementTypesBox = new TableWidget();
	ViewModel* model = new ViewModel(this);
	_elementTypesBox->setModel(model);
	_elementTypesBox->setShowGrid(false);
	_elementTypesBox->setSelectionBehavior(QAbstractItemView::SelectRows);
	_elementTypesBox->setCornerButtonEnabled(false);
	_elementTypesBox->verticalHeader()->hide();
	_elementTypesBox->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	_elementTypesBox->setSelectionMode(QAbstractItemView::SingleSelection);
	_elementTypesBox->setWordWrap(false);
	_elementTypesBox->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	_elementTypesBox->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	_elementTypesBox->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	_elementTypesBox->verticalHeader()->setDefaultSectionSize(_elementTypesBox->verticalHeader()->minimumSectionSize());
	layout->addWidget(new QLabel(tr("Types:"), rollout));
	layout->addWidget(_elementTypesBox);

	// Double-clicking a row toggles the selection state.
	connect(_elementTypesBox, &QTableView::doubleClicked, model, [model](const QModelIndex& index) {
		QVariant value  = model->data(index.siblingAtColumn(0), Qt::CheckStateRole);
		model->setData(index.siblingAtColumn(0), (value.toInt() == Qt::Unchecked) ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
	});

	connect(this, &PropertiesEditor::contentsChanged, this, [this,model](RefTarget* editObject) {
		SelectTypeModifier* modifier = static_object_cast<SelectTypeModifier>(editObject);
		if(modifier)
			_sourcePropertyUI->setContainerRef(modifier->subject());
		else
			_sourcePropertyUI->setContainerRef({});
		QModelIndexList selection = _elementTypesBox->selectionModel()->selectedRows();
		model->refresh();
		if(!selection.empty())
			_elementTypesBox->selectRow(selection.front().row());
	});

	// Status label.
	layout->addSpacing(12);
	layout->addWidget((new ObjectStatusDisplay(this))->statusWidget());
}

/******************************************************************************
* Returns the data stored under the given role for the given RefTarget.
******************************************************************************/
QVariant SelectTypeModifierEditor::ViewModel::data(const QModelIndex& index, int role) const
{
	if(index.isValid() && index.row() < _elementTypes.size()) {
		if(role == Qt::DisplayRole) {
			if(index.column() == 0)
				return _elementTypes[index.row()]->nameOrNumericId();
			else if(index.column() == 1)
				return _elementTypes[index.row()]->numericId();
		}
		else if(role == Qt::DecorationRole) {
			if(index.column() == 0)
				return (QColor)_elementTypes[index.row()]->color();
		}
		else if(role == Qt::CheckStateRole) {
			if(index.column() == 0) {
				if(SelectTypeModifier* mod = static_object_cast<SelectTypeModifier>(editor()->editObject())) {
					const QSet<int>& selectedTypeIDs = mod->selectedTypeIDs();
					return selectedTypeIDs.contains(_elementTypes[index.row()]->numericId()) ? Qt::Checked : Qt::Unchecked;
				}
			}
		}
	}
	return {};
}

/******************************************************************************
* Returns the header data under the given role for the given RefTarget.
******************************************************************************/
QVariant SelectTypeModifierEditor::ViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if(section == 0)
			return tr("Name");
		else if(section == 1)
			return tr("Id");
	}
	return {};
}

/******************************************************************************
* Returns the item flags for the given index.
******************************************************************************/
Qt::ItemFlags SelectTypeModifierEditor::ViewModel::flags(const QModelIndex& index) const
{
	if(index.column() == 0)
		return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
	return QAbstractTableModel::flags(index);
}

/******************************************************************************
* Sets the role data for the item at index to value.
******************************************************************************/
bool SelectTypeModifierEditor::ViewModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(index.isValid() && index.row() < _elementTypes.size()) {
		if(role == Qt::CheckStateRole && index.column() == 0) {
			if(SelectTypeModifier* mod = static_object_cast<SelectTypeModifier>(editor()->editObject())) {
				QSet<int> types = mod->selectedTypeIDs();
				if(value.toInt() == Qt::Checked)
					types.insert(_elementTypes[index.row()]->numericId());
				else
					types.remove(_elementTypes[index.row()]->numericId());
				editor()->undoableTransaction(tr("Select type"), [&]() {
					mod->setSelectedTypeIDs(std::move(types));
				});
				return true;
			}
		}
	}
	return QAbstractItemModel::setData(index, value, role);
}

/******************************************************************************
* Updates the contents of the model.
******************************************************************************/
void SelectTypeModifierEditor::ViewModel::refresh()
{
	beginResetModel();
	_elementTypes.clear();

	SelectTypeModifier* mod = static_object_cast<SelectTypeModifier>(editor()->editObject());
	if(mod && mod->subject() && !mod->sourceProperty().isNull() && mod->sourceProperty().containerClass() == mod->subject().dataClass()) {

		// Populate types list based on the selected input property.
		for(ModifierApplication* modApp : editor()->modifierApplications()) {
			const PipelineFlowState& inputState = modApp->evaluateInputSynchronousAtCurrentTime();
			if(const PropertyContainer* container = inputState.getLeafObject(mod->subject())) {
				if(const PropertyObject* inputProperty = mod->sourceProperty().findInContainer(container)) {
					for(const ElementType* type : inputProperty->elementTypes()) {
						if(!type) continue;

						// Make sure we don't add the same element type twice.
						if(boost::algorithm::any_of(_elementTypes, [&](const ElementType* existingType) {
							return (existingType->numericId() == type->numericId() && existingType->name() == type->name());
						})) {
							continue;
						}

						_elementTypes.push_back(type);
					}
				}
			}
		}
	}

	endResetModel();
}

}	// End of namespace
