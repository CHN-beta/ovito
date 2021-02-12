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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/gui/base/actions/ActionManager.h>
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
PipelineListModel::PipelineListModel(DataSetContainer& datasetContainer, ActionManager* actionManager, QObject* parent) : QAbstractListModel(parent),
	_datasetContainer(datasetContainer),
	_statusInfoIcon(":/gui/mainwin/status/status_info.png"),
	_statusWarningIcon(":/gui/mainwin/status/status_warning.png"),
	_statusErrorIcon(":/gui/mainwin/status/status_error.png"),
	_statusNoneIcon(":/gui/mainwin/status/status_none.png"),
	_statusPendingIcon(":/gui/mainwin/status/status_pending.gif"),
	_sectionHeaderFont(QGuiApplication::font()),
	_modifierGroupCollapsed(":/guibase/actions/modify/modifier_group_collapsed.svg"),
	_modifierGroupExpanded(":/guibase/actions/modify/modifier_group_expanded.svg")
{
	// Create a selection model.
	_selectionModel = new QItemSelectionModel(this);

	// Connect signals and slots.
	connect(&_selectedPipeline, &RefTargetListener<PipelineSceneNode>::notificationEvent, this, &PipelineListModel::onPipelineEvent);
	connect(&_datasetContainer, &DataSetContainer::selectionChangeComplete, this, &PipelineListModel::refreshList);
	connect(_selectionModel, &QItemSelectionModel::selectionChanged, this, &PipelineListModel::selectedItemChanged, Qt::QueuedConnection);
	connect(this, &PipelineListModel::selectedItemChanged, this, &PipelineListModel::updateActions);

	// Set up list item fonts, icons and colors.
	_statusPendingIcon.setCacheMode(QMovie::CacheAll);
	connect(&_statusPendingIcon, &QMovie::frameChanged, this, &PipelineListModel::iconAnimationFrameChanged);
	if(_sectionHeaderFont.pixelSize() < 0)
		_sectionHeaderFont.setPointSize(_sectionHeaderFont.pointSize() * 4 / 5);
	else
		_sectionHeaderFont.setPixelSize(_sectionHeaderFont.pixelSize() * 4 / 5);
	_sectionHeaderBackgroundBrush = QBrush(Qt::lightGray, Qt::Dense4Pattern);
	_sectionHeaderForegroundBrush = QBrush(Qt::blue);
	_disabledForegroundBrush = QGuiApplication::palette().brush(QPalette::Disabled, QPalette::Text);
	_sharedObjectFont.setItalic(true);

	// Create list item actions.
	_deleteItemAction = actionManager->createCommandAction(ACTION_MODIFIER_DELETE, tr("Delete Modifier"), ":/guibase/actions/modify/delete_modifier.bw.svg", tr("Delete the selected modifier from the pipeline."));
	connect(_deleteItemAction, &QAction::triggered, this, &PipelineListModel::deleteSelectedItem);
	_moveItemUpAction = actionManager->createCommandAction(ACTION_MODIFIER_MOVE_UP, tr("Move Modifier Up"), ":/guibase/actions/modify/modifier_move_up.bw.svg", tr("Move the selected modifier up in the pipeline."));
	connect(_moveItemUpAction, &QAction::triggered, this, &PipelineListModel::moveModifierUp);
	_moveItemDownAction = actionManager->createCommandAction(ACTION_MODIFIER_MOVE_DOWN, tr("Move Modifier Down"), ":/guibase/actions/modify/modifier_move_down.bw.svg", tr("Move the selected modifier down in the pipeline."));
	connect(_moveItemDownAction, &QAction::triggered, this, &PipelineListModel::moveModifierDown);
	_toggleModifierGroupAction = actionManager->createCommandAction(ACTION_PIPELINE_TOGGLE_MODIFIER_GROUP, tr("Group Modifiers"), ":/guibase/actions/modify/modifier_group_create.svg", tr("Creates or dissolves a group of modifiers in the pipeline editor."));
	_toggleModifierGroupAction->setCheckable(true);
	connect(_toggleModifierGroupAction, &QAction::triggered, this, &PipelineListModel::toggleModifierGroup);
	_makeElementIndependentAction = actionManager->createCommandAction(ACTION_PIPELINE_MAKE_INDEPENDENT, tr("Replace With Independent Copy"), ":/guibase/actions/modify/make_element_independent.bw.svg", tr("Duplicate an entry that is shared by multiple pipelines."));
	connect(_makeElementIndependentAction, &QAction::triggered, this, &PipelineListModel::makeElementIndependent);
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
		connect(item, &PipelineListItem::subitemsChanged, this, &PipelineListModel::refreshListLater);
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
	_listRefreshPending = false;

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
		ModifierGroup* currentGroup = nullptr;
		while(pipelineObject) {

			// Create entries for the modifier applications.
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(pipelineObject)) {

				if(pipelineObject == firstPipelineObj)
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::ModificationsHeader));

				if(pipelineObject->isPipelineBranch(true))
					newItems.push_back(new PipelineListItem(nullptr, PipelineListItem::PipelineBranch));

				if(modApp->modifierGroup() != currentGroup) {
					if(modApp->modifierGroup())
						newItems.push_back(new PipelineListItem(modApp->modifierGroup(), PipelineListItem::ModifierGroup));
					currentGroup = modApp->modifierGroup();
				}

				if(!currentGroup || !currentGroup->isCollapsed())
					newItems.push_back(new PipelineListItem(modApp, PipelineListItem::Modifier));

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
		refreshListLater();
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
		int i = std::distance(items().begin(), iter);
		Q_EMIT dataChanged(index(i), index(i));

		// Also update available actions if the changed item is currently selected.
		if(selectedItem() == item)
			Q_EMIT selectedItemChanged();
	}
}

