////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 OVITO GmbH, Germany
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
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "ImportFileDialog.h"

namespace Ovito {

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
ImportFileDialog::ImportFileDialog(const QVector<const FileImporterClass*>& importerTypes, DataSet* dataset, QWidget* parent, const QString& caption, bool allowMultiSelection, const QString& dialogClass) :
	HistoryFileDialog(dialogClass, parent, caption), _importerTypes(importerTypes)
{
	if(_importerTypes.empty()) {
		dataset->throwException(tr("There are no importer plugins installed."));
	}

	// Build filter string.
	for(auto importerClass : _importerTypes) {
		// Skip importers that want to remain hidden from the user.
		if(importerClass->fileFilterDescription().isEmpty())
			continue;
		_filterStrings << QString("%1 (%2)").arg(importerClass->fileFilterDescription(), importerClass->fileFilter());
	}
	_filterStrings.prepend(tr("<Auto-detect file format> (*)"));

	setNameFilters(_filterStrings);
	setAcceptMode(QFileDialog::AcceptOpen);
	setFileMode(allowMultiSelection ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);

	selectNameFilter(_filterStrings.front());
}

/******************************************************************************
* Returns the file to import after the dialog has been closed with "OK".
******************************************************************************/
QString ImportFileDialog::fileToImport() const
{
	if(_selectedFile.isEmpty()) {
		QStringList filesToImport = selectedFiles();
		if(filesToImport.isEmpty()) return QString();
		return filesToImport.front();
	}
	else return _selectedFile;
}

/******************************************************************************
* Returns the file to import after the dialog has been closed with "OK".
******************************************************************************/
QUrl ImportFileDialog::urlToImport() const
{
	return Application::instance()->fileManager()->urlFromUserInput(fileToImport());
}

/******************************************************************************
* Returns the list of files to import after the dialog has been closed with "OK".
******************************************************************************/
std::vector<QUrl> ImportFileDialog::urlsToImport() const
{
	std::vector<QUrl> list;
	for(const QString& file : selectedFiles()) {
		list.push_back(Application::instance()->fileManager()->urlFromUserInput(file));
	}
	return list;
}

/******************************************************************************
* Returns the selected importer type or NULL if auto-detection is requested.
******************************************************************************/
const FileImporterClass* ImportFileDialog::selectedFileImporterType() const
{
	int importFilterIndex = _filterStrings.indexOf(_selectedFilter.isEmpty() ? selectedNameFilter() : _selectedFilter) - 1;
	OVITO_ASSERT(importFilterIndex >= -1 && importFilterIndex < _importerTypes.size());

	if(importFilterIndex >= 0)
		return _importerTypes[importFilterIndex];
	else
		return nullptr;
}

}	// End of namespace
