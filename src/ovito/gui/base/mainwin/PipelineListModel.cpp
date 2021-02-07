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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>
#include "PipelineListModel.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineListModel::PipelineListModel(DataSetContainer& datasetContainer, QObject* parent) : QAbstractListModel(parent),
	_datasetContainer(datasetContainer),
	_statusInfoIcon(":/gui/mainwin/status/status_info.png"),
	_statusWarningIcon(":/gui/mainwin/status/status_warning.png"),
	_statusErrorIcon(":/gui/mainwin/status/status_error.png"),
	_statusNoneIcon(":/gui/mainwin/status/status_none.png"),
	_statusPendingIcon(":/gui/mainwin/status/status_pending.gif"),
	_sectionHeaderFont(QGuiApplication::font())
{
	connect(&_selectedPipeline, &RefTargetListener<PipelineSceneNode>::notificationEvent, this, &PipelineListModel::onPipelineEvent);
	connect(&_datasetContainer, &DataSetContainer::selectionChangeComplete, this, &PipelineListModel::refreshList);

	_statusPendingIcon.setCacheMode(QMovie::CacheAll);
	connect(&_statusPendingIcon, &QMovie::frameChanged, this, &PipelineListModel::iconAnimationFrameChanged);
	_selectionModel = new QItemSelectionModel(this);
	connect(_selectionModel, &QItemSelectionModel::selectionChanged, this, &PipelineListModel::selectedItemChanged, Qt::QueuedConnection);
	if(_sectionHeaderFont.pixelSize() < 0)
		_sectionHeaderFont.setPointSize(_sectionHeaderFont.pointSize() * 4 / 5);
	else
		_sectionHeaderFont.setPixelSize(_sectionHeaderFont.pixelSize() * 4 / 5);
	_sectionHeaderBackgroundBrush = QBrush(Qt::lightGray, Qt::Dense4Pattern);
	_sectionHeaderForegroundBrush = QBrush(Qt::blue);

	_sharedObjectFont.setItalic(true);
}

/******************************************************************************
* Populates the model with the given list items.
******************************************************************************/
void PipelineListModel::setItems(std::vector<OORef<PipelineListItem>> newItems)
{
	size_t oldCount = _items.size();
	if(newItems.size() > oldCount) {
		beginInsertRows(QModelIndex(), oldCount, newItems.size() - 1);
		_items.insert(_items.end(), std::make_move_iterator(newItems.begin() + oldCount), std::make_move_iterator(newItems.end()));
		endInsertRows();
	}
	else if(newItems.size() < oldCount) {
		beginRemoveRows(QModelIndex(), newItems.size(), oldCount - 1);
		_items.erase(_items.begin() + newItems.size(), _items.end());
		endRemoveRows();
	}
	for(size_t i = 0; i < newItems.size() && i < oldCount; i++) {
		swap(_items[i], newItems[i]);
		if(_items[i]->object() != newItems[i]->object() || _items[i]->itemType() != newItems[i]->itemType()) {
			Q_EMIT dataChanged(index(i), index(i));
		}
	}
	for(PipelineListItem* item : _items) {
		connect(item, &PipelineListItem::itemChanged, this, &PipelineListModel::refreshItem);
		connect(item, &PipelineListItem::subitemsChanged, this, &PipelineListModel::requestUpdate);
	}
}

/******************************************************************************
* Returns the currently selected item in the modification list.
******************************************************************************/
PipelineListItem* PipelineListModel::selectedItem() const
{
	int index = selectedIndex();
	if(index == -1)
		return nullptr;
	else
		return item(index);
}

/******************************************************************************
* Returns the index of the item that is currently selected in the pipeline editor.
******************************************************************************/
int PipelineListModel::selectedIndex() const
{
	QModelIndexList selection = _selectionModel->selectedRows();
	if(selection.empty())
		return -1;
	else
		return selection.front().row();
}

/******************************************************************************
* Returns the RefTarget object from the pipeline that is currently selected in the pipeline editor.
******************************************************************************/
RefTarget* PipelineListModel::selectedObject() const
{
	if(PipelineListItem* item = selectedItem())
		return item->object();
	return nullptr;
}

