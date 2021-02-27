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
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/gui/base/viewport/NavigationModes.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/gui/base/mainwin/MainWindowInterface.h>
#include <ovito/gui/base/actions/ViewportModeAction.h>
#include "ActionManager.h"

namespace Ovito {

/******************************************************************************
* Initializes the ActionManager.
******************************************************************************/
ActionManager::ActionManager(QObject* parent, MainWindowInterface* mainWindow) : QAbstractListModel(parent), _mainWindow(mainWindow)
{
	// Actions need to be updated whenever a new dataset is loaded or the current selection changes.
	connect(&mainWindow->datasetContainer(), &DataSetContainer::dataSetChanged, this, &ActionManager::onDataSetChanged);
	connect(&mainWindow->datasetContainer(), &DataSetContainer::animationSettingsReplaced, this, &ActionManager::onAnimationSettingsReplaced);
	connect(&mainWindow->datasetContainer(), &DataSetContainer::selectionChangeComplete, this, &ActionManager::onSelectionChangeComplete);

	createCommandAction(ACTION_QUIT, tr("Quit"), ":/guibase/actions/file/file_quit.bw.svg", tr("Quit the application."));
	createCommandAction(ACTION_FILE_OPEN, tr("Load Session State"), ":/guibase/actions/file/file_open.bw.svg", tr("Load a previously saved session from a file."), QKeySequence::Open);
	createCommandAction(ACTION_FILE_SAVE, tr("Save Session State"), ":/guibase/actions/file/file_save.bw.svg", tr("Save the current program session to a file."), QKeySequence::Save);
	createCommandAction(ACTION_FILE_SAVEAS, tr("Save Session State As"), ":/guibase/actions/file/file_save_as.bw.svg", tr("Save the current program session to a new file."), QKeySequence::SaveAs);
	createCommandAction(ACTION_FILE_IMPORT, tr("Load File"), ":/guibase/actions/file/file_import.bw.svg", tr("Import data from a file on this computer."), Qt::Key_I | Qt::CTRL);
	createCommandAction(ACTION_FILE_REMOTE_IMPORT, tr("Load Remote File"), ":/guibase/actions/file/file_import_remote.bw.svg", tr("Import a file from a remote location."), Qt::Key_I | Qt::CTRL | Qt::SHIFT);
	createCommandAction(ACTION_FILE_EXPORT, tr("Export File"), ":/guibase/actions/file/file_export.bw.svg", tr("Export data to a file."), Qt::Key_E | Qt::CTRL);
	createCommandAction(ACTION_FILE_NEW_WINDOW, tr("New Program Window"), ":/guibase/actions/file/new_window.bw.svg", tr("Open another OVITO program window."), QKeySequence::New);
	createCommandAction(ACTION_HELP_ABOUT, tr("About OVITO"), ":/guibase/actions/file/about.bw.svg", tr("Show information about the software."));
	createCommandAction(ACTION_HELP_SHOW_ONLINE_HELP, tr("User Manual"), ":/guibase/actions/file/user_manual.bw.svg", tr("Open the user manual."), QKeySequence::HelpContents);
	createCommandAction(ACTION_HELP_SHOW_SCRIPTING_HELP, tr("Scripting Reference"), ":/guibase/actions/file/scripting_manual.bw.svg", tr("Open the Python API documentation."));
	createCommandAction(ACTION_HELP_OPENGL_INFO, tr("OpenGL Information"), ":/guibase/actions/file/opengl_info.bw.svg", tr("Display OpenGL graphics driver information."));

	createCommandAction(ACTION_EDIT_UNDO, tr("Undo"), ":/guibase/actions/edit/edit_undo.bw.svg", tr("Reverse the last action."), QKeySequence::Undo);
	createCommandAction(ACTION_EDIT_REDO, tr("Redo"), ":/guibase/actions/edit/edit_redo.bw.svg", tr("Restore the previously reversed action."), QKeySequence::Redo);
	createCommandAction(ACTION_EDIT_CLEAR_UNDO_STACK, tr("Clear Undo Stack"), nullptr, tr("Discards all existing undo records."))->setVisible(false);

	createCommandAction(ACTION_NEW_PIPELINE_FILESOURCE, tr("External file"), ":/guibase/actions/edit/create_pipeline.svg", tr("Creates a new pipeline with an external file as data source."));
	createCommandAction(ACTION_EDIT_CLONE_PIPELINE, tr("Clone Pipeline..."), ":/guibase/actions/edit/clone_pipeline.bw.svg", tr("Duplicate the current pipeline to show multiple datasets side by side."));
	createCommandAction(ACTION_EDIT_RENAME_PIPELINE, tr("Rename Pipeline..."), ":/guibase/actions/edit/rename_pipeline.bw.svg", tr("Assign a new name to the selected pipeline."));
	createCommandAction(ACTION_EDIT_DELETE, tr("Delete Pipeline"), ":/guibase/actions/edit/edit_delete.bw.svg", tr("Delete the selected object from the scene."));

	createCommandAction(ACTION_SETTINGS_DIALOG, tr("Application Settings..."), ":/guibase/actions/file/preferences.bw.svg", tr("Open the application settings dialog"), QKeySequence::Preferences);

	createCommandAction(ACTION_RENDER_ACTIVE_VIEWPORT, tr("Render Active Viewport"), ":/guibase/actions/rendering/render_active_viewport.bw.svg", tr("Render an image or animation of the active viewport."));

	createCommandAction(ACTION_VIEWPORT_MAXIMIZE, tr("Maximize Active Viewport"), ":/guibase/actions/viewport/maximize_viewport.bw.svg", tr("Enlarge/reduce the active viewport."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS, tr("Zoom Scene Extents"), ":/guibase/actions/viewport/zoom_scene_extents.bw.svg",
#ifndef Q_OS_MACX
		tr("Zoom active viewport to show everything. Use CONTROL key to zoom all viewports at once."));
#else
		tr("Zoom active viewport to show everything. Use COMMAND key to zoom all viewports at once."));
#endif
	createCommandAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS_ALL, tr("Zoom Scene Extents All"), ":/guibase/actions/viewport/zoom_scene_extents_all.png", tr("Zoom all viewports to show everything."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS, tr("Zoom Selection Extents"), ":/guibase/actions/viewport/zoom_selection_extents.png", tr("Zoom active viewport to show the selected objects."));
	createCommandAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS_ALL, tr("Zoom Selection Extents All"), ":/guibase/actions/viewport/zoom_selection_extents.png", tr("Zoom all viewports to show the selected objects."));

