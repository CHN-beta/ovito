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


#include <ovito/grid/Grid.h>
#include <ovito/stdobj/io/StandardFrameLoader.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>
#include <ovito/mesh/io/ParaViewVTMImporter.h>

#include <QXmlStreamReader>

namespace Ovito::Grid {

/**
 * \brief File parser for reading a VoxelGrid from a ParaView VTS (StructuredGrid) file.
 */
class OVITO_GRID_EXPORT ParaViewVTSGridImporter : public FileSourceImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public FileSourceImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using FileSourceImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.vts"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("ParaView VTS StructuredGrid File"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(ParaViewVTSGridImporter, OOMetaClass)

public:

	/// \brief Constructor.
	Q_INVOKABLE ParaViewVTSGridImporter(ObjectCreationParams params) : FileSourceImporter(params) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("VTS"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		return std::make_shared<FrameLoader>(request);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in a separate thread.
	class FrameLoader : public StandardFrameLoader
	{
	public:

		/// Inherit constructor from base class.
		using StandardFrameLoader::StandardFrameLoader;

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;
	};
};

/**
 * \brief Plugin filter used to customize the loading of VTM files referencing a ParaView VTS file.
 *        This filter is needed to correctly load VTM/VTS file combinations written by the Aspherix simulation code.
 */
class OVITO_GRID_EXPORT GridParaViewVTMFileFilter : public ParaViewVTMFileFilter
{
	OVITO_CLASS(GridParaViewVTMFileFilter)

public:

	/// Constructor.
	Q_INVOKABLE GridParaViewVTMFileFilter() = default;

	/// \brief Is called once before the datasets referenced in a multi-block VTM file will be loaded.
	virtual void preprocessDatasets(std::vector<ParaViewVTMBlockInfo>& blockDatasets, FileSourceImporter::LoadOperationRequest& request, const ParaViewVTMImporter& vtmImporter) override;
};

}	// End of namespace