/******************************************************************************
* Completely rebuilds the pipeline list.
******************************************************************************/
void PipelineListModel::refreshList()
{
	_needListUpdate = false;

	// Determine the currently selected object and
	// select it again after the list has been rebuilt (and it is still there).
	// If _nextObjectToSelect is already non-null then the caller
	// has specified an object to be selected.
	QString nextObjectTitleToSelect;
	if(_nextObjectToSelect == nullptr) {
		if(PipelineListItem* item = selectedItem()) {
			_nextObjectToSelect = item->object();
		}
	}
	RefTarget* defaultObjectToSelect = nullptr;

	// Determine the selected pipeline.
	_selectedPipeline.setTarget(nullptr);
    if(_datasetContainer.currentSet()) {
		SelectionSet* selectionSet = _datasetContainer.currentSet()->selection();
		_selectedPipeline.setTarget(dynamic_object_cast<PipelineSceneNode>(selectionSet->firstNode()));
    }

	std::vector<OORef<PipelineListItem>> newItems;
	if(selectedPipeline()) {

		// Create list items for visualization elements.
		for(DataVis* vis : selectedPipeline()->visElements())
			newItems.push_back(new PipelineListItem(vis, PipelineListItem::VisualElement));
		if(!newItems.empty())
			newItems.insert(newItems.begin(), new PipelineListItem(nullptr, PipelineListItem::VisualElementsHeader));

		// Traverse the modifiers in the pipeline.
		PipelineObject* pipelineObject = selectedPipeline()->dataProvider();
		PipelineObject* firstPipelineObj = pipelineObject;
		while(pipelineObject) {

			// Create entries for the modifier applications.
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(pipelineObject)) {

				if(pipelineObject == firstPipelineObj)
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::ModificationsHeader));

				if(pipelineObject->isPipelineBranch(true))
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));

				PipelineListItem* item = new PipelineListItem(modApp, PipelineListItem::Modifier);
				newItems.push_back(item);

				pipelineObject = modApp->input();
			}
			else if(pipelineObject) {

				if(pipelineObject->isPipelineBranch(true))
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));

				newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::DataSourceHeader));

				// Create a list item for the data source.
				PipelineListItem* item = new PipelineListItem(pipelineObject, PipelineListItem::DataSource);
				newItems.push_back(item);
				if(defaultObjectToSelect == nullptr)
					defaultObjectToSelect = pipelineObject;

				// Create list items for the source's editable data objects.
				if(const DataCollection* collection = pipelineObject->getSourceDataCollection()) {
					createListItemsForSubobjects(collection, newItems, item);
				}

				// Done.
				break;
			}
		}
	}

	int selIndex = -1;
	int selDefaultIndex = -1;
	int selTitleIndex = -1;
	for(int i = 0; i < newItems.size(); i++) {
		if(_nextObjectToSelect && _nextObjectToSelect == newItems[i]->object())
			selIndex = i;
		if(_nextSubObjectTitleToSelect.isEmpty() == false && _nextSubObjectTitleToSelect == newItems[i]->title())
			selTitleIndex = i;
		if(defaultObjectToSelect && defaultObjectToSelect == newItems[i]->object())
			selDefaultIndex = i;
	}
	if(selIndex == -1)
		selIndex = selTitleIndex;
	if(selIndex == -1)
		selIndex = selDefaultIndex;

	setItems(std::move(newItems));
	_nextObjectToSelect = nullptr;
	_nextSubObjectTitleToSelect.clear();

	// Select the right item in the list.
	if(!items().empty()) {
		if(selIndex == -1) {
			for(int index = 0; index < items().size(); index++) {
				if(item(index)->object()) {
					selIndex = index;
					break;
				}
			}
		}
		if(selIndex != -1 && item(selIndex)->isSubObject())
			_nextSubObjectTitleToSelect = item(selIndex)->title();
		_selectionModel->select(index(selIndex), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
	Q_EMIT selectedItemChanged();
}

/******************************************************************************
* Create the pipeline editor entries for the subjects of the given
* object (and their subobjects).
******************************************************************************/
void PipelineListModel::createListItemsForSubobjects(const DataObject* dataObj, std::vector<OORef<PipelineListItem>>& items, PipelineListItem* parentItem)
{
	if(dataObj->showInPipelineEditor() && dataObj->editableProxy()) {
		items.push_back(parentItem = new PipelineListItem(dataObj->editableProxy(), PipelineListItem::DataObject, parentItem));
	}

	// Recursively visit the sub-objects of the data object.
	dataObj->visitSubObjects([&](const DataObject* subObject) {
		createListItemsForSubobjects(subObject, items, parentItem);
		return false;
	});
}

/******************************************************************************
* Handles notification events generated by the selected pipeline node.
******************************************************************************/
void PipelineListModel::onPipelineEvent(const ReferenceEvent& event)
{
	// Update the entire modification list if the PipelineSceneNode has been assigned a new
	// data object, or if the list of visual elements has changed.
	if(event.type() == ReferenceEvent::ReferenceChanged
		|| event.type() == ReferenceEvent::ReferenceAdded
		|| event.type() == ReferenceEvent::ReferenceRemoved
		|| event.type() == ReferenceEvent::PipelineChanged)
	{
		requestUpdate();
	}
}

/******************************************************************************
* Updates the appearance of a single list item.
******************************************************************************/
void PipelineListModel::refreshItem(PipelineListItem* item)
{
	OVITO_CHECK_OBJECT_POINTER(item);
	auto iter = std::find(items().begin(), items().end(), item);
	if(iter != items().end()) {
		int i = iter - items().begin();
		Q_EMIT dataChanged(index(i), index(i));

		// Also update available actions if the changed item is currently selected.
		if(selectedItem() == item)
			Q_EMIT selectedItemChanged();
	}
}

/******************************************************************************
* Inserts the given modifier into the modification pipeline of the
* currently selected scene nodes.
******************************************************************************/
void PipelineListModel::applyModifiers(const QVector<OORef<Modifier>>& modifiers)
{
	if(modifiers.empty() || !selectedPipeline())
		return;

	// Get the selected pipeline entry. The new modifier is inserted right behind it in the pipeline.
	PipelineListItem* currentItem = selectedItem();

	if(currentItem) {
		while(currentItem->parent()) {
			currentItem = currentItem->parent();
		}
		if(OORef<PipelineObject> pobj = dynamic_object_cast<PipelineObject>(currentItem->object())) {
			for(int i = modifiers.size() - 1; i >= 0; i--) {
				Modifier* modifier = modifiers[i];
				std::vector<OORef<RefMaker>> dependentsList;
				pobj->visitDependents([&](RefMaker* dependent) {
					if(dynamic_object_cast<ModifierApplication>(dependent) || dynamic_object_cast<PipelineSceneNode>(dependent)) {
						dependentsList.push_back(dependent);
					}
				});
				OORef<ModifierApplication> modApp = modifier->createModifierApplication();
				modApp->setModifier(modifier);
				modApp->setInput(pobj);
				modifier->initializeModifier(modApp->dataset()->animationSettings()->time(), modApp, Application::instance()->executionContext());
				setNextObjectToSelect(modApp);
				for(RefMaker* dependent : dependentsList) {
					if(ModifierApplication* predecessorModApp = dynamic_object_cast<ModifierApplication>(dependent)) {
						predecessorModApp->setInput(modApp);
					}
					else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
						pipeline->setDataProvider(modApp);
					}
				}
				pobj = modApp;
			}
			return;
		}
	}

	// Insert modifiers at the end of the selected pipelines.
	for(int index = modifiers.size() - 1; index >= 0; --index) {
		setNextObjectToSelect(selectedPipeline()->applyModifier(modifiers[index]));
	}
}

