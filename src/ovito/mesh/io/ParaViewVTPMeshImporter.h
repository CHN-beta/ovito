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
#include <ovito/stdobj/io/StandardFrameLoader.h>
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
		virtual QString fileFilterDescription() const override { return tr("ParaView VTP PolyData Mesh Files"); }

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
	virtual FileSourceImporter::FrameLoaderPtr createFrameLoader(const LoadOperationRequest& request) override {
		return std::make_shared<FrameLoader>(request);
	}

	/// Reads a <DataArray> element from a VTK file and stores it in the given OVITO data buffer.
	static void parseVTKDataArray(DataBuffer* buffer, size_t beginIndex, size_t endIndex, int vectorComponent, QXmlStreamReader& xml, const std::function<size_t(size_t)>& indexMapping = {});

	/// Reads a <DataArray> element from a VTK file and stores it in the given OVITO data buffer.
	static void parseVTKDataArray(DataBuffer* buffer, int vectorComponent, QXmlStreamReader& xml) {
		parseVTKDataArray(buffer, 0, buffer->size(), vectorComponent, xml);
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

		/// Reads a <DataArray> element and returns it as an OVITO property.
		PropertyPtr parseDataArray(QXmlStreamReader& xml, int convertToDataType = 0);
	};
};

}	// End of namespace
}	// End of namespace