	ViewportInputManager* vpInputManager = mainWindow->viewportInputManager();
	createViewportModeAction(ACTION_VIEWPORT_ZOOM, vpInputManager->zoomMode(), tr("Zoom"), ":/guibase/actions/viewport/mode_zoom.bw.svg", tr("Activate zoom mode."));
	createViewportModeAction(ACTION_VIEWPORT_PAN, vpInputManager->panMode(), tr("Pan"), ":/guibase/actions/viewport/mode_pan.bw.svg", tr("Activate pan mode to shift the region visible in the viewports."));
	createViewportModeAction(ACTION_VIEWPORT_ORBIT, vpInputManager->orbitMode(), tr("Orbit Camera"), ":/guibase/actions/viewport/mode_orbit.bw.svg", tr("Activate orbit mode to rotate the camera around the scene."));
	createViewportModeAction(ACTION_VIEWPORT_FOV, vpInputManager->fovMode(), tr("Change Field Of View"), ":/guibase/actions/viewport/mode_fov.bw.svg", tr("Activate field of view mode to change the perspective projection."));
	createViewportModeAction(ACTION_VIEWPORT_PICK_ORBIT_CENTER, vpInputManager->pickOrbitCenterMode(), tr("Set Orbit Center"), ":/guibase/actions/viewport/mode_set_orbit_center.png", tr("Set the center of rotation of the viewport camera."))->setVisible(false);

