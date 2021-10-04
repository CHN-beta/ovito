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


#include <ovito/mesh/Mesh.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>

#include <QXmlStreamReader>

namespace Ovito { namespace Mesh {

/**
 * \brief Describes a single data file referenced by a VTM file. 
 */
struct OVITO_MESH_EXPORT ParaViewVTMBlockInfo
{
	/// The path to the block in the hierarchy of nested data blocks within the VTM file.
	QStringList blockPath;

	/// The URL of the referenced data file.
	QUrl location;

	/// The index of this partial dataset which is part of a multi-block structure.
	int multiBlockIndex = 0;

	/// The total number of partial datasets.
	int multiBlockCount = 1;
};

/**
 * \brief Abstract base class for filters that can customize the loading of VTM files. 
 */
class OVITO_MESH_EXPORT ParaViewVTMFileFilter : public OvitoObject
{
	Q_OBJECT
	OVITO_CLASS(ParaViewVTMFileFilter)

public:

	/// \brief Is called once before the datasets referenced in a multi-block VTM file will be loaded.
	virtual void preprocessDatasets(std::vector<ParaViewVTMBlockInfo>& blockDatasets) {}

	/// \brief Is called for every dataset referenced in a multi-block VTM file.
	virtual Future<> loadDataset(const ParaViewVTMBlockInfo& blockInfo, const FileHandle& referencedFile, const FileSourceImporter::LoadOperationRequest& loadRequest) { return {}; }

	/// \brief Is called before parsing of a dataset referenced in a multi-block VTM file begins.
	virtual void configureImporter(const ParaViewVTMBlockInfo& blockInfo, FileSourceImporter::LoadOperationRequest& loadRequest, FileSourceImporter* importer) {}

	/// \brief Is called after all datasets referenced in a multi-block VTM file have been loaded.
	virtual void postprocessDatasets(FileSourceImporter::LoadOperationRequest& request) {}
};

/**
 * \brief File parser for ParaView Multi-Block files (VTM).
 */
class OVITO_MESH_EXPORT ParaViewVTMImporter : public FileSourceImporter
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
		virtual QString fileFilterDescription() const override { return tr("ParaView Multi-Block VTM Files"); }

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

	/// Loads the data for the given frame from the external file.
	virtual Future<PipelineFlowState> loadFrame(const LoadOperationRequest& request) override;

private:

	/// Parses the given VTM file and returns the list of referenced data files.
	static std::vector<ParaViewVTMBlockInfo> loadVTMFile(const FileHandle& fileHandle);
};

}	// End of namespace
}	// End of namespace
