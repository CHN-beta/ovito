////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include <ovito/core/dataset/data/DataCollection.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/ForEach.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include "ParaViewVTMImporter.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(ParaViewVTMImporter);
IMPLEMENT_OVITO_CLASS(ParaViewVTMFileFilter);

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
	if(xml.name().compare(QStringLiteral("VTKFile")) != 0)
		return false;
	if(xml.attributes().value("type").compare(QStringLiteral("vtkMultiBlockDataSet")) != 0)
		return false;

	return !xml.hasError();
}

/******************************************************************************
* Parses the given VTM file and returns the list of referenced data files.
******************************************************************************/
std::vector<std::pair<QStringList, QUrl>> ParaViewVTMImporter::loadVTMFile(const FileHandle& fileHandle)
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTM file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// The list of <DataSet> elements found in the file.
	std::vector<std::pair<QStringList, QUrl>> datasetList;
	// The current branch in the block hierarchy.
	QStringList blockBranch;

	// Parse the elements of the XML file.
	while(!xml.atEnd()) {
		while(xml.readNextStartElement()) {
			if(xml.name().compare(QStringLiteral("VTKFile")) == 0) {
				if(xml.attributes().value("type").compare(QStringLiteral("vtkMultiBlockDataSet")) != 0)
					xml.raiseError(tr("VTM file is not of type vtkMultiBlockDataSet."));
			}
			else if(xml.name().compare(QStringLiteral("vtkMultiBlockDataSet")) == 0) {
				// Do nothing. Parse child elements.
			}
			else if(xml.name().compare(QStringLiteral("Block")) == 0) {

				// Get value of 'name' attribute.
				blockBranch.push_back(xml.attributes().value("name").toString());

				// Continue by parsing child elements.
			}
			else if(xml.name().compare(QStringLiteral("DataSet")) == 0) {

				// Get value of 'file' attribute.
				QString file = xml.attributes().value("file").toString();
				if(!file.isEmpty()) {
					// The current path in the block hierarchy:
					QStringList path = blockBranch;
					path.append(xml.attributes().value("name").toString());

					// Resolve file path and record URL, which will be loaded later.
					datasetList.emplace_back(std::move(path), fileHandle.sourceUrl().resolved(QUrl(file)));
				}

				xml.skipCurrentElement();
			}
			else {
				xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
			}
		}
		if(xml.tokenType() == QXmlStreamReader::EndElement) {
			if(xml.name().compare(QStringLiteral("Block")) == 0) {
				blockBranch.pop_back();
			}
			else if(xml.name().compare(QStringLiteral("VTKFile")) == 0) {
				break;
			}
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("VTM file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}

	return datasetList;
}

/******************************************************************************
* Loads the data for the given frame from the external file.
******************************************************************************/
Future<PipelineFlowState> ParaViewVTMImporter::loadFrame(const LoadOperationRequest& request)
{
	OVITO_ASSERT(dataset()->undoStack().isRecordingThread() == false);

	struct ExtendedLoadRequest : public LoadOperationRequest {
		/// Constructor.
		ExtendedLoadRequest(const LoadOperationRequest& other) : LoadOperationRequest(other) {}
		/// The multi-block hierarchy path of the current dataset being loaded.
		QStringList blockPath;
		/// Plugin filters processing the datasets referenced by the VTM file.
		std::vector<OORef<ParaViewVTMFileFilter>> filters;
	};

	// Resize property containers to zero elements in the existing pipeline state.
	// This is mainly done to remove the existing particles in those animation frames in which the VTM file 
	// has empty data blocks.
	ExtendedLoadRequest modifiedRequest(request);
	for(const DataObject* obj : modifiedRequest.state.data()->objects()) {
		if(const PropertyContainer* container = dynamic_object_cast<PropertyContainer>(obj)) {
			PropertyContainer* mutableContainer = modifiedRequest.state.mutableData()->makeMutable(container);
			mutableContainer->setElementCount(0);
		}
	}

	// Load the VTM file, which contains the list of referenced data files.
	std::vector<std::pair<QStringList, QUrl>> blockDatasets = loadVTMFile(request.fileHandle);

	// Create filter objects.
	static const QVector<OvitoClassPtr> filterClassList = PluginManager::instance().listClasses(ParaViewVTMFileFilter::OOClass());
	for(OvitoClassPtr clazz : filterClassList) {
		modifiedRequest.filters.push_back(static_object_cast<ParaViewVTMFileFilter>(clazz->createInstance()));

		// Let the plugin filter objects preprocess the multi-block structure before the referenced data files bget loaded.
		modifiedRequest.filters.back()->preprocessDatasets(blockDatasets);
	}

	// Load each dataset referenced by the VTM file. 
	Future<ExtendedLoadRequest> future = for_each(std::move(modifiedRequest), std::move(blockDatasets), dataset()->executor(), [](const std::pair<QStringList, QUrl>& blockDataset, ExtendedLoadRequest& request) {

		// Set up the load request submitted to the FileSourceImporter.
		request.dataBlockPrefix = blockDataset.first.back();
		request.blockPath = blockDataset.first;

		// Retrieve the data file.
		return Application::instance()->fileManager()->fetchUrl(request.dataset->taskManager(), blockDataset.second).then_future(request.dataset->executor(), [&request](SharedFuture<FileHandle> fileFuture) mutable -> Future<> {
			try {
				// Obtain a handle to the referenced data file.
				const FileHandle& file = fileFuture.result();

				// Give plugin filter objects the possibility to override the loading of the data file.
				for(const auto& filter : request.filters) {
					Future<> future = filter->loadDataset(request.blockPath, file, request);
					if(future.isValid())
						return future;
				}
				// If none of the filter objects decided to handle the loading process, fall back to our standard procedure,
				// which consists of detecting the file's format and delegating the file parsing to the corresponding FileSourceImporter class.

				// Detect file format and create an importer for it.
				// This currently works only for FileSourceImporters.
				// Files handled by other kinds of importers will be skipped.
				OORef<FileSourceImporter> importer = dynamic_object_cast<FileSourceImporter>(FileImporter::autodetectFileFormat(request.dataset, request.executionContext, file));
				if(!importer)
					return Future<>::createImmediateEmpty();

				// Remember the current status returned by the loading operations completed so far.
				// We will prepend this existing status text to the one generated by the current file importer.
				PipelineStatus lastStatus = request.state.status();

				// Set up the load request submitted to the FileSourceImporter.
				request.frame = Frame(file);
				request.fileHandle = file;
				request.state.setStatus(PipelineStatus::Success);

				// Give plugin filter objects the possibility to pass additional information to the specific FileSourceImporter.
				for(const auto& filter : request.filters)
					filter->configureImporter(request.blockPath, request, importer);

				// Parse the referenced file.
				// Note: We need to keep the FileSourceImporter object while the asynchronous parsing process is 
				// in progress. That's why we store an otherwise unused pointer to it in the lambda function. 
				return importer->loadFrame(request).then_future([importer, filename = file.sourceUrl().fileName(), &request, lastStatus](Future<PipelineFlowState> blockDataFuture) mutable {
					try {
						request.state = blockDataFuture.result();

						// Concatenate status strings for the data blocks loaded so far.
						QString statusString = lastStatus.text();
						if(!request.state.status().text().isEmpty()) {
							if(!statusString.isEmpty() && !statusString.endsWith(QChar('\n'))) statusString += QChar('\n');
							statusString += request.state.status().text();
						}

						// Also calculate a combined status code.
						PipelineStatus::StatusType statusType = lastStatus.type();
						if(statusType == PipelineStatus::Success || (statusType == PipelineStatus::Warning && request.state.status().type() == PipelineStatus::Error))
							statusType = request.state.status().type();
						request.state.setStatus(PipelineStatus(statusType, std::move(statusString)));
					}
					catch(Exception& ex) {
						ex.prependGeneralMessage(tr("Failed to load VTK multi-block dataset %1: %2").arg(request.dataBlockPrefix).arg(filename));
						throw ex;
					}
				});
			}
			catch(Exception& ex) {
				// Handle file errors, e.g. if the data block file referenced in the VTM file does not exist.
				request.state.setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar(' '))));
				ex.setContext(request.dataset);
				ex.prependGeneralMessage(tr("Failed to access data file referenced by block '%1' in VTK multi-block file.").arg(request.dataBlockPrefix));
				ex.reportError();
				// We treat such an error as recoverable and continue with loading the remaining data blocks. 
				return Future<>::createImmediateEmpty();
			}
		});
	});

	return future.then([](ExtendedLoadRequest&& request) -> PipelineFlowState {

		// Let the plugin filter objects post-process the loaded data.
		for(const auto& filter : request.filters)
			filter->postprocessDatasets(request);

		// Return just the PipelineFlowState to the caller.
		return std::move(request.state);
	});
}

}	// End of namespace
}	// End of namespace
