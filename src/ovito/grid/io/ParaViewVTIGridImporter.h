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

#include <QXmlStreamReader>

namespace Ovito::Grid {

/**
 * \brief File parser for reading a VoxelGrid from a ParaView VTI (ImageData) file.
 */
class OVITO_GRID_EXPORT ParaViewVTIGridImporter : public FileSourceImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public FileSourceImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using FileSourceImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.vti"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("ParaView VTI ImageData File"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(ParaViewVTIGridImporter, OOMetaClass)

public:

	/// \brief Constructor.
	Q_INVOKABLE ParaViewVTIGridImporter(DataSet *dataset) : FileSourceImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("VTI"); }

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

	private:

		/// Creates the right kind of OVITO property object that will receive the data read from a <DataArray> element.
		PropertyObject* createGridPropertyForDataArray(VoxelGrid* gridObj, QXmlStreamReader& xml, int& vectorComponent);
	};
};

}	// End of namespace
