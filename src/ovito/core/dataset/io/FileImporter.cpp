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
Future<OORef<FileImporter>> FileImporter::autodetectFileFormat(DataSet* dataset, const QUrl& url, OORef<FileImporter> existingImporterHint)
{
	if(!url.isValid())
		dataset->throwException(tr("Invalid path or URL."));

	// Resolve filename if it contains a wildcard.
	return FileSourceImporter::findWildcardMatches(url, dataset).then(dataset->executor(), [dataset, existingImporterHint = std::move(existingImporterHint)](std::vector<QUrl>&& urls) {
		if(urls.empty())
			dataset->throwException(tr("There are no files in the directory matching the filename pattern."));

		// Download file so we can determine its format.
		return Application::instance()->fileManager().fetchUrl(urls.front())
			.then(dataset->executor(), [dataset, url = urls.front(), existingImporterHint = std::move(existingImporterHint)](const FileHandle& file) {
				// Detect file format.
				return autodetectFileFormat(dataset, file, std::move(existingImporterHint));
			});
	});
}

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, const FileHandle& file, FileImporter* existingImporterHint)
{
	// Note: FileImporter::autodetectFileFormat() may only be called from the main thread.
	// Event though the implementation of autodetectFileFormat() itself is thread-safe,
	// FileImporterClass::determineFileFormat() is currently limited to the main thread.
	OVITO_ASSERT(QThread::currentThread() == dataset->thread());
	// Note: FileSourceImporter::loadFrame() may not be called while undo recording is active.
	OVITO_ASSERT(dataset->undoStack().isRecordingThread() == false);

	// Cache for the format of files already loaded during the current program session.
	//
	// Keys:   Local filesystem paths 
	// Values: The importer class handling the file and an optional sub-format specifier.
	static std::map<QString, std::pair<const FileImporterClass*, QString>> formatDetectionCache;

	// Mutex for synchronized access to the format detection cache.
	static QMutex formatDetectionCacheMutex;

	// Check the format cache if we have already detected the format of the same file before.
	const QString& fileIdentifier = file.localFilePath();
	QMutexLocker locker(&formatDetectionCacheMutex);
	if(auto entry = formatDetectionCache.find(fileIdentifier); entry != formatDetectionCache.end()) {
		const FileImporterClass* clazz = entry->second.first;
		const QString& format = entry->second.second;
		// Can we reuse the existing importer instance?
		if(existingImporterHint && &existingImporterHint->getOOClass() == clazz) {
			existingImporterHint->setSelectedFileFormat(format);
			return existingImporterHint;
		}
		else {
			// Create a new importer class instance and configure it.
			OORef<FileImporter> importer = static_object_cast<FileImporter>(clazz->createInstance(dataset));
			importer->setSelectedFileFormat(format);
			return importer;
		}
	}
	locker.unlock();

	// If caller has provided an existing importer, check it first against the file.
	if(existingImporterHint) {
		try {
			if(std::optional<QString> formatIdentifier = existingImporterHint->getOOMetaClass().determineFileFormat(file, dataset)) {
				// Insert detected format into cache to speed up future requests for the same file.
				locker.relock();
				formatDetectionCache.emplace(fileIdentifier, std::make_pair(&existingImporterHint->getOOMetaClass(), *formatIdentifier));
				existingImporterHint->setSelectedFileFormat(*formatIdentifier);
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
			if(std::optional<QString> formatIdentifier = importerClass->determineFileFormat(file, dataset)) {
				// Insert detected format into cache to speed up future requests for the same file.
				locker.relock();
				formatDetectionCache.emplace(fileIdentifier, std::make_pair(importerClass, *formatIdentifier));

				// Instantiate the file importer for this file format.
				OORef<FileImporter> importer = static_object_cast<FileImporter>(importerClass->createInstance(dataset));
				importer->setSelectedFileFormat(*formatIdentifier);
				return importer;
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