/******************************************************************************
* Inserts the given modifier(s) into the currently selected pipeline.
******************************************************************************/
void PipelineListModel::applyModifiers(const QVector<OORef<Modifier>>& modifiers, ModifierGroup* group)
{
	if(modifiers.empty() || !selectedPipeline())
		return;

	// Get the selected pipeline item. The new modifier is inserted right behind it in the pipeline.
	PipelineListItem* currentItem = selectedItem();

	if(currentItem) {
		while(currentItem->parent()) {
			currentItem = currentItem->parent();
		}
		
		RefTarget* selectedObject = currentItem->object();
		if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(selectedObject)) {
			selectedObject = group->modifierApplications().first();
		}

		if(OORef<PipelineObject> pobj = dynamic_object_cast<PipelineObject>(selectedObject)) {
			
			ModifierGroup* modifierGroup = nullptr;
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(selectedObject)) {
				if(selectedObject == currentItem->object())
					modifierGroup = modApp->modifierGroup();
			}
			if(!modifierGroup)
				modifierGroup = group;

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
				modApp->setModifierGroup(modifierGroup);
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
			if(group)
				setNextObjectToSelect(group);
			return;
		}
	}

	// Insert modifiers at the end of the selected pipelines.
	for(int index = modifiers.size() - 1; index >= 0; --index) {
		ModifierApplication* modApp = selectedPipeline()->applyModifier(modifiers[index]);
		if(group)
			modApp->setModifierGroup(group);
		else
			setNextObjectToSelect(modApp);
	}
	if(group)
		setNextObjectToSelect(group);
}

/******************************************************************************
* Deletes the modifier or modifier group at the given list index of the model.
******************************************************************************/
void PipelineListModel::deleteItem(int index)
{
	// Get the modifier list item.
	PipelineListItem* selectedItem = item(index);
	if(!selectedItem) return;

	if(OORef<ModifierApplication> modApp = dynamic_object_cast<ModifierApplication>(selectedItem->object())) {
		deleteModifierApplication(modApp);
	}
	else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(selectedItem->object())) {
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Delete modifier group"), [&]() {
			for(ModifierApplication* modApp : group->modifierApplications())
				deleteModifierApplication(modApp);
		});
	}
}

