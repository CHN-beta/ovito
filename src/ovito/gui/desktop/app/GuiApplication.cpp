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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/actions/ActionManager.h>
#include <ovito/gui/desktop/dataset/GuiDataSetContainer.h>
#include <ovito/gui/desktop/utilities/io/GuiFileManager.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/app/ApplicationService.h>
#include "GuiApplication.h"

namespace Ovito {

/******************************************************************************
* Defines the program's command line parameters.
******************************************************************************/
void GuiApplication::registerCommandLineParameters(QCommandLineParser& parser)
{
	StandaloneApplication::registerCommandLineParameters(parser);

	parser.addOption(QCommandLineOption(QStringList{{"nogui"}}, tr("Run in console mode without showing the graphical user interface.")));
	parser.addOption(QCommandLineOption(QStringList{{"noviewports"}}, tr("Do not create any viewports (for debugging purposes only).")));
}

/******************************************************************************
* Interprets the command line parameters provided to the application.
******************************************************************************/
bool GuiApplication::processCommandLineParameters()
{
	if(!StandaloneApplication::processCommandLineParameters())
		return false;

	// Check if program was started in console mode.
	if(!_cmdLineParser.isSet("nogui")) {
		// Enable GUI mode by default.
		_consoleMode = false;
		_headlessMode = false;
	}
	else {
		// Activate console mode.
		_consoleMode = true;
#if defined(Q_OS_LINUX)
		// On Unix/Linux, console mode means headless mode if no X server is available.
		if(!qEnvironmentVariableIsEmpty("DISPLAY")) {
			_headlessMode = false;
		}
#elif defined(Q_OS_OSX)
		// Don't let Qt move the app to the foreground when running in console mode.
		::setenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1", 1);
		_headlessMode = false;
#elif defined(Q_OS_WIN)
		// On Windows, there is always an OpenGL implementation available for background rendering.
		_headlessMode = false;
#endif
	}

	return true;
}

/******************************************************************************
* Create the global instance of the right QCoreApplication derived class.
******************************************************************************/
void GuiApplication::createQtApplication(int& argc, char** argv)
{
	// OVITO prefers the "C" locale over the system's default locale.
	QLocale::setDefault(QLocale::c());

	if(headlessMode()) {
		StandaloneApplication::createQtApplication(argc, argv);
	}
	else {
		// Enable high-resolution toolbar icons on hi-dpi screens.
		QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#if defined(Q_OS_LINUX)
		// Enforce Fusion UI style on Linux.
		qunsetenv("QT_STYLE_OVERRIDE");
		QApplication::setStyle("Fusion");
#endif

		new QApplication(argc, argv);
	}

	// Set the global default OpenGL surface format.
	// This will let Qt use core profile contexts.
	QSurfaceFormat::setDefaultFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());

	// Process events sent to the Qt application by the OS.
	QCoreApplication::instance()->installEventFilter(this);
}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* GuiApplication::createFileManager()
{
	return new GuiFileManager();
}

/******************************************************************************
* Prepares application to start running.
******************************************************************************/
bool GuiApplication::startupApplication()
{
	if(guiMode()) {
		// Set up graphical user interface.

		// Set the application icon.
		QIcon mainWindowIcon;
		mainWindowIcon.addFile(":/guibase/mainwin/window_icon_256.png");
		mainWindowIcon.addFile(":/guibase/mainwin/window_icon_128.png");
		mainWindowIcon.addFile(":/guibase/mainwin/window_icon_48.png");
		mainWindowIcon.addFile(":/guibase/mainwin/window_icon_32.png");
		mainWindowIcon.addFile(":/guibase/mainwin/window_icon_16.png");
		QApplication::setWindowIcon(mainWindowIcon);

		// Create the main window.
		MainWindow* mainWin = new MainWindow();
		_datasetContainer = &mainWin->datasetContainer();

		// Make the application shutdown as soon as the last main window has been closed.
		QGuiApplication::setQuitOnLastWindowClosed(true);

		// Show the main window.
#ifndef OVITO_DEBUG
		mainWin->showMaximized();
#else
		mainWin->show();
#endif
		mainWin->restoreLayout();

#ifdef OVITO_EXPIRATION_DATE
		QDate expirationDate = QDate::fromString(QStringLiteral(OVITO_EXPIRATION_DATE), Qt::ISODate);
		if(QDate::currentDate() > expirationDate) {
			QMessageBox msgbox(mainWin);
			msgbox.setWindowTitle(tr("Expiration - %1").arg(Application::applicationName()));
			msgbox.setStandardButtons(QMessageBox::Close);
			msgbox.setText(tr("<p>This is a preview version of %1 with a limited life span, which did expire on %2.</p>"
				"<p>Please obtain the final program release, which is now available on our website "
				"<a href=\"https://www.ovito.org/\">www.ovito.org</a>.</p>"
				"<p>This pre-release build of %1 can no longer be used and will quit now.</p>")
					.arg(Application::applicationName())
					.arg(expirationDate.toString(Qt::SystemLocaleLongDate)));
			msgbox.setTextInteractionFlags(Qt::TextBrowserInteraction);
			msgbox.setIcon(QMessageBox::Critical);
			msgbox.exec();
			return false;
		}
#endif
	}
	else {
		// Create a dataset container.
		_datasetContainer = new GuiDataSetContainer();
		_datasetContainer->setParent(this);
	}

	return true;
}

