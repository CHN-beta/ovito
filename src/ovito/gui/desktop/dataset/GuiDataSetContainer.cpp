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
#include <ovito/core/dataset/io/FileImporter.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/utilities/io/ObjectSaveStream.h>
#include <ovito/core/utilities/io/ObjectLoadStream.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/dataset/io/FileImporterEditor.h>
#include "GuiDataSetContainer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(GuiDataSetContainer);

/******************************************************************************
* Initializes the dataset manager.
******************************************************************************/
GuiDataSetContainer::GuiDataSetContainer(MainWindow* mainWindow) : DataSetContainer(),
	_mainWindow(mainWindow)
{
	// Prepare scene for display whenever a new dataset becomes active.
	if(Application::instance()->guiMode()) {
		connect(this, &DataSetContainer::dataSetChanged, this, [this](DataSet* dataset) {
			if(dataset) {
				_sceneReadyScheduled = true;
				Q_EMIT scenePreparationBegin();
				_sceneReadyFuture = dataset->whenSceneReady().then(dataset->executor(), [this]() {
					sceneBecameReady();
				});
			}
		});
	}
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool GuiDataSetContainer::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(source == currentSet()) {
		if(Application::instance()->guiMode()) {
			if(event.type() == ReferenceEvent::TargetChanged) {
				// Update viewports as soon as the scene becomes ready.
				if(!_sceneReadyScheduled) {
					_sceneReadyScheduled = true;
					Q_EMIT scenePreparationBegin();
					_sceneReadyFuture = currentSet()->whenSceneReady().then(currentSet()->executor(), [this]() {
						sceneBecameReady();
					});
				}
			}
			else if(event.type() == ReferenceEvent::PreliminaryStateAvailable) {
				// Update viewports when a new preliminiary state from one of the data pipelines
				// becomes available (unless we are playing an animation).
				if(currentSet()->animationSettings()->isPlaybackActive() == false)
					currentSet()->viewportConfig()->updateViewports();
			}
		}
	}
	return DataSetContainer::referenceEvent(source, event);
}

/******************************************************************************
* Is called when scene of the current dataset is ready to be displayed.
******************************************************************************/
void GuiDataSetContainer::sceneBecameReady()
{
	_sceneReadyScheduled = false;
	_sceneReadyFuture.reset();
	if(currentSet())
		currentSet()->viewportConfig()->updateViewports();
	Q_EMIT scenePreparationEnd();
}

/******************************************************************************
* Save the current dataset.
******************************************************************************/
bool GuiDataSetContainer::fileSave()
{
	if(currentSet() == nullptr)
		return false;

	// Ask the user for a filename if there is no one set.
	if(currentSet()->filePath().isEmpty())
		return fileSaveAs();

	// Save dataset to file.
	try {
		currentSet()->saveToFile(currentSet()->filePath());
		currentSet()->undoStack().setClean();
	}
	catch(const Exception& ex) {
		ex.reportError();
		return false;
	}

	return true;
}

/******************************************************************************
* This is the implementation of the "Save As" action.
* Returns true, if the scene has been saved.
******************************************************************************/
bool GuiDataSetContainer::fileSaveAs(const QString& filename)
{
	if(currentSet() == nullptr)
		return false;

	if(filename.isEmpty()) {

		if(!mainWindow())
			currentSet()->throwException(tr("Cannot save session state. No filename has been specified."));

		QFileDialog dialog(mainWindow(), tr("Save Session State As"));
		dialog.setNameFilter(tr("OVITO State Files (*.ovito);;All Files (*)"));
		dialog.setAcceptMode(QFileDialog::AcceptSave);
		dialog.setFileMode(QFileDialog::AnyFile);
		dialog.setConfirmOverwrite(true);
		dialog.setDefaultSuffix("ovito");

		QSettings settings;
		settings.beginGroup("file/scene");

		if(currentSet()->filePath().isEmpty()) {
			QString defaultPath = settings.value("last_directory").toString();
			if(!defaultPath.isEmpty())
				dialog.setDirectory(defaultPath);
		}
		else
			dialog.selectFile(currentSet()->filePath());

		if(!dialog.exec())
			return false;

		QStringList files = dialog.selectedFiles();
		if(files.isEmpty())
			return false;
		QString newFilename = files.front();

		// Remember directory for the next time...
		settings.setValue("last_directory", dialog.directory().absolutePath());

        currentSet()->setFilePath(newFilename);
	}
	else {
		currentSet()->setFilePath(filename);
	}
	return fileSave();
}