/******************************************************************************
* Deletes a modifier application from the pipeline.
******************************************************************************/
void PipelineListModel::deleteModifierApplication(ModifierApplication* modApp)
{
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
		modApp->setModifierGroup(nullptr);

		// Delete modifier if there are no more applications left.
		if(modifier->modifierApplications().empty())
			modifier->deleteReferenceObject();
	});

	// Invalidate the items list of the model.
	refreshListLater();
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

	// While the items of the model are out of date, do not return any data and wait until the items list is rebuilt.
	if(_listRefreshPending)
		return {};

	PipelineListItem* item = this->item(index.row());

	// While the item is being updated, do not access any model data, because it may be in an inconsistent state.
	if(item->isUpdatePending())
		return {};

	if(role == Qt::DisplayRole || role == TitleRole) {
		// Indent modifiers that are part of a group.
		if(item->itemType() == PipelineListItem::Modifier) {
			if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
				if(modApp->modifierGroup())
					return QStringLiteral(" ") + item->title();
			}
		}
		return item->title();
	}
	else if(role == Qt::EditRole) {
		return item->title();
	}
	else if(role == ItemTypeRole) {
		return item->itemType();
	}
	else if(role == IsCollapsedRole) {
		if(item->itemType() == PipelineListItem::ModifierGroup)
			return static_object_cast<ModifierGroup>(item->object())->isCollapsed();
	}
	else if(role == Qt::DecorationRole) {
		if(item->itemType() == PipelineListItem::ModifierGroup) {
			if(!static_object_cast<ModifierGroup>(item->object())->isCollapsed())
				return _modifierGroupExpanded;
		}
		if(item->isObjectActive()) {
			const_cast<QMovie&>(_statusPendingIcon).start();
			return QVariant::fromValue(_statusPendingIcon.currentPixmap());
		}
		if(item->itemType() == PipelineListItem::ModifierGroup) {
			if(item->status().type() == PipelineStatus::Success)
				return _modifierGroupCollapsed;
		}
		if(item->isObjectItem()) {
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
		if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object()))
			return (modApp->modifier() && modApp->modifier()->isEnabled()) ? Qt::Checked : Qt::Unchecked;
		else if(ActiveObject* object = dynamic_object_cast<ActiveObject>(item->object())) {
			if(item->itemType() != PipelineListItem::DataSource)
				return object->isEnabled() ? Qt::Checked : Qt::Unchecked;
		}
		if(role == CheckedRole)
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
		if(!item->isObjectItem())
			return _sectionHeaderForegroundBrush;
		else if(item->itemType() == PipelineListItem::Modifier && static_object_cast<ModifierApplication>(item->object())->modifierAndGroupEnabled() == false)
			return _disabledForegroundBrush;
		else if(item->itemType() == PipelineListItem::ModifierGroup && static_object_cast<ModifierGroup>(item->object())->isEnabled() == false)
			return _disabledForegroundBrush;
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
			return true;
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value.toInt() != Qt::Unchecked) ? tr("Enable modifier") : tr("Disable modifier"), [modApp, &value, index, role, this]() {
				if(modApp->modifier()) 
					modApp->modifier()->setEnabled(value.toInt() != Qt::Unchecked);
			});
			return true;
		}
		else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(item->object())) {
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(),
					(value.toBool()) ? tr("Enable modifier group") : tr("Disable modifier group"), [group, &value]() {
				group->setEnabled(value.toBool());
			});
			return true;
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
			return true;
		}
		else if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
			QString newName = value.toString();
			if(modApp->modifier() && modApp->modifier()->objectTitle() != newName) {
				UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Rename modifier"), [modApp, &newName]() {
					modApp->modifier()->setObjectTitle(newName);
				});
			}
			return true;
		}
		else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(item->object())) {
			QString newName = value.toString();
			if(group->objectTitle() != newName) {
				UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Rename modifier group"), [group, &newName]() {
					group->setObjectTitle(newName);
				});
			}
			return true;
		}
	}
	else if(role == IsCollapsedRole) {
		if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(this->item(index.row())->object())) {
			group->setCollapsed(value.toBool());
			return true;
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
			case PipelineListItem::ModifierGroup:
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
* Updates the state of the actions that can be invoked on the currently 
* selected list item.
******************************************************************************/
void PipelineListModel::updateActions()
{
	PipelineListItem* currentItem = selectedItem();
	RefTarget* currentObject = currentItem ? currentItem->object() : nullptr;

	// While the items of the model are out of date, do not enable any actions and wait until the items list is rebuilt.
	if(_listRefreshPending)
		currentObject = nullptr;

	if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(currentObject)) {
		_deleteItemAction->setEnabled(true);
		int index = std::distance(items().begin(), std::find(items().begin(), items().end(), currentItem));

		_moveItemDownAction->setEnabled(
			modApp->input()
			&& (dynamic_object_cast<ModifierApplication>(modApp->input()) != nullptr || modApp->modifierGroup() != nullptr)
			&& (modApp->input()->isPipelineBranch(true) == false || modApp->modifierGroup() != nullptr)
			&& modApp->pipelines(true).empty() == false
			&& (modApp->modifierGroup() == nullptr || modApp->modifierGroup()->modifierApplications().size() > 1));

		_moveItemUpAction->setEnabled(
			index > 0 
			&& (item(index - 1)->itemType() == PipelineListItem::Modifier || item(index - 1)->itemType() == PipelineListItem::ModifierGroup) 
			&& (modApp->isPipelineBranch(true) == false || modApp->modifierGroup() != nullptr)
			&& modApp->pipelines(true).empty() == false
			&& (modApp->modifierGroup() == nullptr || modApp->modifierGroup()->modifierApplications().size() > 1));

		_toggleModifierGroupAction->setEnabled(true);
		_toggleModifierGroupAction->setChecked(modApp->modifierGroup() != nullptr);
	}
	else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(currentObject)) {
		_deleteItemAction->setEnabled(true);
		_moveItemUpAction->setEnabled(false);
		_moveItemDownAction->setEnabled(false);
		_toggleModifierGroupAction->setEnabled(true);
		_toggleModifierGroupAction->setChecked(true);

		// Determine whether it would be possible to move the entire modifier group up and/or down.
		if(group->pipelines(true).empty() == false) {
			QVector<ModifierApplication*> groupModApps = group->modifierApplications();
			if(ModifierApplication* inputModApp = dynamic_object_cast<ModifierApplication>(groupModApps.back()->input())) {
				OVITO_ASSERT(inputModApp->modifierGroup() != group);
				_moveItemDownAction->setEnabled(!inputModApp->isPipelineBranch(true));
			}
			_moveItemUpAction->setEnabled(groupModApps.front()->getPredecessorModApp() != nullptr);
		}
	}
	else {
		_deleteItemAction->setEnabled(false);
		_moveItemUpAction->setEnabled(false);
		_moveItemDownAction->setEnabled(false);
		_toggleModifierGroupAction->setEnabled(false);
		_toggleModifierGroupAction->setChecked(false);
	}

	_makeElementIndependentAction->setEnabled(isSharedObject(currentObject));
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
	else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(obj)) {
		if(!group->pipelines(true).empty()) {
			auto modApps = group->modifierApplications();
			return isSharedObject(modApps.front());
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

/******************************************************************************
* Moves the selected modifier up one position in the stack.
******************************************************************************/
void PipelineListModel::moveModifierUp()
{
	PipelineListItem* item = selectedItem();
	if(!item) return;

	if(OORef<ModifierApplication> modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Move modifier up"), [modApp]() {
			if(OORef<ModifierApplication> predecessor = modApp->getPredecessorModApp()) {
				OVITO_ASSERT(!predecessor->pipelines(true).empty());
				if(modApp->modifierGroup() != nullptr && predecessor->modifierGroup() != modApp->modifierGroup()) {
					// If the modifier application is the first entry in a modifier group, move it out of the group.
					modApp->setModifierGroup(nullptr);
				}
				else if(modApp->modifierGroup() == nullptr && predecessor->modifierGroup() != nullptr && predecessor->modifierGroup()->isCollapsed() == false) {
					// If the modifier application is preceded by a modifier group that is currently expanded, move the modifier application into the group.
					modApp->setModifierGroup(predecessor->modifierGroup());
				}
				else if(modApp->modifierGroup() == nullptr && predecessor->modifierGroup() != nullptr && predecessor->modifierGroup()->isCollapsed() == true) {
					// If the modifier application is preceded by a modifier group that is currently collapsed, move the modifier application above the entire group.
					OORef<ModifierApplication> current = predecessor;
					for(;;) {
						ModifierApplication* next = nullptr;
						current->visitDependents([&](RefMaker* dependent2) {
							if(ModifierApplication* predecessor2 = dynamic_object_cast<ModifierApplication>(dependent2)) {
								if(predecessor2->modifierGroup() != predecessor->modifierGroup())
									predecessor2->setInput(modApp);
								else 
									next = predecessor2;
							}
							else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent2)) {
								pipeline->setDataProvider(modApp);
							}
						});
						if(!next) break;
						current = next;
					}
					predecessor->setInput(modApp->input());
					modApp->setInput(current);
				}
				else {
					// Standard case: If the modifier application is preceeded by another modifier application, swap the two.
					predecessor->visitDependents([&](RefMaker* dependent2) {
						if(ModifierApplication* predecessor2 = dynamic_object_cast<ModifierApplication>(dependent2)) {
							OVITO_ASSERT(predecessor2->input() == predecessor);
							predecessor2->setInput(modApp);
						}
						else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent2)) {
							OVITO_ASSERT(pipeline->dataProvider() == predecessor);
							pipeline->setDataProvider(modApp);
						}
					});
					predecessor->setInput(modApp->input());
					modApp->setInput(predecessor);
				}
			}
			else if(modApp->modifierGroup() != nullptr) {
				modApp->setModifierGroup(nullptr);
			}
		});
	}
	else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(item->object())) {
		// Determine the modapps that form the head and the tail for the group.
		QVector<ModifierApplication*> groupModApps = group->modifierApplications();
		OORef<ModifierApplication> headModApp = groupModApps.front();
		OORef<ModifierApplication> tailModApp = groupModApps.back();
		ModifierApplication* predecessor = headModApp->getPredecessorModApp();
		OVITO_ASSERT(tailModApp->isReferencedBy(headModApp));
		OVITO_ASSERT(!predecessor || !headModApp->isPipelineBranch(true));

		// Don't move the group it is preceded by a pipeline branch or no modifier application at all.
		if(!predecessor)
			return;

		// Determine where to reinsert the group of modifiers into the pipeline.
		OORef<ModifierApplication> insertBefore = predecessor;
		if(predecessor->modifierGroup() != nullptr) {
			for(;;) {
				ModifierApplication* prev = nullptr;
				insertBefore->visitDependents([&](RefMaker* dependent) {
					if(ModifierApplication* predecessor2 = dynamic_object_cast<ModifierApplication>(dependent)) {
						OVITO_ASSERT(!predecessor2->isPipelineBranch(true));
						if(predecessor2->modifierGroup() == predecessor->modifierGroup()) {
							insertBefore = predecessor2;
							prev = predecessor2;
						}
					}
				});
				if(!prev) break;
			}
		}

		// Make the pipeline rearrangement.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Move modifier group up"), [&]() {
			insertBefore->visitDependents([&](RefMaker* dependent) {
				if(ModifierApplication* predecessor = dynamic_object_cast<ModifierApplication>(dependent)) {
					OVITO_ASSERT(predecessor->input() == insertBefore);
					predecessor->setInput(headModApp);
				}
				else if(PipelineSceneNode* predecessor = dynamic_object_cast<PipelineSceneNode>(dependent)) {
					OVITO_ASSERT(predecessor->dataProvider() == insertBefore);
					predecessor->setDataProvider(headModApp);
				}
			});
			predecessor->setInput(tailModApp->input());
			tailModApp->setInput(insertBefore);
		});
	}
}

