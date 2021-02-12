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

#pragma once


#include <ovito/gui/base/GUIBase.h>
#include <ovito/core/dataset/animation/TimeInterval.h>

namespace Ovito {

//////////////////////// Action identifiers ///////////////////////////

/// This action closes the main window and exits the application.
#define ACTION_QUIT				"Quit"
/// This action shows the file open dialog.
#define ACTION_FILE_OPEN		"FileOpen"
/// This action saves the current file.
#define ACTION_FILE_SAVE		"FileSave"
/// This action shows the file save as dialog.
#define ACTION_FILE_SAVEAS		"FileSaveAs"
/// This action shows the file import dialog.
#define ACTION_FILE_IMPORT		"FileImport"
/// This action shows the remote file import dialog.
#define ACTION_FILE_REMOTE_IMPORT "FileRemoteImport"
/// This action shows the file export dialog.
#define ACTION_FILE_EXPORT		"FileExport"
/// This action opens another main window.
#define ACTION_FILE_NEW_WINDOW	"FileNewWindow"

/// This action shows the about dialog.
#define ACTION_HELP_ABOUT				"HelpAbout"
/// This action shows the online help.
#define ACTION_HELP_SHOW_ONLINE_HELP	"HelpShowOnlineHelp"
/// This action shows the scripting reference manual.
#define ACTION_HELP_SHOW_SCRIPTING_HELP	"HelpShowScriptingReference"
/// This action displays OpenGL diagnostics.
#define ACTION_HELP_OPENGL_INFO			"HelpOpenGLInfo"

/// This action undoes the last operation.
#define ACTION_EDIT_UNDO				"EditUndo"
/// This action does the last undone operation again.
#define ACTION_EDIT_REDO				"EditRedo"
/// This action deletes the selected scene object.
#define ACTION_EDIT_DELETE				"EditDelete"
/// This action duplicates the selected scene object.
#define ACTION_EDIT_CLONE_PIPELINE		"ClonePipeline"
/// This action opens the rename pipeline dialog.
#define ACTION_EDIT_RENAME_PIPELINE		"RenamePipeline"
/// This action clears the current undo stack.
#define ACTION_EDIT_CLEAR_UNDO_STACK	"EditClearUndoStack"

/// This action maximizes the active viewport.
#define ACTION_VIEWPORT_MAXIMIZE					"ViewportMaximize"
/// This action activates the viewport zoom mode.
#define ACTION_VIEWPORT_ZOOM						"ViewportZoom"
/// This action activates the viewport pan mode.
#define ACTION_VIEWPORT_PAN							"ViewportPan"
/// This action activates the viewport orbit mode.
#define ACTION_VIEWPORT_ORBIT						"ViewportOrbit"
/// This action activates the field of view viewport mode.
#define ACTION_VIEWPORT_FOV							"ViewportFOV"
/// This action activates the 'pick center of rotation' input mode.
#define ACTION_VIEWPORT_PICK_ORBIT_CENTER			"ViewportOrbitPickCenter"
/// This zooms the current viewport to the scene extents
#define ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS			"ViewportZoomSceneExtents"
/// This zooms the current viewport to the selection extents
#define ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS		"ViewportZoomSelectionExtents"
/// This zooms all viewports to the scene extents
#define ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS_ALL		"ViewportZoomSceneExtentsAll"
/// This zooms all viewports to the selection extents
#define ACTION_VIEWPORT_ZOOM_SELECTION_EXTENTS_ALL	"ViewportZoomSelectionExtentsAll"

/// This action deletes the currently selected modifier from the modifier stack.
#define ACTION_MODIFIER_DELETE				"ModifierDelete"
/// This action moves the currently selected modifer up one entry in the modifier stack.
#define ACTION_MODIFIER_MOVE_UP				"ModifierMoveUp"
/// This action moves the currently selected modifer down one entry in the modifier stack.
#define ACTION_MODIFIER_MOVE_DOWN			"ModifierMoveDown"
/// This action opens the dialog box for managing modifier templates.
#define ACTION_MODIFIER_MANAGE_TEMPLATES	"ModifierManageTemplates"
/// This action creates a unique copy of the selected pipeline item.
#define ACTION_PIPELINE_MAKE_INDEPENDENT	"PipelineMakeUnique"
/// This action creates or dissolves a modifier group in the pipeline editor.
#define ACTION_PIPELINE_TOGGLE_MODIFIER_GROUP	"PipelineToggleModifierGroup"

/// This action deletes the currently selected viewport layer.
#define ACTION_VIEWPORT_LAYER_DELETE			"ViewportLayerDelete"
/// This action moves the currently selected viewport layer up one entry in the stack.
#define ACTION_VIEWPORT_LAYER_MOVE_UP			"ViewportLayerMoveUp"
/// This action moves the currently selected viewport layer down one entry in the stack.
#define ACTION_VIEWPORT_LAYER_MOVE_DOWN			"ViewportLayerMoveDown"

/// This action jumps to the start of the animation
#define ACTION_GOTO_START_OF_ANIMATION		"AnimationGotoStart"
/// This action jumps to the end of the animation
#define ACTION_GOTO_END_OF_ANIMATION		"AnimationGotoEnd"
/// This action jumps to previous frame in the animation
#define ACTION_GOTO_PREVIOUS_FRAME			"AnimationGotoPreviousFrame"
/// This action jumps to next frame in the animation
#define ACTION_GOTO_NEXT_FRAME				"AnimationGotoNextFrame"
/// This action toggles animation playback
#define ACTION_TOGGLE_ANIMATION_PLAYBACK	"AnimationTogglePlayback"
/// This action starts the animation playback
#define ACTION_START_ANIMATION_PLAYBACK		"AnimationStartPlayback"
/// This action starts the animation playback
#define ACTION_STOP_ANIMATION_PLAYBACK		"AnimationStopPlayback"
/// This action shows the animation settings dialog
#define ACTION_ANIMATION_SETTINGS			"AnimationSettings"
/// This action activates/deactivates the animation mode
#define ACTION_AUTO_KEY_MODE_TOGGLE			"AnimationToggleRecording"

/// This action starts rendering of the current view.
#define ACTION_RENDER_ACTIVE_VIEWPORT		"RenderActiveViewport"
/// This action displays the frame buffer windows showing the last rendered image.
#define ACTION_SHOW_FRAME_BUFFER			"ShowFrameBuffer"

/// This actions open the application's "Settings" dialog.
#define ACTION_SETTINGS_DIALOG				"Settings"
/// Opens a list of commands for quick access by the user.
#define ACTION_COMMAND_QUICKSEARCH			"CommandQuickSearch"

/// This actions activates the scene node selection mode.
#define ACTION_SELECTION_MODE				"SelectionMode"
/// This actions activates the scene node translation mode.
#define ACTION_XFORM_MOVE_MODE				"XFormMoveMode"
/// This actions activates the scene node rotation mode.
#define ACTION_XFORM_ROTATE_MODE			"XFormRotateMode"

/// This actions lets the user select a script file to run.
#define ACTION_SCRIPTING_RUN_FILE			"ScriptingRunFile"
/// This actions lets the user generate script code from the selected data pipeline.
#define ACTION_SCRIPTING_GENERATE_CODE		"ScriptingGenerateCode"

/**
 * \brief Manages all available user interface actions.
 */
class OVITO_GUIBASE_EXPORT ActionManager : public QAbstractListModel
{
	Q_OBJECT

public:

	/// Item model roles supported by this QAbstractListModel.
	enum ModelRoles {
		ActionRole = Qt::UserRole,	///< Pointer to the QAction object.
		ShortcutRole,				///< QKeySequence of the action's shortcut. 
		SearchTextRole				///< The text string used for seaching commands.
	};

	/// Constructor.
	ActionManager(QObject* parent, MainWindowInterface* mainWindow);

	/// Returns the main window this action manager belongs to.
	MainWindowInterface* mainWindow() const { return _mainWindow; }

	/// Returns dataset currently being edited in the main window.
	DataSet* dataset() const;

	/// \brief Returns the action with the given ID or NULL.
	/// \param actionId The identifier string of the action to return.
	QAction* findAction(const QString& actionId) const {
		return findChild<QAction*>(actionId);
	}

	/// \brief Returns the action with the given ID.
	/// \param actionId The unique identifier string of the action to return.
	QAction* getAction(const QString& actionId) const {
		QAction* action = findAction(actionId);
		OVITO_ASSERT_MSG(action != nullptr, "ActionManager::getAction()", "Action does not exist.");
		return action;
	}

	/// \brief Invokes the command action with the given ID.
	/// \param actionId The unique identifier string of the action to invoke.
	/// \throw Exception if the action does not exist or if an error occurs during the action invocation.
	void invokeAction(const QString& actionId);