/******************************************************************************
* Deletes the modifier at the given list index from the pipeline.
******************************************************************************/
void PipelineListModel::deleteModifier(int index)
{
	// Get the modifier list item.
	PipelineListItem* selectedItem = item(index);
	if(!selectedItem) return;

	OORef<ModifierApplication> modApp = dynamic_object_cast<ModifierApplication>(selectedItem->object());
	if(!modApp) return;

	UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Delete modifier"), [modApp, this]() {
		modApp->visitDependents([&](RefMaker* dependent) {
			if(ModifierApplication* precedingModApp = dynamic_object_cast<ModifierApplication>(dependent)) {
				if(precedingModApp->input() == modApp) {
					precedingModApp->setInput(modApp->input());
					setNextObjectToSelect(modApp->input());
				}
			}
			else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
				if(pipeline->dataProvider() == modApp) {
					pipeline->setDataProvider(modApp->input());
					setNextObjectToSelect(pipeline->dataProvider());
				}
			}
		});
		OORef<Modifier> modifier = modApp->modifier();
		modApp->setInput(nullptr);
		modApp->setModifier(nullptr);

		// Delete modifier if there are no more applications left.
		if(modifier->modifierApplications().empty())
			modifier->deleteReferenceObject();
	});	
}

/******************************************************************************
* Is called by the system when the animated status icon changed.
******************************************************************************/
void PipelineListModel::iconAnimationFrameChanged()
{
	bool stopMovie = true;
	for(int i = 0; i < items().size(); i++) {
		if(item(i)->isObjectActive()) {
			dataChanged(index(i), index(i), { Qt::DecorationRole });
			stopMovie = false;
		}
	}
	if(stopMovie)
		_statusPendingIcon.stop();
}

