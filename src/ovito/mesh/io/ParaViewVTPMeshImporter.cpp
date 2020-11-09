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
#include <ovito/mesh/io/SurfaceMeshFrameData.h>
#include <ovito/mesh/surface/SurfaceMeshVertices.h>
#include "ParaViewVTPMeshImporter.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(ParaViewVTPMeshImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParaViewVTPMeshImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = file.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QXmlStreamReader xml(device.get());

	// Parse XML. First element must be <VTKFile type="PolyData">.
	if(xml.readNext() != QXmlStreamReader::StartDocument)
		return false;
	if(xml.readNext() != QXmlStreamReader::StartElement)
		return false;
	if(xml.name() != "VTKFile")
		return false;
	if(xml.attributes().value("type") != "PolyData")
		return false;

	// Continue until we reach the <Piece> element. 
	while(xml.readNextStartElement()) {
		if(xml.name() == "Piece") {
			// Number of triangle strips or polygons must be non-zero.
			if(xml.attributes().value("NumberOfStrips").toULongLong() != 0 || xml.attributes().value("NumberOfPolys").toULongLong() != 0)
				return !xml.hasError();
			break;
		}
	}

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr ParaViewVTPMeshImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTP PolyData file %1").arg(fileHandle().toString()));

	// Create the container for the mesh data to be loaded.
	std::shared_ptr<SurfaceMeshFrameData> frameData = std::make_shared<SurfaceMeshFrameData>();
	SurfaceMeshData& mesh = frameData->mesh();

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTP file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	size_t numberOfPoints = 0;
	size_t numberOfVerts = 0;
	size_t numberOfLines = 0;
	size_t numberOfStrips = 0;
	size_t numberOfPolys = 0;
	size_t numberOfCells = 0;
	SurfaceMeshData::vertex_index vertexBaseIndex = SurfaceMeshData::InvalidIndex;
	std::vector<PropertyPtr> cellDataArrays;

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return {};

		if(xml.name() == "VTKFile") {
			if(xml.attributes().value("type") != "PolyData")
				xml.raiseError(tr("VTK file is not of type PolyData."));
			else if(xml.attributes().value("byte_order") != "LittleEndian")
				xml.raiseError(tr("Byte order must be 'LittleEndian'. Please ask the OVITO developers to extend the capabilities of the file parser."));
			else if(xml.attributes().value("compressor") != "")
				xml.raiseError(tr("Current implementation does not support compressed data arrays. Please ask the OVITO developers to extend the capabilities of the file parser."));
		}
		else if(xml.name() == "PolyData") {
			// Do nothing. Parse child elements.
		}
		else if(xml.name() == "Piece") {
			// Parse geometric entity counts of the current piece.
			numberOfPoints = xml.attributes().value("NumberOfPoints").toULongLong();
			numberOfVerts = xml.attributes().value("NumberOfVerts").toULongLong();
			numberOfLines = xml.attributes().value("NumberOfLines").toULongLong();
			numberOfStrips = xml.attributes().value("NumberOfStrips").toULongLong();
			numberOfPolys = xml.attributes().value("NumberOfPolys").toULongLong();
			numberOfCells = numberOfVerts + numberOfLines + numberOfStrips + numberOfPolys;
			// Create geometry elements.
			vertexBaseIndex = mesh.createVertices(numberOfPoints);
			// Continue by parsing child elements.
		}
		else if(xml.name() == "PointData") {
			// Parse child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				PropertyPtr property = parseDataArray(xml);
				if(!property)
					break;
			}
		}
		else if(xml.name() == "Points") {
			// Parse child <DataArray> element containing the point coordinates.
			if(!xml.readNextStartElement())
				break;
			PropertyPtr property = parseDataArray(xml, PropertyStorage::Float);
			if(!property)
				break;

			// Make sure the data array has the expected data layout.
			if(property->componentCount() != 3 || property->name() != "Points" || property->size() != numberOfPoints) {
				xml.raiseError(tr("Points data array has wrong data layout, size or name."));
				break;
			}
			// Copy point coordinates from temporary array to surface mesh data structure.
			OVITO_ASSERT(property->size() + vertexBaseIndex == mesh.vertexCount());
			boost::copy(ConstPropertyAccess<Point3>(property), mesh.vertexCoordsRange().begin() + vertexBaseIndex);
			xml.skipCurrentElement();
		}
		else if(xml.name() == "Polys") {
			// Parse child <DataArray> element containing the connectivity information.
			if(!xml.readNextStartElement())
				break;
			PropertyPtr connectivityArray = parseDataArray(xml, qMetaTypeId<SurfaceMeshData::vertex_index>());
			if(!connectivityArray)
				break;
			// Make sure the data array has the expected data layout.
			if(connectivityArray->componentCount() != 1 || connectivityArray->name() != "connectivity") {
				xml.raiseError(tr("Connectivity data array has wrong data layout, size or name."));
				break;
			}

			// Parse child <DataArray> element containing the offset information.
			if(!xml.readNextStartElement())
				break;
			PropertyPtr offsetsArray = parseDataArray(xml, PropertyStorage::Int);
			if(!offsetsArray)
				break;
			// Make sure the data array has the expected data layout.
			if(offsetsArray->componentCount() != 1 || offsetsArray->size() != numberOfPolys || offsetsArray->name() != "offsets") {
				xml.raiseError(tr("Offsets data array has wrong data layout, size or name."));
				break;
			}

			// Go through the connectivity and the offsets arrays and create corresponding faces in the output mesh.
			ConstPropertyAccess<SurfaceMeshData::vertex_index> vertexIndices(connectivityArray);
			int previousOffset = 0;
			for(int offset : ConstPropertyAccess<int>(offsetsArray)) {
				if(offset < previousOffset + 3 || offset > vertexIndices.size()) {
					xml.raiseError(tr("Invalid or inconsistent connectivity information in <Polys> element."));
					break;
				}
				mesh.createFace(vertexIndices.begin() + previousOffset, vertexIndices.begin() + offset);
				previousOffset = offset;
			}
			if(xml.hasError())
				break;

			xml.skipCurrentElement();
		}
		else if(xml.name() == "CellData") {
			// Parse <DataArray> child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				if(xml.name() == "DataArray") {
					if(PropertyPtr property = parseDataArray(xml))
						cellDataArrays.push_back(std::move(property));
					else
						break;
				}
				else {
					xml.skipCurrentElement();
				}
			}
		}
		else if(xml.name() == "Verts" || xml.name() == "Lines" || xml.name() == "Strips") {
			// Do nothing. Ignore element contents.
			xml.skipCurrentElement();
		}
		else {
			xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("VTP file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}

	// Add cell data arrays to mesh.
	if(numberOfPolys == numberOfCells) {
		for(auto& property : cellDataArrays) {
			mesh.addFaceProperty(std::move(property));
		}
	}

	// Report number of vertices and faces to the user.
	frameData->setStatus(
		tr("Number of mesh vertices: %1\nNumber of mesh faces: %2")
		.arg(mesh.vertexCount())
		.arg(mesh.faceCount()));

	return std::move(frameData);
}

/******************************************************************************
* Reads a <DataArray> element and returns it as an OVITO property.
******************************************************************************/
PropertyPtr ParaViewVTPMeshImporter::FrameLoader::parseDataArray(QXmlStreamReader& xml, int convertToDataType)
{
	// Make sure it is really an <DataArray>.
	if(xml.name() != "DataArray") {
		xml.raiseError(tr("Expected <DataArray> element but found <%1> element.").arg(xml.name().toString()));
		return {};
	}

	// Check value of the 'format' attribute.
	QString format = xml.attributes().value("format").toString();
	if(format.isEmpty()) {
		xml.raiseError(tr("Expected 'format' attribute in <%1> element.").arg(xml.name().toString()));
		return {};
	}
	if(format != "binary") {
		xml.raiseError(tr("Current implementation supports only binary data arrays. Please ask the OVITO developers to extend the capabilities of the file parser."));
		return {};
	}

	// Check value of the 'type' attribute.
	QString dataType = xml.attributes().value("type").toString();
	size_t dataTypeSize;
	if(dataType == "Float32") {
		OVITO_STATIC_ASSERT(sizeof(float) == 4);
		dataTypeSize = sizeof(float);
	}
	else if(dataType == "Float64") {
		OVITO_STATIC_ASSERT(sizeof(double) == 8);
		dataTypeSize = sizeof(double);
	}
	else if(dataType == "Int32") {
		OVITO_STATIC_ASSERT(sizeof(qint32) == 4);
		dataTypeSize = sizeof(qint32);
	}
	else if(dataType == "Int64") {
		OVITO_STATIC_ASSERT(sizeof(qint64) == 8);
		dataTypeSize = sizeof(qint64);
	}
	else {
		xml.raiseError(tr("Current implementation supports only data arrays of type 'Int32', 'Int64', 'Float32' and 'Float64'. Please ask the OVITO developers to extend the capabilities of the file parser."));
		return {};
	}

	// Parse number of array components.
	int numComponents = std::max(1, xml.attributes().value("NumberOfComponents").toInt());

	// Parse array name.
	QString name = xml.attributes().value("Name").toString();

	// Parse the contents of the XML element and convert binary data from base64 encoding.
	QString text = xml.readElementText();
	QByteArray byteArray = QByteArray::fromBase64(text.toLatin1());

	// Note: Decoded binary data is prepended with array size information.
	qint64 dataArraySize = (byteArray.size() >= sizeof(qint64)) ? *reinterpret_cast<const qint64*>(byteArray.constData()) : -1;
	if(dataArraySize + sizeof(qint64) != byteArray.size()) {
		xml.raiseError(tr("Data array size mismatch: XML element contains more or less data than specified."));
		return {};
	}

	// Calculate the number of array elements from the size in bytes.
	size_t elementCount = dataArraySize / (dataTypeSize * numComponents);
	if(elementCount * dataTypeSize * numComponents != dataArraySize) {
		xml.raiseError(tr("Data array size is invalid: Not an integer number of data elements."));
		return {};
	}

	// Determine data type of the target property to create.
	if(convertToDataType == 0) {
		if(dataType == "Float32" || dataType == "Float64") convertToDataType = PropertyStorage::Float;
		else if(dataType == "Int32") convertToDataType = PropertyStorage::Int;
		else if(dataType == "Int64") convertToDataType = PropertyStorage::Int64;
		else OVITO_ASSERT(false);
	}

	// Allocate destination buffer.
	PropertyPtr property = std::make_shared<PropertyStorage>(elementCount, convertToDataType, numComponents, 0, name, false);

	if(dataType == "Float32") {
		const float* begin = reinterpret_cast<const float*>(byteArray.constData() + sizeof(qint64));
		const float* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyStorage::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else if(dataType == "Float64") {
		const double* begin = reinterpret_cast<const double*>(byteArray.constData() + sizeof(qint64));
		const double* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyStorage::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else if(dataType == "Int32") {
		const qint32* begin = reinterpret_cast<const qint32*>(byteArray.constData() + sizeof(qint64));
		const qint32* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyStorage::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else if(dataType == "Int64") {
		const qint64* begin = reinterpret_cast<const qint64*>(byteArray.constData() + sizeof(qint64));
		const qint64* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyStorage::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyStorage::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else {
		OVITO_ASSERT(false);
	}
	return property;
}

}	// End of namespace
}	// End of namespace
