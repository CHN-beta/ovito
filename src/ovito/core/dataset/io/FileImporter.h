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


#include <ovito/core/Core.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/DataSet.h>

namespace Ovito {

/**
 * \brief A meta-class for file importers (i.e. classes derived from FileImporter).
 */
class OVITO_CORE_EXPORT FileImporterClass : public RefTarget::OOMetaClass
{
public:

	/// Inherit standard constructor from base meta class.
	using RefTarget::OOMetaClass::OOMetaClass;

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	/// \return A wild-card pattern that specifies the file types that can be handled by this import class.
	virtual QString fileFilter() const {
		OVITO_ASSERT_MSG(false, "FileImporterClass::fileFilter()", "This method should be overridden by a meta-subclass of FileImporterClass.");
		return {};
	}

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	/// \return A string that describes the file format.
	virtual QString fileFilterDescription() const {
		OVITO_ASSERT_MSG(false, "FileImporterClass::fileFilterDescription()", "This method should be overridden by a meta-subclass of FileImporterClass.");
		return {};
	}

	/// \brief Checks if the given file has format that can be read by this importer.
	/// \param input The file that contains the data to check.
	/// \return \c true if the data can be parsed.
	//	        \c false if the data has some unknown format.
	/// \throw Exception when the check has failed.
	virtual bool checkFileFormat(const FileHandle& input) const {
		return false;
	}

	/// \brief Returns whether this importer class supports importing data of the given type.
	/// \param dataObjectType A DataObject-derived class.
	/// \return \c true if this importer can import data object the given type.
	virtual bool supportsDataType(const DataObject::OOMetaClass& dataObjectType) const {
		return false;
	}
};

/**
 * \brief Abstract base class for file import services.
 */
class OVITO_CORE_EXPORT FileImporter : public RefTarget
{
	OVITO_CLASS_META(FileImporter, FileImporterClass)

protected:

	/// \brief The constructor.
	using RefTarget::RefTarget;

public:

	/// Import modes that control the behavior of the importFileSet() method.
	enum ImportMode {
		AddToScene,				///< Add the imported data as a new object to the scene.
		ReplaceSelected,		///< Replace existing input data with newly imported data if possible. Add to scene otherwise.
								///  In any case, keep all other objects in the scene as they are.
		ResetScene,				///< Clear the contents of the current scene first before importing the data.
		DontAddToScene			///< Do not add the imported data to the scene.
	};
	Q_ENUM(ImportMode);

	/// \brief Asks the importer if the option to replace the currently selected object
	///        with the new file(s) is available.
	virtual bool isReplaceExistingPossible(const std::vector<QUrl>& sourceUrls) { return false; }

	/// \brief Returns the priority level of this importer, which is used to order multiple files that are imported simultaneously.
	virtual int importerPriority() const { return 0; }

	/// \brief Imports one or more files into the scene.
	/// \param sourceUrlsAndImporters The location of the file(s) to import and the corresponding importers.
	/// \param importMode Controls how the imported data is inserted into the scene.
	/// \param autodetectFileSequences Enables the automatic detection of file sequences.
	/// \return \c The new pipeline if the file has been successfully imported.
	//	        \c nullptr if the operation has been canceled by the user.
	/// \throw Exception when the import operation has failed.
	virtual OORef<PipelineSceneNode> importFileSet(std::vector<std::pair<QUrl, OORef<FileImporter>>> sourceUrlsAndImporters, ImportMode importMode, bool autodetectFileSequences) = 0;

	/// \brief Tries to detect the format of the given file.
	/// \param existingImporterHint Optional existing importer object, which is tested first agains the file. Providing this importer can speed up the auto-detection.
	/// \return The importer class that can handle the given file. If the file format could not be recognized then NULL is returned.
	/// \throw Exception if url is invalid or if operation has been canceled by the user.
	/// \note This is a blocking function, which downloads the file and can take a long time to return.
	static Future<OORef<FileImporter>> autodetectFileFormat(DataSet* dataset, const QUrl& url, OORef<FileImporter> existingImporterHint = {});

	/// \brief Tries to detect the format of the given file.
	/// \param existingImporterHint Optional existing importer object, which is tested first agains the file. Providing this importer can speed up the auto-detection.
	/// \return The importer class that can handle the given file. If the file format could not be recognized then NULL is returned.
	static OORef<FileImporter> autodetectFileFormat(DataSet* dataset, const FileHandle& file, FileImporter* existingImporterHint = nullptr);

	/// Helper function that is called by sub-classes prior to file parsing in order to
	/// activate the default "C" locale.
	static void activateCLocale();

	/// Utility method which splits a string at whitespace separators into tokens.
	static QStringList splitString(const QString& str);

	/// Utility method which splits a string at whitespace separators into tokens.
	static QStringList splitString(QStringView str) { return splitString(str.toString()); }
};

}	// End of namespace