	createViewportModeAction(ACTION_SELECTION_MODE, vpInputManager->selectionMode(), tr("Select"), ":/guibase/actions/edit/mode_select.bw.svg", tr("Select objects in the viewports."));

	createCommandAction(ACTION_GOTO_START_OF_ANIMATION, tr("Go to Start of Animation"), ":/guibase/actions/animation/goto_animation_start.bw.svg", tr("Jump to first frame of the animation."), Qt::Key_Home);
	createCommandAction(ACTION_GOTO_END_OF_ANIMATION, tr("Go to End of Animation"), ":/guibase/actions/animation/goto_animation_end.bw.svg", tr("Jump to the last frame of the animation."), Qt::Key_End);
	createCommandAction(ACTION_GOTO_PREVIOUS_FRAME, tr("Go to Previous Frame"), ":/guibase/actions/animation/goto_previous_frame.bw.svg", tr("Move time slider one animation frame backward."), Qt::Key_Left | Qt::ALT);
	createCommandAction(ACTION_GOTO_NEXT_FRAME, tr("Go to Next Frame"), ":/guibase/actions/animation/goto_next_frame.bw.svg", tr("Move time slider one animation frame forward."), Qt::Key_Right | Qt::ALT);
	createCommandAction(ACTION_START_ANIMATION_PLAYBACK, tr("Start Animation Playback"), ":/guibase/actions/animation/play_animation.bw.svg", tr("Start playing the animation in the viewports."));
	createCommandAction(ACTION_STOP_ANIMATION_PLAYBACK, tr("Stop Animation Playback"), ":/guibase/actions/animation/stop_animation.bw.svg", tr("Stop playing the animation in the viewports."));
	createCommandAction(ACTION_ANIMATION_SETTINGS, tr("Animation Settings"), ":/guibase/actions/animation/animation_settings.bw.svg", tr("Open the animation settings dialog."));
	createCommandAction(ACTION_TOGGLE_ANIMATION_PLAYBACK, tr("Play Animation"), ":/guibase/actions/animation/play_animation.bw.svg", tr("Start/stop animation playback. Hold down Shift key to play backwards."), Qt::Key_Space)->setCheckable(true);
	createCommandAction(ACTION_AUTO_KEY_MODE_TOGGLE, tr("Auto Key Mode"), ":/guibase/actions/animation/animation_mode.bw.svg", tr("Toggle auto-key mode for creating animation keys."))->setCheckable(true);

	connect(getAction(ACTION_VIEWPORT_MAXIMIZE), &QAction::triggered, this, &ActionManager::on_ViewportMaximize_triggered);
	connect(getAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS), &QAction::triggered, this, &ActionManager::on_ViewportZoomSceneExtents_triggered);
	connect(getAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS), &QAction::triggered, this, &ActionManager::on_ViewportZoomSelectionExtents_triggered);
	connect(getAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS_ALL), &QAction::triggered, this, &ActionManager::on_ViewportZoomSceneExtentsAll_triggered);
	connect(getAction(ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS_ALL), &QAction::triggered, this, &ActionManager::on_ViewportZoomSelectionExtentsAll_triggered);
	connect(getAction(ACTION_GOTO_START_OF_ANIMATION), &QAction::triggered, this, &ActionManager::on_AnimationGotoStart_triggered);
	connect(getAction(ACTION_GOTO_END_OF_ANIMATION), &QAction::triggered, this, &ActionManager::on_AnimationGotoEnd_triggered);
	connect(getAction(ACTION_GOTO_PREVIOUS_FRAME), &QAction::triggered, this, &ActionManager::on_AnimationGotoPreviousFrame_triggered);
	connect(getAction(ACTION_GOTO_NEXT_FRAME), &QAction::triggered, this, &ActionManager::on_AnimationGotoNextFrame_triggered);
	connect(getAction(ACTION_START_ANIMATION_PLAYBACK), &QAction::triggered, this, &ActionManager::on_AnimationStartPlayback_triggered);
	connect(getAction(ACTION_STOP_ANIMATION_PLAYBACK), &QAction::triggered, this, &ActionManager::on_AnimationStopPlayback_triggered);
	connect(getAction(ACTION_EDIT_DELETE), &QAction::triggered, this, &ActionManager::on_EditDelete_triggered);
}

