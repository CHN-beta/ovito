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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/PropertiesPanel.h>
#include <ovito/gui/desktop/widgets/general/RolloutContainer.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/oo/RefTargetListener.h>
#include "ModifierListModel.h"

namespace Ovito {

class PipelineListModel;	// defined in PipelineListModel.h
class PipelineListItem;		// defined in PipelineListItem.h

/**
 * The command panel tab lets the user modify the selected object.
 */
class OVITO_GUI_EXPORT ModifyCommandPage : public QWidget
{
	Q_OBJECT

public:

	/// Initializes the modify page.
    ModifyCommandPage(MainWindow* mainWindow, QWidget* parent);

	/// Returns the object that is currently being edited in the properties panel.
	RefTarget* editObject() const { return _propertiesPanel->editObject(); }

	/// Returns the list model that encapsulates the modification pipeline of the selected node(s).
	PipelineListModel* pipelineListModel() const { return _pipelineListModel; }

	/// Returns the list model that lists the available modifiers.
	ModifierListModel* modifierListModel() const { return static_cast<ModifierListModel*>(_modifierSelector->model()); }

protected Q_SLOTS:

	/// This is called after all changes to the selection set have been completed.
	void onSelectionChangeComplete(SelectionSet* newSelection);

	/// Is called when a new modification list item has been selected, or if the currently
	/// selected item has changed.
	void onSelectedItemChanged();

	/// Handles the ACTION_MODIFIER_DELETE command, which deleted the selected modifier from the stack.
	void onDeleteModifier();

	/// This called when the user double clicks on an item in the modifier stack.
	void onModifierStackDoubleClicked(const QModelIndex& index);

	/// Handles the ACTION_MODIFIER_MOVE_UP command, which moves the selected modifier up one entry in the stack.
	void onModifierMoveUp();

	/// Handles the ACTION_MODIFIER_MOVE_DOWN command, which moves the selected modifier down one entry in the stack.
	void onModifierMoveDown();

	/// Handles the ACTION_MODIFIER_TOGGLE_STATE command, which toggles the enabled/disable state of the selected modifier.
	void onModifierToggleState(bool newState);

	/// Handles the ACTION_PIPELINE_MAKE_INDEPENDENT command, which replaces the selected pipeline item with an independent copy.
	void onMakeElementIndependent();

	/// Is called by the system when fetching the news web page from the server is completed.
	void onWebRequestFinished();

private:

	/// Updates the state of the actions that can be invoked on the currently selected item.
	void updateActions(PipelineListItem* currentItem);

	/// Creates the rollout panel that shows information about the application whenever no object is selected.
	void createAboutPanel();

	/// Displays the given HTML page content in the About pane.
	void showProgramNotice(const QString& htmlPage);

private:

	/// The container of the current dataset being edited.
	DataSetContainer& _datasetContainer;

	/// The action manager of the main window.
	ActionManager* _actionManager;

	/// This list box shows the modifier stack of the selected scene node(s).
	QListView* _pipelineWidget;

	/// The Qt model for the data pipeline of the selected node(s).
	PipelineListModel* _pipelineListModel;

	/// This widget displays the list of available modifiers and allows the user to insert a modifier into the pipeline.
	QComboBox* _modifierSelector;

	/// This panel shows the properties of the selected modifier stack entry
	PropertiesPanel* _propertiesPanel;

	/// The panel displaying information about the application when no object is selected.
	Rollout* _aboutRollout;
};

}	// End of namespace
