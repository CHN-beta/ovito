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
#include <ovito/stdmod/modifiers/CombineDatasetsModifier.h>
#include <ovito/core/dataset/data/DataCollection.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/WhenAll.h>
#include <ovito/core/app/Application.h>
#include "ParaViewVTMImporter.h"

namespace Ovito { namespace Mesh {

using namespace Ovito::StdMod; 

IMPLEMENT_OVITO_CLASS(ParaViewVTMImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParaViewVTMImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = file.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QXmlStreamReader xml(device.get());

	// Parse XML. First element must be <VTKFile type="vtkMultiBlockDataSet">.
	if(xml.readNext() != QXmlStreamReader::StartDocument)
		return false;
	if(xml.readNext() != QXmlStreamReader::StartElement)
		return false;
	if(xml.name() != "VTKFile")
		return false;
	if(xml.attributes().value("type") != "vtkMultiBlockDataSet")
		return false;

	return !xml.hasError();
}

/******************************************************************************
* Parses the given VTM file and returns the list of referenced data files.
******************************************************************************/
std::vector<std::pair<QUrl, QString>> ParaViewVTMImporter::loadVTMFile(const FileHandle& fileHandle)
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTM file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// Parse the elements of the XML file.
	std::vector<std::pair<QUrl, QString>> blocks;
	while(xml.readNextStartElement()) {
		if(xml.name() == "VTKFile") {
			if(xml.attributes().value("type") != "vtkMultiBlockDataSet")
				xml.raiseError(tr("VTM file is not of type vtkMultiBlockDataSet."));
		}
		else if(xml.name() == "vtkMultiBlockDataSet") {
			// Do nothing. Parse child elements.
		}
		else if(xml.name() == "DataSet") {

			// Get value of 'file' attribute.
			QString file = xml.attributes().value("file").toString();
			if(!file.isEmpty()) {
				// Resolve file path and record URL, which will be loaded later.
				blocks.emplace_back(fileHandle.sourceUrl().resolved(QUrl(file)), xml.attributes().value("name").toString());
			}

			xml.skipCurrentElement();
		}
		else {
			xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("VTM file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}

	return blocks;
}

/******************************************************************************
* Loads the data for the given frame from the external file.
******************************************************************************/
Future<PipelineFlowState> ParaViewVTMImporter::loadFrame(const Frame& frame, const FileHandle& file, const DataCollection* masterCollection, PipelineObject* dataSource)
{
	ExecutionContext executionContext = Application::instance()->executionContext();

	// Load the VTM file, which contains the list of referenced data files.
	std::vector<std::pair<QUrl, QString>> blocks = loadVTMFile(file);

	// Load the individual data files referenced in the VTM file.
	std::vector<Future<PipelineFlowState>> blockFutures;
	for(const auto& blockInfo : blocks) {
		blockFutures.push_back(loadDataBlock(blockInfo.first, blockInfo.second, executionContext, masterCollection, dataSource));
	}

	// Wait for the load operations to finish.
	return when_all(std::move(blockFutures)).then(executor(), [this, executionContext](std::vector<PipelineFlowState> blockDataCollections) {
		// Third step: Merge the block datasets into a single output data collection.

		// Create a CombineDatasetsModifier, which will take care of merging the blocks of the VTK multi-block dataset.
		OORef<CombineDatasetsModifier> combineMod = OORef<CombineDatasetsModifier>::create(dataset(), executionContext);
		// Create an ad-hoc modifier application.
		OORef<ModifierApplication> modApp = combineMod->createModifierApplication();

		// Create an empty data collection to start with.
		PipelineFlowState result(DataOORef<DataCollection>::create(dataset(), executionContext), PipelineStatus::Success);

		// Iterate over the loaded data blocks and incrementally merge all of them into a single data collection.
		qDebug() << "Merging data collections:" << blockDataCollections.size();
		for(const PipelineFlowState& blockData : blockDataCollections) {
			// Skip empty data blocks.
			if(!blockData) continue;

			// Perform the merging of two data collections.
			qDebug() << blockData.data()->objects().size();
			for(const DataObject* obj  : blockData.data()->objects())
				qDebug() << "  " << obj << "id:" << obj->identifier(); 
			combineMod->combineDatasets(0, modApp, result, blockData);
		}
		qDebug() << "Resulting data collections:" << result.data()->objects().size();
		for(const DataObject* obj  : result.data()->objects())
			qDebug() << "  " << obj << "id:" << obj->identifier(); 

		return result;
	});
}

/******************************************************************************
* Helper method that implements asynchronous loading of datasets referenced by the VTM file.
******************************************************************************/
Future<PipelineFlowState> ParaViewVTMImporter::loadDataBlock(const QUrl& url, const QString& blockName, ExecutionContext executionContext, const DataCollection* masterCollection, PipelineObject* dataSource)
{
	// Retrieve the data file from the given URL.
	return Application::instance()->fileManager()->fetchUrl(dataset()->taskManager(), url).then(executor(), [this, blockName, executionContext, masterCollection = DataOORef<const DataCollection>(masterCollection), dataSource = QPointer<PipelineObject>(dataSource)](const FileHandle& file) {

		// Detect file format and create an importer for it.
		OORef<FileImporter> importer = FileImporter::autodetectFileFormat(dataset(), executionContext, file);

		// This currently works only for FileSourceImporters.
		// Files formats handled by other kinds of importers will be skipped.
		OORef<FileSourceImporter> fsImporter = dynamic_object_cast<FileSourceImporter>(std::move(importer));
		if(!fsImporter)
			return Future<PipelineFlowState>::createImmediateEmplace();

		// Load the file.
		return fsImporter->loadFrame(Frame(file), file, masterCollection, dataSource).then_future(executor(), [this, fsImporter = std::move(fsImporter), filename = file.sourceUrl().fileName(), blockName](Future<PipelineFlowState> blockDataFuture) {
			try {
				return blockDataFuture.result();
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("Failed to load VTK multi-block dataset %1: %2").arg(blockName).arg(filename));
				throw ex;
			}
		});
	});
}

}	// End of namespace
}	// End of namespace