/******************************************************************************
* Returns dataset currently being edited in the main window.
******************************************************************************/
DataSet* ActionManager::dataset() const
{
	return mainWindow()->datasetContainer().currentSet();
}

void ActionManager::onDataSetChanged(DataSet* newDataSet)
{
	disconnect(_canUndoChangedConnection);
	disconnect(_canRedoChangedConnection);
	disconnect(_undoTextChangedConnection);
	disconnect(_redoTextChangedConnection);
	disconnect(_undoTriggeredConnection);
	disconnect(_redoTriggeredConnection);
	disconnect(_clearUndoStackTriggeredConnection);
	QAction* undoAction = getAction(ACTION_EDIT_UNDO);
	QAction* redoAction = getAction(ACTION_EDIT_REDO);
	QAction* clearUndoStackAction = getAction(ACTION_EDIT_CLEAR_UNDO_STACK);
	if(newDataSet) {
		undoAction->setEnabled(newDataSet->undoStack().canUndo());
		redoAction->setEnabled(newDataSet->undoStack().canRedo());
		clearUndoStackAction->setEnabled(true);
		undoAction->setText(tr("Undo %1").arg(newDataSet->undoStack().undoText()));
		redoAction->setText(tr("Redo %1").arg(newDataSet->undoStack().redoText()));
		_canUndoChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::canUndoChanged, undoAction, &QAction::setEnabled);
		_canRedoChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::canRedoChanged, redoAction, &QAction::setEnabled);
		_undoTextChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::undoTextChanged, [this,undoAction](const QString& undoText) {
			undoAction->setText(tr("Undo %1").arg(undoText));
		});
		_redoTextChangedConnection = connect(&newDataSet->undoStack(), &UndoStack::redoTextChanged, [this,redoAction](const QString& redoText) {
			redoAction->setText(tr("Redo %1").arg(redoText));
		});
		_undoTriggeredConnection = connect(undoAction, &QAction::triggered, &newDataSet->undoStack(), &UndoStack::undo);
		_redoTriggeredConnection = connect(redoAction, &QAction::triggered, &newDataSet->undoStack(), &UndoStack::redo);
		_clearUndoStackTriggeredConnection = connect(clearUndoStackAction, &QAction::triggered, &newDataSet->undoStack(), &UndoStack::clear);
	}
	else {
		undoAction->setEnabled(false);
		redoAction->setEnabled(false);
		clearUndoStackAction->setEnabled(false);
	}
}

