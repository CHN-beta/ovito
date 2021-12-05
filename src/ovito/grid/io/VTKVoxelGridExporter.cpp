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

#include <ovito/grid/Grid.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/app/Application.h>
#include "VTKVoxelGridExporter.h"

namespace Ovito::Grid {


/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool VTKVoxelGridExporter::openOutputFile(const QString& filePath, int numberOfFrames, SynchronousOperation operation)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void VTKVoxelGridExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool VTKVoxelGridExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, SynchronousOperation operation)
{
	// Evaluate pipeline.
	const PipelineFlowState& state = getPipelineDataToBeExported(time, operation.subOperation());
	if(operation.isCanceled())
		return false;

	// Look up the VoxelGrid to be exported in the pipeline state.
	DataObjectReference objectRef(&VoxelGrid::OOClass(), dataObjectToExport().dataPath());
	const VoxelGrid* voxelGrid = static_object_cast<VoxelGrid>(state.getLeafObject(objectRef));
	if(!voxelGrid) {
		throwException(tr("The pipeline output does not contain the voxel grid to be exported (animation frame: %1; object key: %2). Available grid keys: (%3)")
			.arg(frameNumber).arg(objectRef.dataPath()).arg(getAvailableDataObjectList(state, VoxelGrid::OOClass())));
	}

	// Make sure the data structure to be exported is consistent.
	voxelGrid->verifyIntegrity();

	operation.setProgressText(tr("Writing file %1").arg(filePath));

	auto dims = voxelGrid->shape();
	textStream() << "# vtk DataFile Version 3.0\n";
	textStream() << "# Voxel grid data written by " << Application::applicationName() << " " << Application::applicationVersionString() << "\n";
	textStream() << "ASCII\n";
	textStream() << "DATASET STRUCTURED_POINTS\n";
	textStream() << "DIMENSIONS " << dims[0] << " " << dims[1] << " " << dims[2] << "\n";
	if(const SimulationCellObject* domain = voxelGrid->domain()) {
		textStream() << "ORIGIN " << domain->cellOrigin().x() << " " << domain->cellOrigin().y() << " " << domain->cellOrigin().z() << "\n";
		textStream() << "SPACING";
		textStream() << " " << domain->cellVector1().length() / std::max(dims[0], (size_t)1);
		textStream() << " " << domain->cellVector2().length() / std::max(dims[1], (size_t)1);
		textStream() << " " << domain->cellVector3().length() / std::max(dims[2], (size_t)1);
		textStream() << "\n";
	}
	else {
		textStream() << "ORIGIN 0 0 0\n";
		textStream() << "SPACING 1 1 1\n";
	}
	textStream() << "POINT_DATA " << voxelGrid->elementCount() << "\n";

	for(const PropertyObject* prop : voxelGrid->properties()) {
		if(prop->dataType() == PropertyObject::Int || prop->dataType() == PropertyObject::Int64 || prop->dataType() == PropertyObject::Float) {

			// Write header of data field.
			QString dataName = prop->name();
			dataName.remove(QChar(' '));
			if(prop->dataType() == PropertyObject::Float && prop->componentCount() == 3) {
				textStream() << "\nVECTORS " << dataName << " double\n";
			}
			else if(prop->componentCount() <= 4) {
				if(prop->dataType() == PropertyObject::Int)
					textStream() << "\nSCALARS " << dataName << " int " << prop->componentCount() << "\n";
				else if(prop->dataType() == PropertyObject::Int64)
					textStream() << "\nSCALARS " << dataName << " long " << prop->componentCount() << "\n";
				else
					textStream() << "\nSCALARS " << dataName << " double " << prop->componentCount() << "\n";
				textStream() << "LOOKUP_TABLE default\n";
			}
			else continue; // VTK format supports only between 1 and 4 vector components. Skipping properties with more components during export.

			// Write payload data.
			size_t cmpnts = prop->componentCount();
			OVITO_ASSERT(prop->stride() == prop->dataTypeSize() * cmpnts);
			if(prop->dataType() == PropertyObject::Float) {
				ConstPropertyAccess<FloatType, true> data(prop);
				for(size_t row = 0, index = 0; row < dims[1]*dims[2]; row++) {
					if(operation.isCanceled())
						return false;
					for(size_t col = 0; col < dims[0]; col++, index++) {
						for(size_t c = 0; c < cmpnts; c++)
							textStream() << data.get(index, c) << " ";
					}
					textStream() << "\n";
				}
			}
			else if(prop->dataType() == PropertyObject::Int) {
				ConstPropertyAccess<int, true> data(prop);
				for(size_t row = 0, index = 0; row < dims[1]*dims[2]; row++) {
					if(operation.isCanceled())
						return false;
					for(size_t col = 0; col < dims[0]; col++, index++) {
						for(size_t c = 0; c < cmpnts; c++)
							textStream() << data.get(index, c) << " ";
					}
					textStream() << "\n";
				}
			}				
			else if(prop->dataType() == PropertyObject::Int64) {
				ConstPropertyAccess<qlonglong, true> data(prop);
				for(size_t row = 0, index = 0; row < dims[1]*dims[2]; row++) {
					if(operation.isCanceled())
						return false;
					for(size_t col = 0; col < dims[0]; col++, index++) {
						for(size_t c = 0; c < cmpnts; c++)
							textStream() << data.get(index, c) << " ";
					}
					textStream() << "\n";
				}
			}
			else {
				throwException(tr("Grid property '%1' has a non-standard data type that cannot be exported.").arg(prop->name()));
			}
		}
	}

	return !operation.isCanceled();
}

}	// End of namespace
