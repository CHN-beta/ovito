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
#include <ovito/gui/desktop/properties/RefTargetListParameterUI.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(RefTargetListParameterUI);
DEFINE_VECTOR_REFERENCE_FIELD(RefTargetListParameterUI, targets);

/******************************************************************************
* The constructor.
******************************************************************************/
RefTargetListParameterUI::RefTargetListParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& refField, const RolloutInsertionParameters& rolloutParams, OvitoClassPtr defaultEditorClass)
	: ParameterUI(parentEditor), _refField(refField), _rolloutParams(rolloutParams), _defaultEditorClass(defaultEditorClass)
{
	OVITO_ASSERT_MSG(refField.isVector(), "RefTargetListParameterUI constructor", "The reference field bound to this parameter UI must be a vector reference field.");

	_model = new ListViewModel(this);

	if(_defaultEditorClass)
		openSubEditor();
}

/******************************************************************************
* Destructor.
******************************************************************************/
RefTargetListParameterUI::~RefTargetListParameterUI()
{
	_subEditor = nullptr;
	clearAllReferences();

	// Release GUI controls.
	delete _viewWidget;
}

/******************************************************************************
* Returns the list view managed by this ParameterUI.
******************************************************************************/
QListView* RefTargetListParameterUI::listWidget(int listWidgetHeight)
{
	OVITO_ASSERT(!_viewWidget || qobject_cast<QListView*>(_viewWidget));
	if(!_viewWidget) {
		class MyListView : public QListView {
		private:
			int _listWidgetHeight;
		public:
			MyListView(int listWidgetHeight) : QListView(), _listWidgetHeight(listWidgetHeight) {}
			virtual QSize sizeHint() const { return QSize(320, _listWidgetHeight); }
		};

		_viewWidget = new MyListView(listWidgetHeight);
		_viewWidget->setModel(_model);
		connect(_viewWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RefTargetListParameterUI::onSelectionChanged);
	}
	return qobject_cast<QListView*>(_viewWidget);
}