/******************************************************************************
* If the scene has been changed this will ask the user if he wants
* to save the changes.
* Returns false if the operation has been canceled by the user.
******************************************************************************/
bool GuiDataSetContainer::askForSaveChanges()
{
	if(!currentSet() || currentSet()->undoStack().isClean() || currentSet()->filePath().isEmpty() || !mainWindow())
		return true;

	QString message;
	if(currentSet()->filePath().isEmpty() == false) {
		message = tr("The current session state has been modified. Do you want to save the changes?");
		message += QString("\n\nFile: %1").arg(currentSet()->filePath());
	}
	else {
		message = tr("The current program session has not been saved. Do you want to save it?");
	}

	QMessageBox::StandardButton result = QMessageBox::question(mainWindow(), tr("Save changes"),
		message,
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
	if(result == QMessageBox::Cancel)
		return false; // Operation canceled by user
	else if(result == QMessageBox::No)
		return true; // Continue without saving scene first.
	else {
		// Save scene first.
        return fileSave();
	}
}

/******************************************************************************
* Creates an empty dataset and makes it the current dataset.
******************************************************************************/
bool GuiDataSetContainer::fileNew()
{
	OORef<DataSet> newSet = new DataSet();
	if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive && Application::instance()->guiMode())
		newSet->loadUserDefaults();
	setCurrentSet(newSet);
	return true;
}

/******************************************************************************
* Loads the given state file.
******************************************************************************/
bool GuiDataSetContainer::fileLoad(const QString& filename)
{
	// Make path absolute.
	QString absoluteFilepath = QFileInfo(filename).absoluteFilePath();

	// Load dataset from file.
	OORef<DataSet> dataSet;
	try {

		QFile fileStream(absoluteFilepath);
		if(!fileStream.open(QIODevice::ReadOnly))
			throw Exception(tr("Failed to open session state file '%1' for reading.").arg(absoluteFilepath), this);

		QDataStream dataStream(&fileStream);
		ObjectLoadStream stream(dataStream, SynchronousOperation::create(taskManager()));

		// Issue a warning when the floating-point precision of the input file does not match
		// the precision used in this build.
		if(stream.floatingPointPrecision() > sizeof(FloatType)) {
			if(mainWindow()) {
				QString msg = tr("The session state file has been written with a version of this program that uses %1-bit floating-point precision. "
					   "The version of this program that you are currently using only supports %2-bit precision numbers. "
					   "The precision of all numbers stored in the input file will be truncated during loading.").arg(stream.floatingPointPrecision()*8).arg(sizeof(FloatType)*8);
				QMessageBox::warning(mainWindow(), tr("Floating-point precision mismatch"), msg);
			}
		}

		dataSet = stream.loadObject<DataSet>();
		stream.close();

		if(!dataSet)
			throw Exception(tr("Session state file '%1' does not contain a dataset.").arg(absoluteFilepath), this);
	}
	catch(Exception& ex) {
		// Provide a local context for the error.
		ex.setContext(this);
		throw ex;
	}
	OVITO_CHECK_OBJECT_POINTER(dataSet);
	dataSet->setFilePath(absoluteFilepath);
	setCurrentSet(dataSet);
	return true;
}