/******************************************************************************
* Moves the selected modifier down one position in the stack.
******************************************************************************/
void PipelineListModel::moveModifierDown()
{
	PipelineListItem* item = selectedItem();
	if(!item) return;

	if(OORef<ModifierApplication> modApp = dynamic_object_cast<ModifierApplication>(item->object())) {
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Move modifier down"), [modApp]() {
			OORef<ModifierApplication> successor = dynamic_object_cast<ModifierApplication>(modApp->input());
			if(successor && successor->isPipelineBranch(true) == false) {
				if(modApp->modifierGroup() != nullptr && successor->modifierGroup() != modApp->modifierGroup()) {
					// If the modifier application is the last entry in the modifier group, move it out of the group. 
					modApp->setModifierGroup(nullptr);
				}
				else if(modApp->modifierGroup() == nullptr && successor->modifierGroup() != nullptr && successor->modifierGroup()->isCollapsed() == false) {
					// If the modifier application is above a group that is currently expanded, move it into the group.
					modApp->setModifierGroup(successor->modifierGroup());
				}
				else {
					// Standard case: If the modifier application is followed by another modifier application, swap the two.
					OORef<ModifierApplication> insertAfter = successor;

					// If the modifier application is above a group that is currently collapsed, move it all the way below that group.
					if(modApp->modifierGroup() == nullptr && successor->modifierGroup() != nullptr && successor->modifierGroup()->isCollapsed() == true) {
						while(ModifierApplication* next = dynamic_object_cast<ModifierApplication>(insertAfter->input())) {
							if(next->modifierGroup() != successor->modifierGroup())
								break;
							insertAfter = next;
						}
					}

					// Make the pipeline rearrangement.
					modApp->visitDependents([&](RefMaker* dependent) {
						if(ModifierApplication* predecessor = dynamic_object_cast<ModifierApplication>(dependent)) {
							predecessor->setInput(successor);
						}
						else if(PipelineSceneNode* predecessor = dynamic_object_cast<PipelineSceneNode>(dependent)) {
							predecessor->setDataProvider(successor);
						}
					});
					modApp->setInput(insertAfter->input());
					insertAfter->setInput(modApp);
				}
			}
			else if(modApp->modifierGroup() != nullptr) {
				modApp->setModifierGroup(nullptr);
			}
		});
	}
	else if(ModifierGroup* group = dynamic_object_cast<ModifierGroup>(item->object())) {
		QVector<ModifierApplication*> groupModApps = group->modifierApplications();
		OORef<ModifierApplication> headModApp = groupModApps.front();
		OORef<ModifierApplication> tailModApp = groupModApps.back();
		ModifierApplication* successor = dynamic_object_cast<ModifierApplication>(tailModApp->input());

		// Don't move the group over a pipeline branch.
		if(!successor || successor->isPipelineBranch(true))
			return;

		// Determine where to reinsert the group of modifiers into the pipeline.
		OORef<ModifierApplication> insertAfter = successor;
		if(successor->modifierGroup() != nullptr) {
			while(ModifierApplication* next = dynamic_object_cast<ModifierApplication>(insertAfter->input())) {
				if(next->modifierGroup() != successor->modifierGroup())
					break;
				insertAfter = next;
			}
		}

		// Make the pipeline rearrangement.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Move modifier group down"), [&]() {
			headModApp->visitDependents([&](RefMaker* dependent) {
				if(ModifierApplication* predecessor = dynamic_object_cast<ModifierApplication>(dependent)) {
					predecessor->setInput(successor);
				}
				else if(PipelineSceneNode* predecessor = dynamic_object_cast<PipelineSceneNode>(dependent)) {
					predecessor->setDataProvider(successor);
				}
			});
			tailModApp->setInput(insertAfter->input());
			insertAfter->setInput(headModApp);
		});
	}
}

