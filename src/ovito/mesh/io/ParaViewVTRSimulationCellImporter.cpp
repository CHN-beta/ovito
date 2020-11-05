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

#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/simcell/SimulationCellVis.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "ParaViewVTRSimulationCellImporter.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(ParaViewVTRSimulationCellImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParaViewVTRSimulationCellImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = file.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QXmlStreamReader xml(device.get());

	// Parse XML. First element must be <VTKFile type="RectilinearGrid">.
	if(xml.readNext() != QXmlStreamReader::StartDocument)
		return false;
	if(xml.readNext() != QXmlStreamReader::StartElement)
		return false;
	if(xml.name() != "VTKFile")
		return false;
	if(xml.attributes().value("type") != "RectilinearGrid")
		return false;

	return !xml.hasError();
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr ParaViewVTRSimulationCellImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTR RectilinearGrid file %1").arg(fileHandle().toString()));

	// Create the container for the data to be loaded.
	std::shared_ptr<RectilinearGridFrameData> frameData = std::make_shared<RectilinearGridFrameData>();

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTR file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// The simulation cell matrix.
	AffineTransformation cellMatrix = AffineTransformation::Zero();

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return {};

		if(xml.name() == "VTKFile") {
			if(xml.attributes().value("type") != "RectilinearGrid")
				xml.raiseError(tr("VTK file is not of type RectilinearGrid."));
			else if(xml.attributes().value("byte_order") != "LittleEndian")
				xml.raiseError(tr("Byte order must be 'LittleEndian'. Please contact the OVITO developers to request an extension of the file parser."));
		}
		else if(xml.name() == "RectilinearGrid") {
			// Do nothing. Parse child elements.
		}
		else if(xml.name() == "Piece") {
			// Do nothing. Parse child elements.
		}
		else if(xml.name() == "Coordinates") {
			// Parse three <DataArray> elements.
			for(size_t dim = 0; dim < 3; dim++) {
				if(!xml.readNextStartElement())
					break;
				if(xml.name() == "DataArray") {
					FloatType rangeMin = xml.attributes().value("RangeMin").toDouble();
					FloatType rangeMax = xml.attributes().value("RangeMax").toDouble();
					cellMatrix(dim, dim) = rangeMax - rangeMin;
					cellMatrix(dim, 3) = rangeMin;
					xml.skipCurrentElement();
				}
				else {
					xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
				}
			}
			break;
		}
		else if(xml.name() == "PointData" || xml.name() == "CellData" || xml.name() == "DataArray") {
			// Do nothing. Ignore element contents.
			xml.skipCurrentElement();
		}
		else {
			xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("VTR file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}

	frameData->simulationCell().setMatrix(cellMatrix);

	return std::move(frameData);
}

/******************************************************************************
* Inserts the loaded data into the provided container object.
******************************************************************************/
OORef<DataCollection> ParaViewVTRSimulationCellImporter::RectilinearGridFrameData::handOver(const DataCollection* existing, bool isNewFile, CloneHelper& cloneHelper, FileSource* fileSource, const QString& identifierPrefix)
{
	// Start with a fresh data collection that will be populated.
	OORef<DataCollection> output = new DataCollection(fileSource->dataset());

	// Create the simulation cell.
	const SimulationCellObject* existingCell = existing ? existing->getObject<SimulationCellObject>() : nullptr;
	if(!existingCell) {
		// Create a new SimulationCellObject.
		SimulationCellObject* cell = output->createObject<SimulationCellObject>(fileSource, simulationCell());

		// Initialize the simulation cell and its vis element with default values.
		cell->loadUserDefaults(Application::instance()->executionContext());

		// Set up the vis element for the simulation cell.
		if(SimulationCellVis* cellVis = dynamic_object_cast<SimulationCellVis>(cell->visElement())) {

			// Choose an appropriate line width depending on the cell's size.
			FloatType cellDiameter = (
					simulationCell().matrix().column(0) +
					simulationCell().matrix().column(1) +
					simulationCell().matrix().column(2)).length();
			cellVis->setDefaultCellLineWidth(std::max(cellDiameter * FloatType(1.4e-3), FloatType(1e-8)));
			cellVis->setCellLineWidth(cellVis->defaultCellLineWidth());
		}
	}
	else {
		// Adopt pbc flags from input file only if it is a new file.
		// This gives the user the option to change the pbc flags without them
		// being overwritten when a new frame from a simulation sequence is loaded.
		SimulationCellObject* cell = cloneHelper.cloneObject(existingCell, false); 
		output->addObject(cell);
		cell->setData(simulationCell(), isNewFile);
	}

	return output;
}

}	// End of namespace
}	// End of namespace