/******************************************************************************
* Imports a given file into the scene.
******************************************************************************/
bool GuiDataSetContainer::importFile(const QUrl& url, const FileImporterClass* importerType)
{
	OVITO_ASSERT(currentSet() != nullptr);

	if(!url.isValid())
		throw Exception(tr("Failed to import file. URL is not valid: %1").arg(url.toString()), currentSet());

	OORef<FileImporter> importer;
	if(!importerType) {

		// Detect file format.
		Future<OORef<FileImporter>> importerFuture = FileImporter::autodetectFileFormat(currentSet(), url);
		if(!taskManager().waitForFuture(importerFuture))
			return false;

		importer = importerFuture.result();
		if(!importer)
			currentSet()->throwException(tr("Could not detect the format of the file to be imported. The format might not be supported."));
	}
	else {
		importer = static_object_cast<FileImporter>(importerType->createInstance(currentSet()));
		if(!importer)
			currentSet()->throwException(tr("Failed to import file. Could not initialize import service."));
	}

	// Load user-defined default settings for the importer.
	importer->loadUserDefaults();

	// Show the optional user interface (which is provided by the corresponding FileImporterEditor class) for the new importer.
	for(OvitoClassPtr clazz = &importer->getOOClass(); clazz != nullptr; clazz = clazz->superClass()) {
		OvitoClassPtr editorClass = PropertiesEditor::registry().getEditorClass(clazz);
		if(editorClass && editorClass->isDerivedFrom(FileImporterEditor::OOClass())) {
			OORef<FileImporterEditor> editor = dynamic_object_cast<FileImporterEditor>(editorClass->createInstance(nullptr));
			if(editor) {
				if(!editor->inspectNewFile(importer, url, mainWindow()))
					return false;
			}
		}
	}

	// Determine how the file's data should be inserted into the current scene.
	FileImporter::ImportMode importMode = FileImporter::ResetScene;

	if(mainWindow()) {
		if(importer->isReplaceExistingPossible(url)) {
			// Ask user if the current import node including any applied modifiers should be kept.
			QMessageBox msgBox(QMessageBox::Question, tr("Import file"),
					tr("When importing the selected file, do you want to keep the existing objects?"),
					QMessageBox::NoButton, mainWindow());

			QPushButton* cancelButton = msgBox.addButton(QMessageBox::Cancel);
			QPushButton* resetSceneButton = msgBox.addButton(tr("No"), QMessageBox::NoRole);
			QPushButton* addToSceneButton = msgBox.addButton(tr("Add to scene"), QMessageBox::YesRole);
			QPushButton* replaceSourceButton = msgBox.addButton(tr("Replace selected"), QMessageBox::AcceptRole);
			msgBox.setDefaultButton(resetSceneButton);
			msgBox.setEscapeButton(cancelButton);
			msgBox.exec();

			if(msgBox.clickedButton() == cancelButton) {
				return false; // Operation canceled by user.
			}
			else if(msgBox.clickedButton() == resetSceneButton) {
				importMode = FileImporter::ResetScene;
				// Ask user if current scene should be saved before it is replaced by the imported data.
				if(!askForSaveChanges())
					return false;
			}
			else if(msgBox.clickedButton() == addToSceneButton) {
				importMode = FileImporter::AddToScene;
			}
			else {
				importMode = FileImporter::ReplaceSelected;
			}
		}
		else if(currentSet()->sceneRoot()->children().empty() == false) {
			// Ask user if the current scene should be completely replaced by the imported data.
			QMessageBox::StandardButton result = QMessageBox::question(mainWindow(), tr("Import file"),
				tr("Do you want to keep the existing objects in the current scene?"),
				QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Cancel);

			if(result == QMessageBox::Cancel) {
				return false; // Operation canceled by user.
			}
			else if(result == QMessageBox::No) {
				importMode = FileImporter::ResetScene;

				// Ask user if current scene should be saved before it is replaced by the imported data.
				if(!askForSaveChanges())
					return false;
			}
			else {
				importMode = FileImporter::AddToScene;
			}
		}
	}

	return importer->importFile({url}, importMode, true);
}

}	// End of namespace