/******************************************************************************
* Replaces the selected pipeline item with an independent copy.
******************************************************************************/
void PipelineListModel::makeElementIndependent()
{
	// Get the currently selected pipeline item.
	PipelineListItem* item = selectedItem();
	if(!item) return;

	if(DataVis* visElement = dynamic_object_cast<DataVis>(item->object())) {
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Make visual element independent"), [&]() {
			PipelineSceneNode* pipeline = selectedPipeline();
			DataVis* replacementVisElement = pipeline->makeVisElementIndependent(visElement);
			setNextObjectToSelect(replacementVisElement);
		});
	}
	else if(PipelineObject* selectedPipelineObj = dynamic_object_cast<PipelineObject>(item->object())) {
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Make pipeline element independent"), [&]() {
			CloneHelper cloneHelper;
			PipelineObject* clonedObject = makeElementIndependentImpl(selectedPipelineObj, cloneHelper);
			if(clonedObject)
				setNextObjectToSelect(clonedObject);
		});
	}
	else if(ModifierGroup* selectedGroup = dynamic_object_cast<ModifierGroup>(item->object())) {
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Make modifier group independent"), [&]() {
			CloneHelper cloneHelper;
			for(ModifierApplication* modApp : selectedGroup->modifierApplications()) {
				ModifierApplication* clonedModApp = static_object_cast<ModifierApplication>(makeElementIndependentImpl(modApp, cloneHelper));
				OVITO_ASSERT(clonedModApp);
				if(clonedModApp && clonedModApp->modifierGroup()) {
					setNextObjectToSelect(clonedModApp->modifierGroup());
				}
			}
		});
	}
}

