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

#pragma once


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/dataset/GuiDataSetContainer.h>
#include <ovito/core/app/UserInterface.h>

namespace Ovito {

/**
 * \brief The main window of the application.
 *
 * Note that is is possible to open multiple main windows per
 * application instance to edit multiple datasets simultaneously.
 */
class OVITO_GUI_EXPORT MainWindow : public QMainWindow, public UserInterface
{
	Q_OBJECT

public:

	/// The pages of the command panel.
	enum CommandPanelPage {
		MODIFY_PAGE		= 0,
		RENDER_PAGE		= 1,
		OVERLAY_PAGE	= 2
	};

public:

	/// Constructor of the main window class.
	MainWindow();

	/// Destructor.
	virtual ~MainWindow();

	/// Returns the main toolbar of the window.
	QToolBar* mainToolbar() const { return _mainToolbar; }

	/// Displays a message string in the window's status bar.
	virtual void showStatusBarMessage(const QString& message, int timeout = 0) override;

	/// Hides any messages currently displayed in the window's status bar.
	virtual void clearStatusBarMessage() override;

	/// Gives the active viewport the input focus.
	virtual void setViewportInputFocus() override;

	/// Closes the user interface and shuts down the entire application after displaying an error message.
	virtual void exitWithFatalError(const Exception& ex) override;

	/// Creates a frame buffer of the requested size for rendering and displays it in a window in the user interface.
	virtual std::shared_ptr<FrameBuffer> createAndShowFrameBuffer(int width, int height, MainThreadOperation& renderingOperation) override;

	/// Returns the frame buffer window for displaying the rendered image (may be null).
	FrameBufferWindow* frameBufferWindow() const { return _frameBufferWindow; }

	/// Returns the recommended size for this window.
	virtual QSize sizeHint() const override { return QSize(1024,768); }

	/// Loads the layout of the docked widgets from the settings store.
	void restoreLayout();

	/// Saves the layout of the docked widgets to the settings store.
	void saveLayout();

	/// Immediately repaints all viewports that are flagged for an update.
	void processViewportUpdates();

	/// Returns the container that keeps a reference to the current dataset.
	GuiDataSetContainer& datasetContainer() { return _datasetContainer; }

	/// Returns the widget that numerically displays the transformation.
	CoordinateDisplayWidget* coordinateDisplay() const { return _coordinateDisplay; }

	/// Returns the container widget for viewports.
	ViewportsPanel* viewportsPanel() const { return _viewportsPanel; }

	/// Returns the layout manager for the status bar area of the main window.
	QHBoxLayout* statusBarLayout() const { return _statusBarLayout; }

	/// Returns the page of the command panel that is currently visible.
	CommandPanelPage currentCommandPanelPage() const;

	/// Sets the page of the command panel that is currently visible.
	void setCurrentCommandPanelPage(CommandPanelPage page);

	/// Provides access to the main window's command panel.
	CommandPanel* commandPanel() const { return _commandPanel; }

	/// Opens the data inspector panel and shows the data object generated by the given data source.
	bool openDataInspector(PipelineObject* dataSource, const QString& objectNameHint = {}, const QVariant& modeHint = {});

	/// Sets the file path associated with this window and updates the window's title.
	void setWindowFilePath(const QString& filePath);

	/// Determines whether the application window uses a dark theme.
	bool darkTheme() const;

protected:

	/// Is called when the user closes the window.
	virtual void closeEvent(QCloseEvent* event) override;

	/// Is called when the window receives an event.
	virtual bool event(QEvent *event) override;

	/// Handles global key input.
	virtual void keyPressEvent(QKeyEvent* event) override;

	/// Called by the system when a drag is in progress and the mouse enters this window.
	virtual void dragEnterEvent(QDragEnterEvent* event) override;

	/// Called by the system when the drag is dropped on this window.
	virtual void dropEvent(QDropEvent* event) override;

private:

	/// Creates the main menu.
	void createMainMenu();

	/// Creates the main toolbar.
	void createMainToolbar();

	/// Creates a dock panel.
	QDockWidget* createDockPanel(const QString& caption, const QString& objectName, Qt::DockWidgetArea dockArea, Qt::DockWidgetAreas allowedAreas, QWidget* contents);

private:

	/// The upper main toolbar.
	QToolBar* _mainToolbar;

	/// The internal status bar widget.
	StatusBar* _statusBar;

	/// The frame buffer window displaying the rendered image.
	FrameBufferWindow* _frameBufferWindow = nullptr;

	/// The command panel.
	CommandPanel* _commandPanel;

	/// Manages the asynchronous tasks running in the context of this window.
	TaskManager _taskManager;

	/// Container that keeps a reference to the current dataset.
	GuiDataSetContainer _datasetContainer;

	/// The container widget for viewports.
	ViewportsPanel* _viewportsPanel;

	/// The widget that numerically displays the object transformation.
	CoordinateDisplayWidget* _coordinateDisplay;

	/// The layout manager for the status bar area of the main window.
	QHBoxLayout* _statusBarLayout;

	/// The OpenGL context used for rendering the viewports.
	QPointer<QOpenGLContext> _glcontext;

	/// The UI panel containing the data inspector tabs.
	DataInspectorPanel* _dataInspector;

	/// The title string to use for the main window (without any dynamic content).
	QString _baseWindowTitle;
};

}	// End of namespace
