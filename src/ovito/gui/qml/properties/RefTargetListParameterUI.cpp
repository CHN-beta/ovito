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

#include <ovito/gui/qml/GUI.h>
#include <ovito/gui/qml/properties/RefTargetListParameterUI.h>
#include <QJSEngine>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(RefTargetListParameterUI);
DEFINE_VECTOR_REFERENCE_FIELD(RefTargetListParameterUI, targets);

/******************************************************************************
* Is called when the user has selected an item in the list/table view.
******************************************************************************/
void RefTargetListParameterUI::onSelectionChanged()
{
//	openSubEditor();
}

/******************************************************************************
* This method is called when a new editable object has loaded into the editor.
******************************************************************************/
void RefTargetListParameterUI::onEditObjectReplaced()
{
	// Get the currently selected index.
//	QModelIndexList selection = _viewWidget->selectionModel()->selectedRows();
//	int selectionIndex = !selection.empty() ? selection.front().row() : 0;

	_targets.clear(this, PROPERTY_FIELD(targets));
	_targetToRow.clear();
	_rowToTarget.clear();

	if(editObject() && propertyField()) {
		// Create a local copy of the list of ref targets.
		int count = editObject()->getVectorReferenceFieldSize(*propertyField());
		for(int i = 0; i < count; i++) {
			RefTarget* t = editObject()->getVectorReferenceFieldTarget(*propertyField(), i);
			_targetToRow.push_back(_rowToTarget.size());
			if(t != nullptr)
				_rowToTarget.push_back(_targets.size());
			_targets.push_back(this, PROPERTY_FIELD(targets), t);
		}
	}

	if(_model)
		_model->resetList();

//	selectionIndex = std::min(selectionIndex, _model->rowCount() - 1);
//	if(selectionIndex >= 0)
//		_viewWidget->selectionModel()->select(_model->index(selectionIndex, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
//	else
//		_viewWidget->selectionModel()->clear();
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
	QJSEngine::setObjectOwnership(targets()[targetIndex], QJSEngine::CppOwnership);
	return targets()[targetIndex];
}

/******************************************************************************
* Returns the RefTarget that is currently selected in the UI.
******************************************************************************/
RefTarget* RefTargetListParameterUI::selectedObject() const
{
//	if(!_viewWidget) return nullptr;
//	QModelIndexList selection = _viewWidget->selectionModel()->selectedRows();
//	if(selection.empty()) return nullptr;
//	return objectAtIndex(selection.front().row());
	return nullptr;
}

/******************************************************************************
* Selects the given sub-object in the list.
******************************************************************************/
int RefTargetListParameterUI::setSelectedObject(RefTarget* selObj)
{
#if 0
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
#endif
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
			if(refevent.field() == propertyField()) {
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
				int count = editObject()->getVectorReferenceFieldSize(*propertyField());
				for(int i = 0; i < count; i++) {
					RefTarget* t = editObject()->getVectorReferenceFieldTarget(*propertyField(), i);
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
			if(refevent.field() == propertyField()) {
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
				int count = editObject()->getVectorReferenceFieldSize(*propertyField());
				for(int i = 0; i < count; i++) {
					RefTarget* t = editObject()->getVectorReferenceFieldTarget(*propertyField(), i);
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
			if(refevent.field() == propertyField()) {
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
				int count = editObject()->getVectorReferenceFieldSize(*propertyField());
				for(int i = 0; i < count; i++) {
					RefTarget* t = editObject()->getVectorReferenceFieldTarget(*propertyField(), i);
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
			return QVariant::fromValue(static_cast<QObject*>(target));
//			return target->objectTitle();
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