/******************************************************************************
* This is called when new animation settings have been loaded.
******************************************************************************/
void ActionManager::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_autoKeyModeChangedConnection);
	disconnect(_autoKeyModeToggledConnection);
	disconnect(_animationIntervalChangedConnection);
	disconnect(_animationPlaybackChangedConnection);
	disconnect(_animationPlaybackToggledConnection);
	if(newAnimationSettings) {
		QAction* autoKeyModeAction = getAction(ACTION_AUTO_KEY_MODE_TOGGLE);
		QAction* animationPlaybackAction = getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK);
		autoKeyModeAction->setChecked(newAnimationSettings->autoKeyMode());
		animationPlaybackAction->setChecked(newAnimationSettings->isPlaybackActive());
		_autoKeyModeChangedConnection = connect(newAnimationSettings, &AnimationSettings::autoKeyModeChanged, autoKeyModeAction, &QAction::setChecked);
		_autoKeyModeToggledConnection = connect(autoKeyModeAction, &QAction::toggled, newAnimationSettings, &AnimationSettings::setAutoKeyMode);
		_animationIntervalChangedConnection = connect(newAnimationSettings, &AnimationSettings::intervalChanged, this, &ActionManager::onAnimationIntervalChanged);
		_animationPlaybackChangedConnection = connect(newAnimationSettings, &AnimationSettings::playbackChanged, animationPlaybackAction, &QAction::setChecked);
		_animationPlaybackToggledConnection = connect(animationPlaybackAction, &QAction::toggled, newAnimationSettings, &AnimationSettings::setAnimationPlayback);
		onAnimationIntervalChanged(newAnimationSettings->animationInterval());
	}
	else {
		onAnimationIntervalChanged(TimeInterval(0));
	}
}

/******************************************************************************
* This is called when the active animation interval has changed.
******************************************************************************/
void ActionManager::onAnimationIntervalChanged(TimeInterval newAnimationInterval)
{
	bool isAnimationInterval = newAnimationInterval.duration() != 0;
	getAction(ACTION_GOTO_START_OF_ANIMATION)->setEnabled(isAnimationInterval);
	getAction(ACTION_GOTO_PREVIOUS_FRAME)->setEnabled(isAnimationInterval);
	getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK)->setEnabled(isAnimationInterval);
	getAction(ACTION_GOTO_NEXT_FRAME)->setEnabled(isAnimationInterval);
	getAction(ACTION_GOTO_END_OF_ANIMATION)->setEnabled(isAnimationInterval);
	getAction(ACTION_AUTO_KEY_MODE_TOGGLE)->setEnabled(isAnimationInterval);
	if(!isAnimationInterval && getAction(ACTION_AUTO_KEY_MODE_TOGGLE)->isChecked())
		getAction(ACTION_AUTO_KEY_MODE_TOGGLE)->setChecked(false);
}

/******************************************************************************
* This is called whenever the scene node selection changed.
******************************************************************************/
void ActionManager::onSelectionChangeComplete(SelectionSet* selection)
{
	getAction(ACTION_EDIT_DELETE)->setEnabled(selection && !selection->nodes().empty());
	getAction(ACTION_EDIT_CLONE_PIPELINE)->setEnabled(selection && !selection->nodes().empty());
	getAction(ACTION_EDIT_RENAME_PIPELINE)->setEnabled(selection && !selection->nodes().empty());
}

/******************************************************************************
* Invokes the command action with the given ID.
******************************************************************************/
void ActionManager::invokeAction(const QString& actionId)
{
	QAction* action = getAction(actionId);
	if(!action) throw Exception(tr("Action with id '%1' is not defined.").arg(actionId), dataset());
	action->trigger();
}

/******************************************************************************
* Registers an action with the ActionManager.
******************************************************************************/
void ActionManager::addAction(QAction* action)
{
	OVITO_CHECK_POINTER(action);
	OVITO_ASSERT_MSG(action->parent() == this || findAction(action->objectName()) == nullptr, "ActionManager::addAction()", "There is already an action with the same ID.");
	OVITO_ASSERT(!_actions.contains(action));

	// Make the action a child of this object.
	action->setParent(this);
	beginInsertRows(QModelIndex(), _actions.size(), _actions.size());
	_actions.push_back(action);
	endInsertRows();
}

/******************************************************************************
* Removes the given action from the ActionManager and deletes it.
******************************************************************************/
void ActionManager::deleteAction(QAction* action)
{
	OVITO_CHECK_POINTER(action);
	OVITO_ASSERT_MSG(action->parent() == this, "ActionManager::deleteAction()", "The action is not owned by the ActionManager.");
	OVITO_ASSERT_MSG(_actions.contains(action), "ActionManager::deleteAction()", "The action has not been registered with the ActionManager.");

	// Make the action a child of this object.
	int index = _actions.indexOf(action);
	beginRemoveRows(QModelIndex(), index, index);
	_actions.remove(index);
	delete action;
	endRemoveRows();
}

