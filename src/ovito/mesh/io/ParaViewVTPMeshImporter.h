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
 * \brief File parser for reading a SurfaceMesh from a ParaView VTP (PolyData) file.
 */
class OVITO_MESH_EXPORT ParaViewVTPMeshImporter : public FileSourceImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public FileSourceImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using FileSourceImporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.vtp"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("ParaView VTP PolyData File"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(ParaViewVTPMeshImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE ParaViewVTPMeshImporter(DataSet *dataset) : FileSourceImporter(dataset) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("VTP"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const Frame& frame, const FileHandle& file, const DataCollection* masterCollection, PipelineObject* dataSource) override {
		return std::make_shared<FrameLoader>(frame, file);
	}

private:

	/// The format-specific task object that is responsible for reading an input file in a separate thread.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const FileSourceImporter::Frame& frame, const FileHandle& file)
			: FileSourceImporter::FrameLoader(frame, file) {}

	protected:

		/// Reads the frame data from the external file.
		virtual void loadFile() override;

		/// Reads a <DataArray> element and returns it as an OVITO property.
		PropertyPtr parseDataArray(QXmlStreamReader& xml, int convertToDataType = 0);
	};
};

}	// End of namespace
}	// End of namespace