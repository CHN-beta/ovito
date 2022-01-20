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
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/core/dataset/io/FileExporter.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>

namespace Ovito::Grid {

/**
 * \brief Exporter that writes the data of a VoxelGrid object to a VTK file.
 */
class OVITO_GRID_EXPORT VTKVoxelGridExporter : public FileExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public FileExporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using FileExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be exported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.vtk"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("VTK Voxel Grid File"); }
	};

	OVITO_CLASS_META(VTKVoxelGridExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE VTKVoxelGridExporter(DataSet* dataset) : FileExporter(dataset) {}

	/// \brief Returns the type(s) of data objects that this exporter service can export.
	virtual std::vector<DataObjectClassPtr> exportableDataObjectClass() const override {
		return { &VoxelGrid::OOClass() };
	}

protected:

	/// \brief This is called once for every output file to be written and before exportData() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, MainThreadOperation& operation) override;

	/// \brief This is called once for every output file written after exportData() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, MainThreadOperation& operation) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

	/// Returns the text stream used to write into the current output file.
	CompressedTextWriter& textStream() { return *_outputStream; }

private:

	/// The output file stream.
	QFile _outputFile;

	/// The stream object used to write into the output file.
	std::unique_ptr<CompressedTextWriter> _outputStream;
};

}	// End of namespace