	/// \brief Registers a new action with the ActionManager.
	/// \param action The action to be registered. The ActionManager will take ownership of the object.
	void addAction(QAction* action);

	/// \brief Creates and registers a new command action with the ActionManager.
	QAction* createCommandAction(const QString& id,
						const QString& title,
						const char* iconPath = nullptr,
						const QString& statusTip = QString(),
						const QKeySequence& shortcut = QKeySequence());

	/// \brief Creates and registers a new action with the ActionManager.
	QAction* createViewportModeAction(const QString& id,
						ViewportInputMode* inputHandler,
						const QString& title,
						const char* iconPath = nullptr,
						const QString& statusTip = QString(),
						const QKeySequence& shortcut = QKeySequence());

	/// \brief Removes the given action from the ActionManager and deletes it.
	/// \param action The action to be deletes.
	void deleteAction(QAction* action);

	/// \brief Returns the number of rows in this list model.
	virtual int rowCount(const QModelIndex& parent) const override { return _actions.size(); }

	/// \brief Returns the data stored in this list model under the given role.
	virtual QVariant data(const QModelIndex& index, int role) const override;

	/// \brief Returns the flags for an item in this list model.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

Q_SIGNALS:

	/// \brief This signal is emitted by the ActionManager when the quick command search is activated. It tells the system to refresh the enabled/disabled state of actions as needed. 
	void actionUpdateRequested();

private Q_SLOTS:

	/// This is called when a new dataset has been loaded.
	void onDataSetChanged(DataSet* newDataSet);

	/// This is called when new animation settings have been loaded.
	void onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings);

	/// This is called when the active animation interval has changed.
	void onAnimationIntervalChanged(TimeInterval newAnimationInterval);

	/// This is called whenever the scene node selection changed.
	void onSelectionChangeComplete(SelectionSet* selection);

	void on_ViewportMaximize_triggered();
	void on_ViewportZoomSceneExtents_triggered();
	void on_ViewportZoomSelectionExtents_triggered();
	void on_ViewportZoomSceneExtentsAll_triggered();
	void on_ViewportZoomSelectionExtentsAll_triggered();
	void on_AnimationGotoStart_triggered();
	void on_AnimationGotoEnd_triggered();
	void on_AnimationGotoPreviousFrame_triggered();
	void on_AnimationGotoNextFrame_triggered();
	void on_AnimationStartPlayback_triggered();
	void on_AnimationStopPlayback_triggered();
	void on_EditDelete_triggered();

protected:

	void updateActionStates();

private:

	/// The main window that owns this action manager.
	MainWindowInterface* _mainWindow;

	/// The list of registered actions.
	QVector<QAction*> _actions;

	QMetaObject::Connection _canUndoChangedConnection;
	QMetaObject::Connection _canRedoChangedConnection;
	QMetaObject::Connection _undoTextChangedConnection;
	QMetaObject::Connection _redoTextChangedConnection;
	QMetaObject::Connection _undoTriggeredConnection;
	QMetaObject::Connection _redoTriggeredConnection;
	QMetaObject::Connection _clearUndoStackTriggeredConnection;
	QMetaObject::Connection _autoKeyModeChangedConnection;
	QMetaObject::Connection _autoKeyModeToggledConnection;
	QMetaObject::Connection _animationIntervalChangedConnection;
	QMetaObject::Connection _animationPlaybackChangedConnection;
	QMetaObject::Connection _animationPlaybackToggledConnection;
};

}	// End of namespace
