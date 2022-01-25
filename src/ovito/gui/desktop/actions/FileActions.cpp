////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
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
#include <ovito/gui/desktop/actions/WidgetActionManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/dialogs/ApplicationSettingsDialog.h>
#include <ovito/gui/desktop/dialogs/ImportFileDialog.h>
#include <ovito/gui/desktop/dialogs/ImportRemoteFileDialog.h>
#include <ovito/gui/desktop/dialogs/FileExporterSettingsDialog.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/io/FileImporter.h>
#include <ovito/core/dataset/io/FileExporter.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>

namespace Ovito {

/******************************************************************************
* Handles the ACTION_QUIT command.
******************************************************************************/
void WidgetActionManager::on_Quit_triggered()
{
	mainWindow().close();
}

/******************************************************************************
* Handles the ACTION_HELP_ABOUT command.
******************************************************************************/
void WidgetActionManager::on_HelpAbout_triggered()
{
	QMessageBox msgBox(QMessageBox::NoIcon, Application::applicationName(),
			tr("<h3>%1 (Open Visualization Tool)</h3>"
				"<p>Version %2</p>").arg(Application::applicationName()).arg(Application::applicationVersionString()),
			QMessageBox::Ok, &mainWindow());
	QString informativeText = QStringLiteral(OVITO_COPYRIGHT_NOTICE);
	// Use the dynamic properties attached to the global Application object to replace any
	// placeholders in the copyright notice with text strings generated by plugins at runtime. 
	for(const QByteArray& pname : Application::instance()->dynamicPropertyNames()) {
		QString value = Application::instance()->property(pname.data()).toString();
		informativeText.replace(QStringLiteral("[[%1]]").arg(QString::fromLatin1(pname)), value);
	}
	msgBox.setInformativeText(informativeText);
	msgBox.setDefaultButton(QMessageBox::Ok);
	QPixmap icon = QApplication::windowIcon().pixmap(32 * mainWindow().devicePixelRatio());
	icon.setDevicePixelRatio(mainWindow().devicePixelRatio());
	msgBox.setIconPixmap(icon);
	msgBox.exec();
}

/******************************************************************************
* Handles the ACTION_HELP_SHOW_ONLINE_HELP command.
******************************************************************************/
void WidgetActionManager::on_HelpShowOnlineHelp_triggered()
{
	ActionManager::openHelpTopic(QString());
}

/******************************************************************************
* Handles the ACTION_HELP_SHOW_SCRIPTING_HELP command.
******************************************************************************/
void WidgetActionManager::on_HelpShowScriptingReference_triggered()
{
	ActionManager::openHelpTopic(QStringLiteral("python/index.html"));
}

/******************************************************************************
* Handles the ACTION_HELP_GRAPHICS_SYSINFO command.
******************************************************************************/
void WidgetActionManager::on_HelpSystemInfo_triggered()
{
	QDialog dlg(&mainWindow());
	dlg.setWindowTitle(tr("System Information"));
	QVBoxLayout* layout = new QVBoxLayout(&dlg);
	QTextEdit* textEdit = new QTextEdit(&dlg);
	textEdit->setReadOnly(true);
	textEdit->setPlainText(mainWindow().generateSystemReport());
	textEdit->setMinimumSize(QSize(600, 400));
	layout->addWidget(textEdit);
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dlg);
	connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
	connect(buttonBox->addButton(tr("Copy to clipboard"), QDialogButtonBox::ActionRole), &QPushButton::clicked, [textEdit]() { QApplication::clipboard()->setText(textEdit->toPlainText()); });
	layout->addWidget(buttonBox);
	dlg.exec();
}