/******************************************************************************
* Is called at program startup once the event loop is running.
******************************************************************************/
void GuiApplication::postStartupInitialization()
{
	GuiDataSetContainer* container = static_object_cast<GuiDataSetContainer>(datasetContainer());

	// Load state file specified on the command line.
	if(cmdLineParser().positionalArguments().empty() == false) {
		QString startupFilename = cmdLineParser().positionalArguments().front();
		if(startupFilename.endsWith(".ovito", Qt::CaseInsensitive)) {
			try {
				container->fileLoad(startupFilename);
			}
			catch(const Exception& ex) {
				ex.reportError();
			}
		}
	}

	// Create an empty dataset if nothing has been loaded.
	if(container->currentSet() == nullptr)
		container->fileNew();

	// Import data file specified on the command line.
	if(cmdLineParser().positionalArguments().empty() == false) {
		QString importFilename = cmdLineParser().positionalArguments().front();
		if(!importFilename.endsWith(".ovito", Qt::CaseInsensitive)) {
			QUrl importURL = Application::instance()->fileManager()->urlFromUserInput(importFilename);
			try {
				container->importFile(importURL);
			}
			catch(const Exception& ex) {
				ex.reportError();
			}
			if(container->currentSet())
				container->currentSet()->undoStack().setClean();
		}
	}

	StandaloneApplication::postStartupInitialization();
}

/******************************************************************************
* Handles events sent to the Qt application object.
******************************************************************************/
bool GuiApplication::eventFilter(QObject* watched, QEvent* event)
{
	if(event->type() == QEvent::FileOpen) {
		QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
		GuiDataSetContainer* container = static_object_cast<GuiDataSetContainer>(datasetContainer());
		try {
			if(openEvent->file().endsWith(".ovito", Qt::CaseInsensitive)) {
				container->fileLoad(openEvent->file());
			}
			else {
				container->importFile(openEvent->url());
				container->currentSet()->undoStack().setClean();
			}
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
	return StandaloneApplication::eventFilter(watched, event);
}

/******************************************************************************
* Handler function for exceptions used in GUI mode.
******************************************************************************/
void GuiApplication::reportError(const Exception& ex, bool blocking)
{
	// Always display errors in the terminal window.
	Application::reportError(ex, blocking);

	if(guiMode()) {
		if(!blocking) {

			// Deferred display of the error.
			if(_errorList.empty())
				QMetaObject::invokeMethod(this, "showErrorMessages", Qt::QueuedConnection);

			// Queue error messages.
			_errorList.push_back(ex);
		}
		else {
			_errorList.push_back(ex);
			showErrorMessages();
		}
	}
}

/******************************************************************************
* Displays an error message box. This slot is called by reportError().
******************************************************************************/
void GuiApplication::showErrorMessages()
{
	while(!_errorList.empty()) {

		// Show next exception from queue.
		const Exception& exception = _errorList.front();

		// Prepare a message box dialog.
		QPointer<QMessageBox> msgbox = new QMessageBox();
		msgbox->setWindowTitle(tr("Error - %1").arg(Application::applicationName()));
		msgbox->setStandardButtons(QMessageBox::Ok);
		msgbox->setText(exception.message());
		msgbox->setIcon(QMessageBox::Critical);

		// If the exception has been thrown within the context of a DataSet or a DataSetContainer,
		// show the message box under the corresponding window.
		QWidget* window;
		if(DataSet* dataset = qobject_cast<DataSet*>(exception.context())) {
			window = MainWindow::fromDataset(dataset);
		}
		else if(GuiDataSetContainer* datasetContainer = qobject_cast<GuiDataSetContainer*>(exception.context())) {
			window = datasetContainer->mainWindow();
		}
		else {
			window = qobject_cast<QWidget*>(exception.context());
		}

		if(window) {

			// Stop animation playback when an error occurred.
			if(MainWindow* mainWindow = qobject_cast<MainWindow*>(window)) {
				QAction* playbackAction = mainWindow->actionManager()->getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK);
				if(playbackAction->isChecked())
					playbackAction->trigger();
			}

			// If there currently is a modal dialog box being shown,
			// make the error message dialog a child of this dialog to prevent a UI dead-lock.
			for(QDialog* dialog : window->findChildren<QDialog*>()) {
				if(dialog->isModal()) {
					window = dialog;
					dialog->show();
					break;
				}
			}

			msgbox->setParent(window);
			msgbox->setWindowModality(Qt::WindowModal);
		}

		// If the exception is associated with additional message strings,
		// show them in the Details section of the message box dialog.
		if(exception.messages().size() > 1) {
			QString detailText;
			for(int i = 1; i < exception.messages().size(); i++)
				detailText += exception.messages()[i] + QStringLiteral("\n");
			msgbox->setDetailedText(detailText);
		}

		// Show message box.
		msgbox->exec();
		if(!msgbox)
			return;
		else
			delete msgbox;
		_errorList.pop_front();
	}
}

}	// End of namespace