/******************************************************************************
* Returns the data for the QListView widget.
******************************************************************************/
QVariant PipelineListModel::data(const QModelIndex& index, int role) const
{
	OVITO_ASSERT(index.row() >= 0 && index.row() < _items.size());

	PipelineListItem* item = this->item(index.row());

	if(role == Qt::DisplayRole || role == TitleRole) {
		return item->title();
	}
	else if(role == Qt::EditRole) {
		return item->title();
	}
	else if(role == ItemTypeRole) {
		return item->itemType();
	}
	else if(role == Qt::DecorationRole) {
		if(item->isObjectActive()) {
			const_cast<QMovie&>(_statusPendingIcon).start();
			return QVariant::fromValue(_statusPendingIcon.currentPixmap());
		}
		else if(item->isObjectItem()) {
			switch(item->status().type()) {
			case PipelineStatus::Warning: return QVariant::fromValue(_statusWarningIcon);
			case PipelineStatus::Error: return QVariant::fromValue(_statusErrorIcon);
			default: return QVariant::fromValue(_statusNoneIcon);
			}
		}
	}
	else if(role == Qt::ToolTipRole) {
		return QVariant::fromValue(item->status().text());
	}
	else if(role == Qt::CheckStateRole || role == CheckedRole) {
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object()))
			return vis->isEnabled() ? Qt::Checked : Qt::Unchecked;
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object()))
			return (modApp->modifier() && modApp->modifier()->isEnabled()) ? Qt::Checked : Qt::Unchecked;
		else if(role == CheckedRole)
			return false;
	}
	else if(role == Qt::TextAlignmentRole) {
		if(!item->isObjectItem()) {
			return Qt::AlignCenter;
		}
	}
	else if(role == Qt::BackgroundRole) {
		if(!item->isObjectItem()) {
			if(item->itemType() != PipelineListItem::PipelineBranch)
				return _sectionHeaderBackgroundBrush;
			else
				return QBrush(Qt::lightGray, Qt::Dense6Pattern);
		}
	}
	else if(role == Qt::ForegroundRole) {
		if(!item->isObjectItem()) {
			return _sectionHeaderForegroundBrush;
		}
	}
	else if(role == Qt::FontRole) {
		if(!item->isObjectItem())
			return _sectionHeaderFont;
		else if(isSharedObject(item->object()))
			return _sharedObjectFont;
	}

	return {};
}

/******************************************************************************
* Changes the data associated with a list entry.
******************************************************************************/
bool PipelineListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::CheckStateRole || role == CheckedRole) {
		PipelineListItem* item = this->item(index.row());
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value.toBool()) ? tr("Enable visual element") : tr("Disable visual element"), [vis, &value]() {
				vis->setEnabled(value.toBool());
			});
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value.toBool()) ? tr("Enable modifier") : tr("Disable modifier"), [modApp, &value]() {
				if(modApp->modifier()) 
					modApp->modifier()->setEnabled(value.toBool());
			});
		}
	}
	else if(role == Qt::EditRole) {
		PipelineListItem* item = this->item(index.row());
		if(DataVis* vis = dynamic_object_cast<DataVis>(item->object())) {
			QString newName = value.toString();
			if(vis->objectTitle() != newName) {
				UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Rename visual element"), [vis, &newName]() {
					vis->setObjectTitle(newName);
				});
			}
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			QString newName = value.toString();
			if(modApp->modifier() && modApp->modifier()->objectTitle() != newName) {
				UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Rename modifier"), [modApp, &newName]() {
					modApp->modifier()->setObjectTitle(newName);
				});
			}
		}
	}
	return QAbstractListModel::setData(index, value, role);
}

