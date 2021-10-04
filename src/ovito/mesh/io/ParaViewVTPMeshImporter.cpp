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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/mesh/surface/SurfaceMeshAccess.h>
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
	if(xml.name().compare(QStringLiteral("VTKFile")) != 0)
		return false;
	if(xml.attributes().value("type").compare(QStringLiteral("PolyData")) != 0)
		return false;

	// Continue until we reach the <Piece> element. 
	while(xml.readNextStartElement()) {
		if(xml.name().compare(QStringLiteral("Piece")) == 0) {
			// Number of triangle strips or polygons must be non-zero.
			if(xml.attributes().value("NumberOfStrips").toULongLong() != 0 || xml.attributes().value("NumberOfPolys").toULongLong() != 0)
				return !xml.hasError();
			break;
		}
	}

	return false;
}

/******************************************************************************
* Parses the input file.
******************************************************************************/
void ParaViewVTPMeshImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTP PolyData file %1").arg(fileHandle().toString()));

	// Create the destination mesh object.
	QString meshIdentifier = loadRequest().dataBlockPrefix;
	SurfaceMesh* meshObj = state().getMutableLeafObject<SurfaceMesh>(SurfaceMesh::OOClass(), meshIdentifier);
	if(!meshObj) {
		meshObj = state().createObject<SurfaceMesh>(dataSource(), executionContext());
		meshObj->setIdentifier(meshIdentifier);
		SurfaceMeshVis* vis = meshObj->visElement<SurfaceMeshVis>();
		if(vis) {
			vis->setShowCap(false);
			vis->setSmoothShading(true);
		}
		if(!meshIdentifier.isEmpty()) {
			meshObj->setTitle(tr("Mesh: %1").arg(meshIdentifier));
			if(vis) vis->setTitle(tr("Mesh: %1").arg(meshIdentifier));
		}
	}
	SurfaceMeshAccess mesh(meshObj);

	// Reset mesh or append data to existing mesh.
	if(!loadRequest().appendData)
		mesh.clearMesh();

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
	SurfaceMeshAccess::vertex_index vertexBaseIndex = SurfaceMeshAccess::InvalidIndex;
	SurfaceMeshAccess::face_index faceBaseIndex = SurfaceMeshAccess::InvalidIndex;
	std::vector<PropertyPtr> cellDataArrays;

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return;

		if(xml.name().compare(QStringLiteral("VTKFile")) == 0) {
			if(xml.attributes().value("type").compare(QStringLiteral("PolyData")) != 0)
				xml.raiseError(tr("VTK file is not of type PolyData."));
			else if(xml.attributes().value("byte_order").compare(QStringLiteral("LittleEndian")) != 0)
				xml.raiseError(tr("Byte order must be 'LittleEndian'. Please ask the OVITO developers to extend the capabilities of the file parser."));
			else if(!xml.attributes().value("compressor").isEmpty())
				xml.raiseError(tr("Current implementation does not support compressed data arrays. Please ask the OVITO developers to extend the capabilities of the file parser."));
		}
		else if(xml.name().compare(QStringLiteral("PolyData")) == 0) {
			// Do nothing. Parse child elements.
		}
		else if(xml.name().compare(QStringLiteral("Piece")) == 0) {
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
		else if(xml.name().compare(QStringLiteral("PointData")) == 0) {
			// Parse child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				PropertyPtr property = parseDataArray(xml);
				if(!property)
					break;
			}
		}
		else if(xml.name().compare(QStringLiteral("Points")) == 0) {
			// Parse child <DataArray> element containing the point coordinates.
			if(!xml.readNextStartElement())
				break;
			PropertyPtr property = parseDataArray(xml, PropertyObject::Float);
			if(!property)
				break;

			// Make sure the data array has the expected data layout.
			if(property->componentCount() != 3 || property->name() != "Points" || property->size() != numberOfPoints) {
				xml.raiseError(tr("Points data array has wrong data layout, size or name."));
				break;
			}
			// Copy point coordinates from temporary array to surface mesh data structure.
			OVITO_ASSERT(property->size() + vertexBaseIndex == mesh.vertexCount());
			boost::copy(ConstPropertyAccess<Point3>(property), std::next(std::begin(mesh.mutableVertexPositions()), vertexBaseIndex));
			xml.skipCurrentElement();
		}
		else if(xml.name().compare(QStringLiteral("Polys")) == 0) {
			// Parse child <DataArray> element containing the connectivity information.
			if(!xml.readNextStartElement())
				break;
			PropertyPtr connectivityArray = parseDataArray(xml, qMetaTypeId<SurfaceMeshAccess::vertex_index>());
			if(!connectivityArray)
				break;
			// Make sure the data array has the expected data layout.
			if(connectivityArray->componentCount() != 1 || connectivityArray->name() != "connectivity") {
				xml.raiseError(tr("Connectivity data array has wrong data layout, size or name."));
				break;
			}
			faceBaseIndex = mesh.faceCount();

			// Parse child <DataArray> element containing the offset information.
			if(!xml.readNextStartElement())
				break;
			PropertyPtr offsetsArray = parseDataArray(xml, PropertyObject::Int);
			if(!offsetsArray)
				break;
			// Make sure the data array has the expected data layout.
			if(offsetsArray->componentCount() != 1 || offsetsArray->size() != numberOfPolys || offsetsArray->name() != "offsets") {
				xml.raiseError(tr("Offsets data array has wrong data layout, size or name."));
				break;
			}

			// Shift vertex indices in the array by base vertex offset.
			PropertyAccess<SurfaceMeshAccess::vertex_index> vertexIndices(connectivityArray);
			if(vertexBaseIndex != 0)
				for(SurfaceMeshAccess::vertex_index& idx : vertexIndices) idx += vertexBaseIndex;

			// Go through the connectivity and the offsets arrays and create corresponding faces in the output mesh.
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
		else if(xml.name().compare(QStringLiteral("CellData")) == 0) {
			// Parse <DataArray> child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				if(xml.name().compare(QStringLiteral("DataArray")) == 0) {
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
		else if(xml.name().compare(QStringLiteral("Verts")) == 0 || xml.name().compare(QStringLiteral("Lines")) == 0 || xml.name().compare(QStringLiteral("Strips")) == 0) {
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
	if(isCanceled())
		return;

	// Add cell data arrays to the mesh.
	if(numberOfPolys == numberOfCells) {
		for(auto& property : cellDataArrays) {
			OVITO_ASSERT(property->size() == numberOfCells);
			// If it is the first partial dataset we are loading, or if we are loading the mesh in once piece, then 
			// the loaded property arrays can simply be added to the mesh faces.
			// Otherwise, if we are loading subsequent parts of the distributed mesh, 
			// then the loaded property values must be copied into the correct subrange of the existing
			// face properties.
			if(!loadRequest().appendData) {
				OVITO_ASSERT(property->size() == mesh.faceCount());
				OVITO_ASSERT(faceBaseIndex == 0);
				mesh.addFaceProperty(std::move(property));
			}
			else {
				PropertyObject* existingProperty = property->type() != SurfaceMeshFaces::UserProperty 
					? mesh.mutableFaceProperty(static_cast<SurfaceMeshFaces::Type>(property->type())) 
					: mesh.mutableFaceProperty(property->name());
				if(existingProperty && existingProperty->dataType() == property->dataType() && existingProperty->componentCount() == property->componentCount()) {
					existingProperty->copyRangeFrom(*property, 0, faceBaseIndex, property->size());
				}
			}
		}
	}

	// Report number of vertices and faces to the user.
	if(meshIdentifier.isEmpty()) {
		state().setStatus(
			tr("Number of mesh vertices: %1\nNumber of mesh faces: %2")
			.arg(mesh.vertexCount())
			.arg(mesh.faceCount()));
	}
	else {
		state().setStatus(
			tr("Mesh %1: %2 vertices / %3 faces")
			.arg(meshIdentifier)
			.arg(mesh.vertexCount())
			.arg(mesh.faceCount()));
	}

	// Call base implementation.
	StandardFrameLoader::loadFile();
}

/******************************************************************************
* Reads a <DataArray> element and returns it as an OVITO property.
******************************************************************************/
PropertyPtr ParaViewVTPMeshImporter::FrameLoader::parseDataArray(QXmlStreamReader& xml, int convertToDataType)
{
	// Make sure it is really an <DataArray>.
	if(xml.name().compare(QStringLiteral("DataArray")) != 0) {
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
	QString text = xml.readElementText(QXmlStreamReader::SkipChildElements);
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
		if(dataType == "Float32" || dataType == "Float64") convertToDataType = PropertyObject::Float;
		else if(dataType == "Int32") convertToDataType = PropertyObject::Int;
		else if(dataType == "Int64") convertToDataType = PropertyObject::Int64;
		else OVITO_ASSERT(false);
	}

	// Allocate destination buffer.
	PropertyPtr property = DataOORef<PropertyObject>::create(dataset(), executionContext(), elementCount, convertToDataType, numComponents, 0, name, false);

	if(dataType == "Float32") {
		const float* begin = reinterpret_cast<const float*>(byteArray.constData() + sizeof(qint64));
		const float* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyObject::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else if(dataType == "Float64") {
		const double* begin = reinterpret_cast<const double*>(byteArray.constData() + sizeof(qint64));
		const double* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyObject::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else if(dataType == "Int32") {
		const qint32* begin = reinterpret_cast<const qint32*>(byteArray.constData() + sizeof(qint64));
		const qint32* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyObject::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int64) {
			std::copy(begin, end, PropertyAccess<qlonglong, true>(property).begin());
		}
	}
	else if(dataType == "Int64") {
		const qint64* begin = reinterpret_cast<const qint64*>(byteArray.constData() + sizeof(qint64));
		const qint64* end = begin + (elementCount * numComponents);
		if(property->dataType() == PropertyObject::Float) {
			std::copy(begin, end, PropertyAccess<FloatType, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int) {
			std::copy(begin, end, PropertyAccess<int, true>(property).begin());
		}
		else if(property->dataType() == PropertyObject::Int64) {
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