/******************************************************************************
* Replaces the a pipeline item with an independent copy.
******************************************************************************/
PipelineObject* PipelineListModel::makeElementIndependentImpl(PipelineObject* pipelineObj, CloneHelper& cloneHelper)
{
	OORef<PipelineObject> currentObj = selectedPipeline()->dataProvider();
	ModifierApplication* predecessorModApp = nullptr;
	// Walk up the pipeline, starting at the node, until we reach the selected pipeline object.
	// Duplicate all shared ModifierApplications to remove pipeline branches.
	// When arriving at the selected modifier application, duplicate the modifier too
	// in case it is being shared by multiple pipelines.
	while(currentObj) {
		PipelineObject* nextObj = nullptr;
		if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(currentObj)) {

			// Clone all modifier applications along the way if they are shared by multiple pipeline branches.
			if(modApp->pipelines(true).size() > 1) {
				OORef<ModifierApplication> clonedModApp = cloneHelper.cloneObject(modApp, false);
				if(predecessorModApp)
					predecessorModApp->setInput(clonedModApp);
				else
					selectedPipeline()->setDataProvider(clonedModApp);
				predecessorModApp = clonedModApp;
			}
			else {
				predecessorModApp = modApp;
			}

			// Terminate pipeline walk at the target object to be made independent.
			if(currentObj == pipelineObj) {
				// Clone the selected modifier if it is referenced by multiple modapps.
				if(predecessorModApp->modifier()) {
					QSet<PipelineSceneNode*> pipelines;
					for(ModifierApplication* modApp : predecessorModApp->modifier()->modifierApplications())
						pipelines.unite(modApp->pipelines(true));
					if(pipelines.size() > 1)
						predecessorModApp->setModifier(cloneHelper.cloneObject(predecessorModApp->modifier(), true));
				}
				return predecessorModApp;
			}
			currentObj = predecessorModApp->input();
		}
		else if(currentObj == pipelineObj) {
			// If the object to be made independent is not a modifier application, simply clone it.
			if(currentObj->pipelines(true).size() > 1) {
				OORef<PipelineObject> clonedObject = cloneHelper.cloneObject(currentObj, false);
				if(predecessorModApp)
					predecessorModApp->setInput(clonedObject);
				else
					selectedPipeline()->setDataProvider(clonedObject);
				return clonedObject;
			}
			return currentObj;
		}
		else {
			OVITO_ASSERT(false);
			break;
		}
	}
	return nullptr;
}

