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

#pragma once


#include <ovito/mesh/Mesh.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>

#include <QXmlStreamReader>

namespace Ovito { namespace Mesh {

/**
 * \brief File parser for ParaView Multi-Block files (VTM).
 */
class OVITO_MESHMOD_EXPORT ParaViewVTMImporter : public FileSourceImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public FileSourceImporter::OOMetaClass
	{
	public:
	
		/// Inherit standard constructor from base meta class.
		using FileSourceImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.vtm"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("ParaView Multi-Block VTM File"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(ParaViewVTMImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE ParaViewVTMImporter(DataSet *dataset) : FileSourceImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("VTM"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const FileHandle& file) override {
		return std::make_shared<FrameLoader>(frame, file);
	}

	/// Loads the data for the given frame from the external file.
	virtual Future<FrameDataPtr> loadFrame(const Frame& frame, const FileHandle& file) override;

private:

	class MultiBlockFrameData : public FileSourceImporter::FrameData
	{
	public:

		/// Inserts the loaded loaded into the provided pipeline state structure. This function is
		/// called by the system from the main thread after the asynchronous loading task has finished.
		virtual OORef<DataCollection> handOver(const DataCollection* existing, bool isNewFile, CloneHelper& cloneHelper, FileSource* fileSource, const QString& identifierPrefix = {}) override;

		/// Adds an URL to the list of URLs that are part of the multi-block dataset.
		void addUrl(QUrl&& url, QStringRef blockName) { 
			_urls.push_back(std::move(url)); 
			_blockNames.push_back(blockName.toString());
		}

		/// Returns the list of URLs referenced by the VTM file.
		const std::vector<QUrl>& urls() const { return _urls; }

		/// Removes and returns next next URL from the list of URLs referenced by the VTM file.
		bool takeUrl(QUrl& url, QString& blockName) {
			if(_urls.empty()) return false;
			url = std::move(_urls.back());
			_urls.pop_back();
			blockName = std::move(_blockNames.back());
			_blockNames.pop_back();
			return true;
		}

		/// Adds a dataset to the multi-block dataset.
		void addBlockData(FrameDataPtr blockData, const QString& name) {
			OVITO_ASSERT(blockData);
			_blockData.insert(_blockData.begin(), std::move(blockData));
			_loadedBlockNames.insert(_loadedBlockNames.begin(), name);
		}

	private:

		/// The list of URLs referenced by the VTM file.
		std::vector<QUrl> _urls;

		/// The names of the child blocks.
		std::vector<QString> _blockNames;

		/// The loaded data of the child blocks.
		std::vector<FrameDataPtr> _blockData;

		/// The names of the loaded child blocks.
		std::vector<QString> _loadedBlockNames;
	};

	/// The format-specific task object that is responsible for reading an input file in a separate thread.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const FileSourceImporter::Frame& frame, const FileHandle& file)
			: FileSourceImporter::FrameLoader(frame, file) {}

	protected:

		/// Reads the frame data from the external file.
		virtual FrameDataPtr loadFile() override;
	};

	/// Helper method that implements asynchronous loading of datasets referenced by the VTM file.
	Future<FrameDataPtr> loadNextDataset(FrameDataPtr frameData);
};

}	// End of namespace
}	// End of namespace
