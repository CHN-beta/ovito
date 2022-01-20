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

#include <ovito/core/Core.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>
#include <ovito/core/app/Application.h>
#include "FileImporter.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(FileImporter);

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
Future<OORef<FileImporter>> FileImporter::autodetectFileFormat(DataSet* dataset, ObjectInitializationHints initializationHints, const QUrl& url, OORef<FileImporter> existingImporterHint)
{
	if(!url.isValid())
		dataset->throwException(tr("Invalid path or URL."));

	// Resolve filename if it contains a wildcard.
	return FileSourceImporter::findWildcardMatches(url, dataset).then(dataset->executor(), [dataset, initializationHints, existingImporterHint = std::move(existingImporterHint)](std::vector<QUrl>&& urls) {
		if(urls.empty())
			dataset->throwException(tr("There are no files in the directory matching the filename pattern."));

		// Download file so we can determine its format.
		return Application::instance()->fileManager().fetchUrl(urls.front())
			.then(dataset->executor(), [dataset, initializationHints, url = urls.front(), existingImporterHint = std::move(existingImporterHint)](const FileHandle& file) {
				// Detect file format.
				return autodetectFileFormat(dataset, initializationHints, file, std::move(existingImporterHint));
			});
	});
}

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, ObjectInitializationHints initializationHints, const FileHandle& file, FileImporter* existingImporterHint)
{
	OVITO_ASSERT(dataset->undoStack().isRecordingThread() == false);

	// This is a cache for the file formats of files that have been loaded before.
	static std::map<QString, const FileImporterClass*> formatDetectionCache;

	// Mutex for synchronized access to the format detection cache.
	static QMutex formatDetectionCacheMutex;

	// Check the format cache if we have already detected the format of the same file before.
	const QString& fileIdentifier = file.localFilePath();
	QMutexLocker locker(&formatDetectionCacheMutex);
	if(auto entry = formatDetectionCache.find(fileIdentifier); entry != formatDetectionCache.end()) {
		if(existingImporterHint && &existingImporterHint->getOOClass() == entry->second)
			return existingImporterHint;
		return static_object_cast<FileImporter>(entry->second->createInstance(dataset, initializationHints));
	}
	locker.unlock();

	// If caller has provided an existing importer, check it first against the file.
	if(existingImporterHint) {
		try {
			if(existingImporterHint->getOOMetaClass().checkFileFormat(file)) {
				// Insert detected format into cache to speed up future requests for the same file.
				locker.relock();
				formatDetectionCache.emplace(fileIdentifier, &existingImporterHint->getOOMetaClass());
				return existingImporterHint;
			}
		}
		catch(const Exception&) {
			// Ignore errors that occur during file format detection.
		}		
	}

	// Test all installed importer types.
	for(const FileImporterClass* importerClass : PluginManager::instance().metaclassMembers<FileImporter>()) {
		try {
			if(importerClass->checkFileFormat(file)) {
				// Insert detected format into cache to speed up future requests for the same file.
				locker.relock();
				formatDetectionCache.emplace(fileIdentifier, importerClass);

				// Instantiate the file importer for this file format.
				return static_object_cast<FileImporter>(importerClass->createInstance(dataset, initializationHints));
			}
		}
		catch(const Exception&) {
			// Ignore errors that occur during file format detection.
		}
	}
	return nullptr;
}

/******************************************************************************
* Helper function that is called by sub-classes prior to file parsing in order to
* activate the default "C" locale.
******************************************************************************/
void FileImporter::activateCLocale()
{
	// The setlocale() function is not thread-safe and should only be called from the main thread.
	if(QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread())
		std::setlocale(LC_ALL, "C");
}

/******************************************************************************
* Utility method which splits a string at whitespace separators into tokens.
******************************************************************************/
QStringList FileImporter::splitString(const QString& str)
{
	static const QRegularExpression ws_re(QStringLiteral("\\s+"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	return str.split(ws_re, Qt::SkipEmptyParts);
#else
	return str.split(ws_re, QString::SkipEmptyParts);
#endif
}

}	// End of namespace