/******************************************************************************
* Creates or dissolves a group of modifiers.
******************************************************************************/
void PipelineListModel::toggleModifierGroup()
{
	PipelineListItem* item = selectedItem();
	if(!item) return;
	RefTarget* object = item->object();
	
	if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(object)) {
		// If a modifier application is currently selected, put it into a new group.
		// But first make sure the modifier application isn't already part of an existing group.
		if(!modApp->modifierGroup()) {
			// Create a new group.
			OORef<ModifierGroup> group = OORef<ModifierGroup>::create(modApp->dataset(), Application::instance()->executionContext());
			UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Create modifier group"), [&]() {
				modApp->setModifierGroup(std::move(group));
			});
		}
		else {
			// Dissolve the modifier's group below.
			object = modApp->modifierGroup();
		}
	}
	
	if(OORef<ModifierGroup> group = dynamic_object_cast<ModifierGroup>(object)) {
		// If an existing modifier group is currently selected, dissolve the group.
		UndoableTransaction::handleExceptions(_datasetContainer.currentSet()->undoStack(), tr("Dissolve modifier group"), [&]() {
			QVector<ModifierApplication*> groupModApps = group->modifierApplications();
			if(selectedItem()->object() == group)
				setNextObjectToSelect(groupModApps.front());
			for(ModifierApplication* modApp : groupModApps)
				modApp->setModifierGroup(nullptr);
			group->deleteReferenceObject();
		});
	}
}

}	// End of namespace