/******************************************************************************
* Handles the ACTION_FILE_NEW_WINDOW command.
******************************************************************************/
void WidgetActionManager::on_FileNewWindow_triggered()
{
	try {
		MainWindow* mainWin = new MainWindow();
		mainWin->show();
		mainWin->restoreLayout();

		// Optionally load the user's default state from the standard location.
		QString defaultsFilePath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("defaults.ovito"));
		if(!defaultsFilePath.isEmpty()) {
			try {
				mainWin->datasetContainer().loadDataset(defaultsFilePath, MainThreadOperation::create(*mainWin));
				mainWin->datasetContainer().currentSet()->setFilePath({});
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("An error occured while loading the user's default session state from the file: %1").arg(defaultsFilePath));
				ex.reportError();
			}
		}

		if(mainWin->datasetContainer().currentSet() == nullptr) {
			mainWin->datasetContainer().newDataset();
		}
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Handles the ACTION_FILE_OPEN command.
******************************************************************************/
void WidgetActionManager::on_FileOpen_triggered()
{
	try {
		if(!mainWindow().datasetContainer().askForSaveChanges())
			return;

		QSettings settings;
		settings.beginGroup("file/scene");

		// Go to the last directory used.
		QString defaultPath;
		OORef<DataSet> dataSet = mainWindow().datasetContainer().currentSet();
		if(dataSet == NULL || dataSet->filePath().isEmpty())
			defaultPath = settings.value("last_directory").toString();
		else
			defaultPath = dataSet->filePath();

		QString filename = QFileDialog::getOpenFileName(&mainWindow(), tr("Load Session State"),
				defaultPath, tr("OVITO State Files (*.ovito);;All Files (*)"));
		if(filename.isEmpty())
			return;

		// Remember directory for the next time...
		settings.setValue("last_directory", QFileInfo(filename).absolutePath());

		mainWindow().datasetContainer().loadDataset(filename, MainThreadOperation::create(mainWindow(), true));
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Handles the ACTION_FILE_SAVE command.
******************************************************************************/
void WidgetActionManager::on_FileSave_triggered()
{
	// Set focus to main window.
	// This will process any pending user inputs in QLineEdit fields.
	mainWindow().setFocus();

	try {
		mainWindow().datasetContainer().fileSave();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Handles the ACTION_FILE_SAVEAS command.
******************************************************************************/
void WidgetActionManager::on_FileSaveAs_triggered()
{
	try {
		mainWindow().datasetContainer().fileSaveAs();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Handles the ACTION_SETTINGS_DIALOG command.
******************************************************************************/
void WidgetActionManager::on_Settings_triggered()
{
	ApplicationSettingsDialog dlg(mainWindow());
	dlg.exec();
}

/******************************************************************************
* Handles the ACTION_FILE_IMPORT command.
******************************************************************************/
void WidgetActionManager::on_FileImport_triggered()
{
	try {
		// Let the user select one or more files.
		ImportFileDialog dialog(PluginManager::instance().metaclassMembers<FileImporter>(), dataset(), &mainWindow(), tr("Load File"), true);
		if(dialog.exec() != QDialog::Accepted)
			return;

		// Import file(s).
		mainWindow().datasetContainer().importFiles(
			dialog.urlsToImport(), 
			MainThreadOperation::create(mainWindow(), true),
			dialog.selectedFileImporterType());
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Handles the ACTION_FILE_REMOTE_IMPORT command.
******************************************************************************/
void WidgetActionManager::on_FileRemoteImport_triggered()
{
	try {
		// Let the user enter the URL of the remote file.
		ImportRemoteFileDialog dialog(PluginManager::instance().metaclassMembers<FileImporter>(), dataset(), &mainWindow(), tr("Load Remote File"));
		if(dialog.exec() != QDialog::Accepted)
			return;

		// Import URL.
		mainWindow().datasetContainer().importFiles(
			{ dialog.urlToImport() }, 
			MainThreadOperation::create(mainWindow(), true),
			dialog.selectedFileImporterType());
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Handles the ACTION_FILE_EXPORT command.
******************************************************************************/
void WidgetActionManager::on_FileExport_triggered()
{
	// Build filter string.
	QStringList filterStrings;
	QVector<const FileExporterClass*> exporterTypes = PluginManager::instance().metaclassMembers<FileExporter>();
	if(exporterTypes.empty()) {
		Exception(tr("This function is disabled, because no file exporter plugins have been installed."), dataset()).reportError();
		return;
	}
	std::sort(exporterTypes.begin(), exporterTypes.end(), [](const FileExporterClass* a, const FileExporterClass* b) {
		return a->fileFilterDescription() < b->fileFilterDescription();
	});
	for(const FileExporterClass* exporterClass : exporterTypes) {
		// Skip exporters that want to remain hidden from the user.
		if(exporterClass->fileFilterDescription().isEmpty())
			continue;
#ifndef Q_OS_WIN
		filterStrings << QStringLiteral("%1 (%2)").arg(exporterClass->fileFilterDescription(), exporterClass->fileFilter());
#else
		// Workaround for bug in Windows file selection dialog (https://bugreports.qt.io/browse/QTBUG-45759)
		filterStrings << QStringLiteral("%1 (*)").arg(exporterClass->fileFilterDescription());
#endif
	}

	QSettings settings;
	settings.beginGroup("file/export");

	// Let the user select a destination file.
	HistoryFileDialog dialog("export", &mainWindow(), tr("Export Data"));
	dialog.setNameFilters(filterStrings);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);

	// Go to the last directory used.
	QString lastExportDirectory = settings.value("last_export_dir").toString();
	if(!lastExportDirectory.isEmpty())
		dialog.setDirectory(lastExportDirectory);
	// Select the last export filter being used ...
	QString lastExportFilter = settings.value("last_export_filter").toString();
	if(!lastExportFilter.isEmpty())
		dialog.selectNameFilter(lastExportFilter);

	if(!dialog.exec())
		return;

	QStringList files = dialog.selectedFiles();
	if(files.isEmpty())
		return;

	QString exportFile = files.front();

	// Remember directory for the next time...
	settings.setValue("last_export_dir", dialog.directory().absolutePath());
	// Remember export filter for the next time...
	settings.setValue("last_export_filter", dialog.selectedNameFilter());

	// Export to selected file.
	try {
		int exportFilterIndex = filterStrings.indexOf(dialog.selectedNameFilter());
		OVITO_ASSERT(exportFilterIndex >= 0 && exportFilterIndex < exporterTypes.size());

		// Create exporter and initialize it.
		OORef<FileExporter> exporter = static_object_cast<FileExporter>(exporterTypes[exportFilterIndex]->createInstance(dataset()));

		// Pass output filename to exporter.
		exporter->setOutputFilename(exportFile);

		// Block until the scene is ready.
		{
			ProgressDialog progressDialog(&mainWindow(), mainWindow(), tr("Waiting for pipelines to complete"));
			if(!progressDialog.waitForFuture(dataset()->whenSceneReady()))
				return;
		}

		// Choose the pipelines to be exported.
		exporter->selectDefaultExportableData();

		// Let the user adjust the settings of the exporter.
		FileExporterSettingsDialog settingsDialog(mainWindow(), exporter);
		if(settingsDialog.exec() != QDialog::Accepted)
			return;

		// Show progress dialog.
		ProgressDialog progressDialog(&mainWindow(), mainWindow(), tr("Exporting to file"));

		// Let the exporter do its work.
		exporter->doExport(progressDialog);
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

}	// End of namespace