/******************************************************************************
* Creates and registers a new command action with the ActionManager.
******************************************************************************/
QAction* ActionManager::createCommandAction(const QString& id, const QString& title, const char* iconPath, const QString& statusTip, const QKeySequence& shortcut)
{
	QAction* action = new QAction(title, this);
	action->setObjectName(id);
	if(!shortcut.isEmpty())
		action->setShortcut(shortcut);
	if(!statusTip.isEmpty())
		action->setStatusTip(statusTip);
	if(!shortcut.isEmpty())
		action->setToolTip(QStringLiteral("%1 [%2]").arg(title).arg(shortcut.toString(QKeySequence::NativeText)));
	if(iconPath)
		action->setIcon(QIcon(QString(iconPath)));
	addAction(action);
	return action;
}

/******************************************************************************
* Creates and registers a new viewport mode action with the ActionManager.
******************************************************************************/
QAction* ActionManager::createViewportModeAction(const QString& id, ViewportInputMode* inputHandler, const QString& title, const char* iconPath, const QString& statusTip, const QKeySequence& shortcut)
{
	QAction* action = new ViewportModeAction(mainWindow(), title, this, inputHandler);
	action->setObjectName(id);
	if(!shortcut.isEmpty())
		action->setShortcut(shortcut);
	action->setStatusTip(statusTip);
	if(!shortcut.isEmpty())
		action->setToolTip(QStringLiteral("%1 [%2]").arg(title).arg(shortcut.toString(QKeySequence::NativeText)));
	if(iconPath)
		action->setIcon(QIcon(QString(iconPath)));
	addAction(action);
	return action;
}

/******************************************************************************
* Returns the data stored in this list model under the given role.
******************************************************************************/
QVariant ActionManager::data(const QModelIndex& index, int role) const
{
	QAction* action = _actions[index.row()];
	if(role == Qt::DisplayRole) {
		QString text = action->text();
		if(text.endsWith(QStringLiteral("...")))
			text.chop(3);
		return text;
	}
	else if(role == SearchTextRole)
		return QStringLiteral("%1 %2").arg(action->text(), action->statusTip());
	else if(role == ActionRole)
		return QVariant::fromValue(action);
	else if(role == Qt::StatusTipRole)
		return action->statusTip();
	else if(role == Qt::DecorationRole)
		return action->icon();
	else if(role == ShortcutRole)
		return action->shortcut();
	else if(role == Qt::FontRole) {
		static QFont font = QGuiApplication::font();
		font.setBold(true);
		return font;
	}
	return {};
}

/******************************************************************************
* Returns the flags for an item in this list model.
******************************************************************************/
Qt::ItemFlags ActionManager::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractListModel::flags(index);
	QAction* action = _actions[index.row()];
	if(!action->isEnabled())
		flags.setFlag(Qt::ItemIsEnabled, false);
	return flags;
}

/******************************************************************************
* Updates the enabled/disabled state of all actions.
******************************************************************************/
void ActionManager::updateActionStates()
{
	Q_EMIT actionUpdateRequested();
}

/******************************************************************************
* Handles ACTION_EDIT_DELETE command
******************************************************************************/
void ActionManager::on_EditDelete_triggered()
{
	UndoableTransaction::handleExceptions(dataset()->undoStack(), tr("Delete pipeline"), [this]() {
		// Delete all nodes in selection set.
		for(SceneNode* node : dataset()->selection()->nodes())
			node->deleteNode();

		// Automatically select one of the remaining nodes.
		if(dataset()->sceneRoot()->children().isEmpty() == false)
			dataset()->selection()->setNode(dataset()->sceneRoot()->children().front());
	});
}

}	// End of namespace