/******************************************************************************
* Returns the table view managed by this ParameterUI.
******************************************************************************/
QTableView* RefTargetListParameterUI::tableWidget(int tableWidgetHeight)
{
	OVITO_ASSERT(!_viewWidget || qobject_cast<QTableView*>(_viewWidget));
	if(!_viewWidget) {
		class MyTableView : public QTableView {
		private:
			int _tableWidgetHeight;
		public:
			MyTableView(int tableWidgetHeight) : QTableView(), _tableWidgetHeight(tableWidgetHeight) {}
			virtual QSize sizeHint() const override { return QSize(320, _tableWidgetHeight); }
		};
		MyTableView* tableView = new MyTableView(tableWidgetHeight);
		tableView->setShowGrid(false);
		tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
		tableView->setCornerButtonEnabled(false);
		tableView->verticalHeader()->hide();
		tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		tableView->setSelectionMode(QAbstractItemView::SingleSelection);
		tableView->setWordWrap(false);
		tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

		_viewWidget = tableView;
		_viewWidget->setModel(_model);
		connect(_viewWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RefTargetListParameterUI::onSelectionChanged);
	}
	return qobject_cast<QTableView*>(_viewWidget);
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to. The parameter UI should react to this change appropriately and
* show the properties value for the new edit object in the UI.
******************************************************************************/
void RefTargetListParameterUI::resetUI()
{
	ParameterUI::resetUI();

	if(_viewWidget) {
		_viewWidget->setEnabled(editObject() != nullptr);

		// Get the currently selected index.
		QModelIndexList selection = _viewWidget->selectionModel()->selectedRows();
		int selectionIndex = !selection.empty() ? selection.front().row() : 0;

		_targets.clear(this, PROPERTY_FIELD(targets));
		_targetToRow.clear();
		_rowToTarget.clear();

		if(editObject()) {
			// Create a local copy of the list of ref targets.
			int count = editObject()->getVectorReferenceFieldSize(referenceField());
			for(int i = 0; i < count; i++) {
				RefTarget* t = editObject()->getVectorReferenceFieldTarget(referenceField(), i);
				_targetToRow.push_back(_rowToTarget.size());
				if(t != nullptr)
					_rowToTarget.push_back(_targets.size());
				_targets.push_back(this, PROPERTY_FIELD(targets), t);
			}
		}

		_model->resetList();

		selectionIndex = std::min(selectionIndex, _model->rowCount() - 1);
		if(selectionIndex >= 0)
			_viewWidget->selectionModel()->select(_model->index(selectionIndex, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
		else
			_viewWidget->selectionModel()->clear();
	}
	openSubEditor();
}

/******************************************************************************
* Is called when the user has selected an item in the list/table view.
******************************************************************************/
void RefTargetListParameterUI::onSelectionChanged()
{
	openSubEditor();
}

/******************************************************************************
* Opens a sub-editor for the object that is selected in the list view.
******************************************************************************/
void RefTargetListParameterUI::openSubEditor()
{
	try {
		RefTarget* selection = selectedObject();

		if(subEditor()) {
			// Close old editor if it is no longer needed.
			if(!selection || subEditor()->editObject() == nullptr ||
					subEditor()->editObject()->getOOClass() != selection->getOOClass()) {

				if(selection || &subEditor()->getOOClass() != _defaultEditorClass)
					_subEditor.reset();
			}
		}
		if(!subEditor() && editor()) {
			if(selection) {
				_subEditor = PropertiesEditor::create(selection);
			}
			else if(_defaultEditorClass) {
				_subEditor = dynamic_object_cast<PropertiesEditor>(_defaultEditorClass->createInstance());
			}
			else {
				return;
			}
			if(subEditor()) {
				subEditor()->initialize(editor()->container(), editor()->mainWindow(), _rolloutParams, editor());
			}
		}
		if(subEditor()) {
			subEditor()->setEditObject(selection);
		}
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Returns a RefTarget from the list.
******************************************************************************/
RefTarget* RefTargetListParameterUI::objectAtIndex(int index) const
{
	if(index >= _rowToTarget.size()) return nullptr;
	int targetIndex = _rowToTarget[index];
	OVITO_ASSERT(targetIndex < _targets.size());
	OVITO_CHECK_OBJECT_POINTER(targets()[targetIndex]);
	return targets()[targetIndex];
}

/******************************************************************************
* Returns the RefTarget that is currently selected in the UI.
******************************************************************************/
RefTarget* RefTargetListParameterUI::selectedObject() const
{
	if(!_viewWidget) return nullptr;
	QModelIndexList selection = _viewWidget->selectionModel()->selectedRows();
	if(selection.empty()) return nullptr;
	return objectAtIndex(selection.front().row());
}

/******************************************************************************
* Selects the given sub-object in the list.
******************************************************************************/
int RefTargetListParameterUI::setSelectedObject(RefTarget* selObj)
{
	if(!_viewWidget) return -1;
	OVITO_ASSERT(_targetToRow.size() == targets().size());
	if(selObj != nullptr) {
		for(int i = 0; i< targets().size(); i++) {
			if(targets()[i] == selObj) {
				int rowIndex = _targetToRow[i];
				_viewWidget->selectionModel()->select(_model->index(rowIndex, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
				return rowIndex;
			}
		}
	}
	_viewWidget->selectionModel()->clear();
	return -1;
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool RefTargetListParameterUI::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == editObject()) {
		if(event.type() == ReferenceEvent::ReferenceAdded) {
			const ReferenceFieldEvent& refevent = static_cast<const ReferenceFieldEvent&>(event);
			if(refevent.field() == &referenceField()) {
				int rowIndex;
				if(refevent.index() < _targetToRow.size())
					rowIndex = _targetToRow[refevent.index()];
				else
					rowIndex = _rowToTarget.size();
				if(refevent.newTarget() != nullptr)
					_model->beginInsert(rowIndex);
				_targets.insert(this, PROPERTY_FIELD(targets), refevent.index(), refevent.newTarget());
				_targetToRow.insert(refevent.index(), rowIndex);
				for(int i = rowIndex; i < _rowToTarget.size(); i++)
					_rowToTarget[i]++;
				if(refevent.newTarget() != nullptr) {
					_rowToTarget.insert(rowIndex, refevent.index());
					for(int i = refevent.index()+1; i < _targetToRow.size(); i++)
						_targetToRow[i]++;
					_model->endInsert();
				}
#ifdef OVITO_DEBUG
				// Check internal list structures.
				int numRows = 0;
				int numTargets = 0;
				int count = editObject()->getVectorReferenceFieldSize(referenceField());
				for(int i = 0; i < count; i++) {
					RefTarget* t = editObject()->getVectorReferenceFieldTarget(referenceField(), i);
					OVITO_ASSERT(targets()[numTargets] == t);
					OVITO_ASSERT(_targetToRow[numTargets] == numRows);
					if(t != nullptr) {
						OVITO_ASSERT(_rowToTarget[numRows] == numTargets);
						numRows++;
					}
					numTargets++;
				}
#endif
			}
		}
		else if(event.type() == ReferenceEvent::ReferenceRemoved) {
			const ReferenceFieldEvent& refevent = static_cast<const ReferenceFieldEvent&>(event);
			OVITO_ASSERT(false);
			if(refevent.field() == &referenceField()) {
				int rowIndex = _targetToRow[refevent.index()];
				if(refevent.oldTarget())
					_model->beginRemove(rowIndex);
				OVITO_ASSERT(refevent.oldTarget() == targets()[refevent.index()]);
				_targets.remove(this, PROPERTY_FIELD(targets), refevent.index());
				_targetToRow.remove(refevent.index());
				for(int i = rowIndex; i < _rowToTarget.size(); i++)
					_rowToTarget[i]--;
				if(refevent.oldTarget()) {
					_rowToTarget.remove(rowIndex);
					for(int i = refevent.index(); i < _targetToRow.size(); i++)
						_targetToRow[i]--;
					_model->endRemove();
				}
#ifdef OVITO_DEBUG
				// Check internal list structures.
				int numRows = 0;
				int numTargets = 0;
				int count = editObject()->getVectorReferenceFieldSize(referenceField());
				for(int i = 0; i < count; i++) {
					RefTarget* t = editObject()->getVectorReferenceFieldTarget(referenceField(), i);
					OVITO_ASSERT(targets()[numTargets] == t);
					OVITO_ASSERT(_targetToRow[numTargets] == numRows);
					if(t != NULL) {
						OVITO_ASSERT(_rowToTarget[numRows] == numTargets);
						numRows++;
					}
					numTargets++;
				}
#endif
			}
		}
		else if(event.type() == ReferenceEvent::ReferenceChanged) {
			const ReferenceFieldEvent& refevent = static_cast<const ReferenceFieldEvent&>(event);
			if(refevent.field() == &referenceField()) {
				OVITO_ASSERT(refevent.newTarget() != nullptr && refevent.oldTarget() != nullptr);
				_targets.set(this, PROPERTY_FIELD(targets), refevent.index(), refevent.newTarget());
				// Update a single item.
				int rowIndex = _targetToRow[refevent.index()];
				_model->updateItem(rowIndex);
				onSelectionChanged();
#ifdef OVITO_DEBUG
				// Check internal list structures.
				int numRows = 0;
				int numTargets = 0;
				int count = editObject()->getVectorReferenceFieldSize(referenceField());
				for(int i = 0; i < count; i++) {
					RefTarget* t = editObject()->getVectorReferenceFieldTarget(referenceField(), i);
					OVITO_ASSERT(targets()[numTargets] == t);
					OVITO_ASSERT(_targetToRow[numTargets] == numRows);
					if(t != nullptr) {
						OVITO_ASSERT(_rowToTarget[numRows] == numTargets);
						numRows++;
					}
					numTargets++;
				}
#endif
			}
		}		
	}
	else if(event.type() == ReferenceEvent::TitleChanged || event.type() == ReferenceEvent::TargetChanged) {
		OVITO_ASSERT(_targetToRow.size() == _targets.size());
		for(int i = 0; i < targets().size(); i++) {
			if(targets()[i] == source) {
				// Update a single item.
				_model->updateItem(_targetToRow[i]);
			}
		}
	}
	return ParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Returns the data stored under the given role for the given RefTarget.
* Calls the RefTargetListParameterUI::getItemData() virtual method to obtain
* the display data.
******************************************************************************/
QVariant RefTargetListParameterUI::ListViewModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid())
		return QVariant();

	if(index.row() >= owner()->_rowToTarget.size())
		return QVariant();

	int targetIndex = owner()->_rowToTarget[index.row()];
	OVITO_ASSERT(targetIndex < owner()->targets().size());

	RefTarget* t = owner()->targets()[targetIndex];
	return owner()->getItemData(t, index, role);
}

/******************************************************************************
* Returns the header data under the given role for the given RefTarget.
* Calls the RefTargetListParameterUI::getHeaderData() virtual method to obtain
* the data from the parameter UI.
******************************************************************************/
QVariant RefTargetListParameterUI::ListViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Vertical) {

		if(section >= owner()->_rowToTarget.size())
			return QVariant();

		int targetIndex = owner()->_rowToTarget[section];
		OVITO_ASSERT(targetIndex < owner()->targets().size());

		RefTarget* t = owner()->targets()[targetIndex];
		return owner()->getVerticalHeaderData(t, section, role);
	}
	else {
		return owner()->getHorizontalHeaderData(section, role);
	}
}

/******************************************************************************
* Returns the item flags for the given index.
******************************************************************************/
Qt::ItemFlags RefTargetListParameterUI::ListViewModel::flags(const QModelIndex& index) const
{
	if(!index.isValid() || index.row() >= owner()->_rowToTarget.size())
		return QAbstractItemModel::flags(index);

	int targetIndex = owner()->_rowToTarget[index.row()];
	OVITO_ASSERT(targetIndex < owner()->targets().size());

	RefTarget* t = owner()->targets()[targetIndex];
	return owner()->getItemFlags(t, index);
}

/******************************************************************************
* Sets the role data for the item at index to value.
******************************************************************************/
bool RefTargetListParameterUI::ListViewModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(!index.isValid() || index.row() >= owner()->_rowToTarget.size())
		return QAbstractItemModel::setData(index, value, role);

	int targetIndex = owner()->_rowToTarget[index.row()];
	OVITO_ASSERT(targetIndex < owner()->targets().size());

	RefTarget* t = owner()->targets()[targetIndex];
	return owner()->setItemData(t, index, value, role);
}

/******************************************************************************
* Returns the data stored under the given role for the given RefTarget.
******************************************************************************/
QVariant RefTargetListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	if(role == Qt::DisplayRole) {
		if(target) {
			return target->objectTitle();
		}
	}
	return {};
}

/******************************************************************************
* Returns the header data under the given role.
******************************************************************************/
QVariant RefTargetListParameterUI::getVerticalHeaderData(RefTarget* target, int index, int role)
{
	if(role == Qt::DisplayRole) {
		return QVariant(index);
	}
	return {};
}

/******************************************************************************
* Returns the header data under the given role.
******************************************************************************/
QVariant RefTargetListParameterUI::getHorizontalHeaderData(int index, int role)
{
	if(role == Qt::DisplayRole) {
		return QVariant(index);
	}
	return {};
}

}	// End of namespace
