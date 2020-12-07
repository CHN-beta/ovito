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
#include <ovito/core/dataset/data/DataCollection.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/utilities/concurrent/ForEach.h>
#include <ovito/core/app/Application.h>
#include "ParaViewVTMImporter.h"

namespace Ovito { namespace Mesh {

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
	if(xml.name().compare(QStringLiteral("VTKFile")) != 0)
		return false;
	if(xml.attributes().value("type").compare(QStringLiteral("vtkMultiBlockDataSet")) != 0)
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
		if(xml.name().compare(QStringLiteral("VTKFile")) == 0) {
			if(xml.attributes().value("type").compare(QStringLiteral("vtkMultiBlockDataSet")) != 0)
				xml.raiseError(tr("VTM file is not of type vtkMultiBlockDataSet."));
		}
		else if(xml.name().compare(QStringLiteral("vtkMultiBlockDataSet")) == 0) {
			// Do nothing. Parse child elements.
		}
		else if(xml.name().compare(QStringLiteral("DataSet")) == 0) {

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
Future<PipelineFlowState> ParaViewVTMImporter::loadFrame(const LoadOperationRequest& request)
{
	// Load the VTM file, which contains the list of referenced data files.
	std::vector<std::pair<QUrl, QString>> blocks = loadVTMFile(request.fileHandle);

	// Load each data block referenced in the VTM file. 
	Future<LoadOperationRequest> future = for_each(request, std::move(blocks), executor(), [this](const std::pair<QUrl, QString>& block, LoadOperationRequest& request) {

		// Set up the load request submitted to the FileSourceImporter.
		request.dataBlockPrefix = block.second;

		// Retrieve the data file.
		return Application::instance()->fileManager()->fetchUrl(dataset()->taskManager(), block.first).then_future(executor(), [this, &request](SharedFuture<FileHandle> fileFuture) mutable -> Future<> {
			try {
				// Detect file format and create an importer for it.
				const FileHandle& file = fileFuture.result();
				OORef<FileImporter> importer = FileImporter::autodetectFileFormat(dataset(), request.executionContext, file);

				// This currently works only for FileSourceImporters.
				// Files formats handled by other kinds of importers will be skipped.
				OORef<FileSourceImporter> fsImporter = dynamic_object_cast<FileSourceImporter>(std::move(importer));
				if(!fsImporter)
					return Future<>::createImmediateEmpty();

				// Remember the current status returned by the load operations performed so far.
				// We will prepend this existing status to the one generated by the current file importer.
				PipelineStatus lastStatus = request.state.status();

				// Set up the load request submitted to the FileSourceImporter.
				request.frame = Frame(file);
				request.fileHandle = file;
				request.state.setStatus(PipelineStatus::Success);

				// Load the file.
				return fsImporter->loadFrame(request).then_future([this, fsImporter = std::move(fsImporter), filename = file.sourceUrl().fileName(), &request, lastStatus](Future<PipelineFlowState> blockDataFuture) mutable {
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
				ex.setContext(dataset());
				ex.prependGeneralMessage(tr("Failed to access data file referenced by block '%1' in VTK multi-block file.").arg(request.dataBlockPrefix));
				ex.reportError();
				// We treat such an error as recoverable and continue with loading the remaining data blocks. 
				return Future<>::createImmediateEmpty();
			}
		});
	});

	return future.then([](LoadOperationRequest&& request) -> PipelineFlowState {
		return std::move(request.state);
	});
}

}	// End of namespace
}	// End of namespace
