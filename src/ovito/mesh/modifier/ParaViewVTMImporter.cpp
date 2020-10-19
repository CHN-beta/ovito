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
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr ParaViewVTMImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTM file %1").arg(fileHandle().toString()));

	// Create the storage container for the parsed data.
	std::shared_ptr<MultiBlockFrameData> frameData = std::make_shared<MultiBlockFrameData>();

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTM file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return {};

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
				frameData->addUrl(fileHandle().sourceUrl().resolved(QUrl(file)), xml.attributes().value("name"));
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

	return std::move(frameData);
}

/******************************************************************************
* Loads the data for the given frame from the external file.
******************************************************************************/
Future<FileSourceImporter::FrameDataPtr> ParaViewVTMImporter::loadFrame(const Frame& frame, const FileHandle& file)
{
	// First step: Load the VTM file, which contains the list of referenced dataset files.
	Future<FrameDataPtr> future = FileSourceImporter::loadFrame(frame, file);

	// Second step: Load the individual dataset files referenced in the VTM file.
	return std::move(future).then(executor(), [this](FrameDataPtr&& frameData) {
		return loadNextDataset(std::move(frameData));
	});
}

/******************************************************************************
* Helper method that implements asynchronous loading of datasets referenced by the VTM file.
******************************************************************************/
Future<FileSourceImporter::FrameDataPtr> ParaViewVTMImporter::loadNextDataset(FrameDataPtr frameData)
{
	MultiBlockFrameData& data = static_cast<MultiBlockFrameData&>(*frameData.get());

	// Take next file referenced by the VTM file.
	QUrl url;
	QString blockName;
	if(!data.takeUrl(url, blockName))
		return Future<FrameDataPtr>::createImmediate(std::move(frameData));

	// Third step: Retrieve the data file from the given URL.
	return Application::instance()->fileManager()->fetchUrl(dataset()->taskManager(), url).then(executor(), [this, frameData = std::move(frameData), blockName](const FileHandle& file) mutable {

		// Fourth step: Detect file's format and create an importer for it.
		OORef<FileImporter> importer = FileImporter::autodetectFileFormat(dataset(), file);

		// This currently works only for FileSourceImporters.
		// Files with other kinds of importers will be skipped.
		OORef<FileSourceImporter> fsImporter = dynamic_object_cast<FileSourceImporter>(std::move(importer));
		if(!fsImporter)
			return loadNextDataset(std::move(frameData));

		// Fifth step: Load the file.
		return fsImporter->loadFrame(Frame(file), file).then_future(executor(), [this, frameData = std::move(frameData), fsImporter = std::move(fsImporter), filename = file.sourceUrl().fileName(), blockName](Future<FrameDataPtr> blockDataFuture) mutable {
			MultiBlockFrameData& data = static_cast<MultiBlockFrameData&>(*frameData.get());
			try {
				// Add the loaded dataset to the multi-block container.
				if(FrameDataPtr blockData = blockDataFuture.result()) {
					data.addBlockData(std::move(blockData), blockName);
				}

				// Continue with loading the next block referenced by the VTM file. 
				return loadNextDataset(std::move(frameData));
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("Failed to load VTK multi-block dataset index %1: %2").arg(data.urls().size() + 1).arg(filename));
				throw ex;
			}
		});
	});
}

/******************************************************************************
* Inserts the loaded data into the provided container object.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
OORef<DataCollection> ParaViewVTMImporter::MultiBlockFrameData::handOver(const DataCollection* existing, bool isNewFile, CloneHelper& cloneHelper, FileSource* fileSource, const QString& identifierPrefix)
{
	// Create a CombineDatasetsModifier, which will take care of merging the blocks of the VTK multi-block dataset.
	OORef<CombineDatasetsModifier> combineMod = new CombineDatasetsModifier(fileSource->dataset());
	// Create an ad-hoc modifier application.
	OORef<ModifierApplication> modApp = combineMod->createModifierApplication();

	// Create an empty data collection to start with.
	OORef<DataCollection> output = new DataCollection(fileSource->dataset());
	PipelineFlowState state(output, PipelineStatus::Success);
	OVITO_ASSERT(state.data() == output);

	// Iterate over the loaded data blocks and merge all of them into a single data collection.
	auto blockName = _loadedBlockNames.cbegin();
	for(const FrameDataPtr& blockData : _blockData) {

		// Obtain the pipeline state of the block data importer.
		OORef<DataCollection> blockCollection = blockData->handOver(existing, isNewFile, cloneHelper, fileSource, *blockName++);
		PipelineFlowState secondaryState(blockCollection, PipelineStatus::Success);
		OVITO_ASSERT(secondaryState.data() == blockCollection);

		// Perform the merging of two data collections.
		combineMod->combineDatasets(fileSource->dataset()->animationSettings()->time(), modApp, state, secondaryState);
	}

	return output;
}

}	// End of namespace
}	// End of namespace