/******************************************************************************
* Returns the flags for an item.
******************************************************************************/
Qt::ItemFlags PipelineListModel::flags(const QModelIndex& index) const
{
	if(index.row() >= 0 && index.row() < _items.size()) {
		switch(this->item(index.row())->itemType()) {
			case PipelineListItem::VisualElement:
			case PipelineListItem::Modifier:
				return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
			case PipelineListItem::DataSource:
			case PipelineListItem::DataObject:
				return QAbstractListModel::flags(index);
			default:
				return Qt::NoItemFlags;
		}
	}
	return QAbstractListModel::flags(index);
}

/******************************************************************************
* Returns the model's role names.
******************************************************************************/
QHash<int, QByteArray> PipelineListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[TitleRole] = "title";
	roles[ItemTypeRole] = "type";
	roles[CheckedRole] = "ischecked";
	return roles;
}

/******************************************************************************
* Returns the list of allowed MIME types.
******************************************************************************/
QStringList PipelineListModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("application/ovito.modifier.list");
}

/******************************************************************************
* Returns an object that contains serialized items of data corresponding to the
* list of indexes specified.
******************************************************************************/
QMimeData* PipelineListModel::mimeData(const QModelIndexList& indexes) const
{
	QByteArray encodedData;
	QDataStream stream(&encodedData, QIODevice::WriteOnly);
	for(const QModelIndex& index : indexes) {
		if(index.isValid()) {
			stream << index.row();
		}
	}
	std::unique_ptr<QMimeData> mimeData(new QMimeData());
	mimeData->setData(QStringLiteral("application/ovito.modifier.list"), encodedData);
	return mimeData.release();
}

/******************************************************************************
* Returns true if the model can accept a drop of the data.
******************************************************************************/
bool PipelineListModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
	if(!data->hasFormat(QStringLiteral("application/ovito.modifier.list")))
		return false;

	if(column > 0)
		return false;

	return true;
}

/******************************************************************************
* Handles the data supplied by a drag and drop operation that ended with the
* given action.
******************************************************************************/
bool PipelineListModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	if(!canDropMimeData(data, action, row, column, parent))
		return false;

	if(action == Qt::IgnoreAction)
		return true;

	if(row == -1 && parent.isValid())
		row = parent.row();
	if(row == -1)
		return false;

    QByteArray encodedData = data->data(QStringLiteral("application/ovito.modifier.list"));
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QVector<int> indexList;
    while(!stream.atEnd()) {
    	int index;
    	stream >> index;
    	indexList.push_back(index);
    }
    if(indexList.size() != 1)
    	return false;

	// The list item being dragged.
    PipelineListItem* movedItem = item(indexList[0]);
//	if(!movedItem->modifierApplication())
//		return false;

#if 0
	// The ModifierApplication being dragged.
	OORef<ModifierApplication> modApp = movedItem->modifierApplications()[0];
	int indexDelta = -(row - indexList[0]);

	UndoableTransaction::handleExceptions(modApp->dataset()->undoStack(), tr("Move modifier"), [movedItem, modApp, indexDelta]() {
		// Determine old position in list.
		int index = _items.indexOf(movedItem);
		if(indexDelta == 0 || index + indexDelta < 0 || index+indexDelta >= pipelineObj->modifierApplications().size())
			return;
		// Remove ModifierApplication from the PipelineObject.
		pipelineObj->removeModifierApplication(index);
		// Re-insert ModifierApplication into the PipelineObject.
		pipelineObj->insertModifierApplication(index + indexDelta, modApp);
	});
#endif

	return true;
}

/******************************************************************************
* Helper method that determines if the given object is part of more than one pipeline.
******************************************************************************/
bool PipelineListModel::isSharedObject(RefTarget* obj)
{
	if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj)) {
		if(modApp->modifier()) {
			QSet<PipelineSceneNode*> pipelines;
			for(ModifierApplication* ma : modApp->modifier()->modifierApplications())
				pipelines.unite(ma->pipelines(true));
			return pipelines.size() > 1;
		}
	}
	else if(PipelineObject* pipelineObject = dynamic_object_cast<PipelineObject>(obj)) {
		return pipelineObject->pipelines(true).size() > 1;
	}
	else if(DataVis* visElement = dynamic_object_cast<DataVis>(obj)) {
		return visElement->pipelines(true).size() > 1;
	}
	return false;
}

}	// End of namespace
