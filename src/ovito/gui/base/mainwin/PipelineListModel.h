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

#pragma once


#include <ovito/gui/base/GUIBase.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/oo/RefTargetListener.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include "PipelineListItem.h"

namespace Ovito {

/**
 * This Qt model class is used to populate the QListView widget.
 */
class OVITO_GUIBASE_EXPORT PipelineListModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(Ovito::RefTarget* selectedObject READ selectedObject NOTIFY selectedItemChanged);
	Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedItemChanged);

public:

	enum ItemRoles {
		TitleRole = Qt::UserRole + 1,
		ItemTypeRole,
		CheckedRole
	};

	/// Constructor.
	PipelineListModel(DataSetContainer& datasetContainer, QObject* parent);

	/// Returns the number of list items.
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override { return _items.size(); }

	/// Returns the data associated with a list entry.
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	/// Changes the data associated with a list entry.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	/// Returns the flags for an item.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

	/// Returns the model's role names.
	virtual QHash<int, QByteArray> roleNames() const override;

	/// Discards all list items.
	void clear() {
		if(_items.empty()) return;
		beginRemoveRows(QModelIndex(), 0, _items.size() - 1);
		_items.clear();
		_selectedPipeline.setTarget(nullptr);
		endRemoveRows();
		_needListUpdate = false;
	}

	/// Returns the associated selection model.
	QItemSelectionModel* selectionModel() const { return _selectionModel; }

	/// Returns the currently selected item in the modification list.
	PipelineListItem* selectedItem() const;

	/// Returns the index of the item that is currently selected in the pipeline editor.
	int selectedIndex() const;

	/// Sets the index of the item that is currently selected in the pipeline editor.
	void setSelectedIndex(int index) { 
		if(selectedIndex() != index) {
			_selectionModel->select(this->index(index), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
		}
	}

	/// Returns the RefTarget object from the pipeline that is currently selected in the pipeline editor.
	RefTarget* selectedObject() const;

	/// Returns an item from the list model.
	PipelineListItem* item(int index) const {
		OVITO_ASSERT(index >= 0 && index < _items.size());
		return _items[index];
	}

	/// Populates the model with the given list items.
	void setItems(std::vector<OORef<PipelineListItem>> newItems);

	/// Returns the list of items.
	const std::vector<OORef<PipelineListItem>>& items() const { return _items; }

	/// Returns the type of drag and drop operations supported by the model.
	Qt::DropActions supportedDropActions() const override {
	    return Qt::MoveAction;
	}

	/// Returns the list of allowed MIME types.
	QStringList mimeTypes() const override;

	/// Returns an object that contains serialized items of data corresponding to the list of indexes specified.
	QMimeData* mimeData(const QModelIndexList& indexes) const override;

	/// Returns true if the model can accept a drop of the data.
	bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

	/// Handles the data supplied by a drag and drop operation that ended with the given action.
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

	/// Returns true if the list model is currently in a valid state.
	bool isUpToDate() const { return !_needListUpdate; }

	/// The list of currently selected PipelineSceneNode.
	PipelineSceneNode* selectedPipeline() const { return _selectedPipeline.target(); }

	/// Returns the container of the dataset being edited.
	DataSetContainer& datasetContainer() { return _datasetContainer; }

	/// Inserts the given modifiers into the modification pipeline of the selected scene nodes.
	void applyModifiers(const QVector<OORef<Modifier>>& modifiers);

	/// Sets the item in the modification list that should be selected on the next list update.
	void setNextObjectToSelect(RefTarget* obj) { _nextObjectToSelect = obj; }

	/// Sets the item in the modification list that should be selected on the next list update.
	void setNextSubObjectToSelectByTitle(const QString& title) { _nextSubObjectTitleToSelect = title; }

	/// Helper method that determines if the given object is part of more than one pipeline.
	static bool isSharedObject(RefTarget* obj);

Q_SIGNALS:

	/// This signal is emitted if a new list item has been selected, or if the currently
	/// selected item has changed.
	void selectedItemChanged();

public Q_SLOTS:

	/// Rebuilds the complete list immediately.
	void refreshList();

	/// Updates the appearance of a single list item.
	void refreshItem(PipelineListItem* item);

	/// Rebuilds the list of modification items as soon as possible.
	void requestUpdate() {
		if(_needListUpdate) return;	// Update is already pending.
		_needListUpdate = true;
		// Invoke actual refresh function at some later time.
		QMetaObject::invokeMethod(this, "refreshList", Qt::QueuedConnection);
	}

	/// Deletes the modifier at the given list index from the pipeline.
	void deleteModifier(int index);

private Q_SLOTS:

	/// Is called by the system when the animated status icon changed.
	void iconAnimationFrameChanged();

	/// Handles notification events generated by the selected pipeline node.
	void onPipelineEvent(const ReferenceEvent& event);

private:

	/// Create the pipeline editor entries for the subjects of the given object (and their subobjects).
	static void createListItemsForSubobjects(const DataObject* dataObj, std::vector<OORef<PipelineListItem>>& items, PipelineListItem* parentItem);

	/// List of visible items in the model.
	std::vector<OORef<PipelineListItem>> _items;

	/// Holds reference to the currently selected PipelineSceneNode.
	RefTargetListener<PipelineSceneNode> _selectedPipeline;

	/// The item in the list that should be selected on the next list update.
	RefTarget* _nextObjectToSelect = nullptr;

	/// The sub-object to select in the pipeline editor during the next refresh.
	QString _nextSubObjectTitleToSelect;

	/// The selection model of the list view widget.
	QItemSelectionModel* _selectionModel;

	/// Indicates that the list of items needs to be updated.
	bool _needListUpdate = false;

	// Status icons:
	QPixmap _statusInfoIcon;
	QPixmap _statusWarningIcon;
	QPixmap _statusErrorIcon;
	QPixmap _statusNoneIcon;
	QMovie _statusPendingIcon;

	/// Font used for section headers.
	QFont _sectionHeaderFont;

	/// Font used to highlight shared pipeline objects.
	QFont _sharedObjectFont;

	/// The background brush used for list section headers.
	QBrush _sectionHeaderBackgroundBrush;

	/// The foreground brush used for list section headers.
	QBrush _sectionHeaderForegroundBrush;

	/// Container of the dataset being edited.
	DataSetContainer& _datasetContainer;
};

}	// End of namespace
