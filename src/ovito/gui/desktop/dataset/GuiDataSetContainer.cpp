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
GuiDataSetContainer::GuiDataSetContainer(MainWindow* mainWindow) : _mainWindow(mainWindow)
{
}

/******************************************************************************
* Returns the graphical user interface this dataset container is associated with.
******************************************************************************/
UserInterface* GuiDataSetContainer::guiInterface() 
{ 
	return _mainWindow; 
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
* Imports a given file into the scene.
******************************************************************************/
bool GuiDataSetContainer::importFiles(const std::vector<QUrl>& urls, const FileImporterClass* importerType)
{
	OVITO_ASSERT(currentSet() != nullptr);
	OVITO_ASSERT(!urls.empty());

	std::vector<std::pair<QUrl, OORef<FileImporter>>> urlImporters;
	for(const QUrl& url : urls) {
		if(!url.isValid())
			throw Exception(tr("Failed to import file. URL is not valid: %1").arg(url.toString()), currentSet());

		OORef<FileImporter> importer;
		if(!importerType) {

			// Detect file format.
			Future<OORef<FileImporter>> importerFuture = FileImporter::autodetectFileFormat(currentSet(), ExecutionContext::Interactive, url);
			if(!taskManager().waitForFuture(importerFuture))
				return false;

			importer = importerFuture.result();
			if(!importer)
				currentSet()->throwException(tr("Could not auto-detect the format of the file %1. The file format might not be supported.").arg(url.fileName()));
		}
		else {
			importer = static_object_cast<FileImporter>(importerType->createInstance(currentSet(), ExecutionContext::Interactive));
			if(!importer)
				currentSet()->throwException(tr("Failed to import file. Could not initialize import service."));
		}

		urlImporters.push_back(std::make_pair(url, std::move(importer)));
	}

	// Order URLs and their corresponding importers.
	std::stable_sort(urlImporters.begin(), urlImporters.end(), [](const auto& a, const auto& b) {
		int pa = a.second->importerPriority();
		int pb = b.second->importerPriority();
		if(pa > pb) return true;
		if(pa < pb) return false;
		return a.second->getOOClass().name() < b.second->getOOClass().name();
	});

	// Display the optional UI (which is provided by the corresponding FileImporterEditor class) for each importer.
	if(mainWindow()) {
		for(const auto& item : urlImporters) {
			const QUrl& url = item.first;
			const OORef<FileImporter>& importer = item.second;
			for(OvitoClassPtr clazz = &importer->getOOClass(); clazz != nullptr; clazz = clazz->superClass()) {
				OvitoClassPtr editorClass = PropertiesEditor::registry().getEditorClass(clazz);
				if(editorClass && editorClass->isDerivedFrom(FileImporterEditor::OOClass())) {
					OORef<FileImporterEditor> editor = dynamic_object_cast<FileImporterEditor>(editorClass->createInstance());
					if(editor) {
						if(!editor->inspectNewFile(importer, url, mainWindow()))
							return false;
					}
				}
			}
		}
	}

	// Determine how the file's data should be inserted into the current scene.
	FileImporter::ImportMode importMode = FileImporter::ResetScene;

	const QUrl& url = urlImporters.front().first;
	OORef<FileImporter> importer = urlImporters.front().second;
	if(mainWindow()) {
		if(importer->isReplaceExistingPossible(urls)) {
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

	return importer->importFileSet(std::move(urlImporters), importMode, true);
}

}	// End of namespace
