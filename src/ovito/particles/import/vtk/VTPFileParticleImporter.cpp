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

#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include "VTPFileParticleImporter.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(VTPFileParticleImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool VTPFileParticleImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
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
			// Number of lines, triangle strips, and polygons must be zero.
			if(xml.attributes().value("NumberOfLines").toULongLong() == 0 && xml.attributes().value("NumberOfStrips").toULongLong() == 0 && xml.attributes().value("NumberOfPolys").toULongLong() == 0) {
				// Number of vertices must match number of points.
				if(xml.attributes().value("NumberOfPoints") == xml.attributes().value("NumberOfVerts")) {
					return !xml.hasError();
				}
			}
			break;
		}
	}

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr VTPFileParticleImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTP particles file %1").arg(fileHandle().toString()));

	// Create the container for the particle data to be loaded.
	std::shared_ptr<ParticleFrameData> frameData = std::make_shared<ParticleFrameData>();
	size_t numParticles = 0;

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTP file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return {};

		if(xml.name() == "VTKFile") {
			if(xml.attributes().value("type") != "PolyData")
				xml.raiseError(tr("VTK file is not of type PolyData."));
			else if(xml.attributes().value("byte_order") != "LittleEndian")
				xml.raiseError(tr("Byte order must be 'LittleEndian'. Please contact the OVITO developers to request an extension of the file parser."));
			else if(xml.attributes().value("compressor") != "")
				xml.raiseError(tr("The parser does not support compressed data arrays. Please contact the OVITO developers to request an extension of the file parser."));
		}
		else if(xml.name() == "PolyData") {
			// Do nothing. Parse child elements.
		}
		else if(xml.name() == "Piece") {

			// Parse number of lines, triangle strips and polygons.
			if(xml.attributes().value("NumberOfLines").toULongLong() != 0
					|| xml.attributes().value("NumberOfStrips").toULongLong() != 0
					|| xml.attributes().value("NumberOfPolys").toULongLong() != 0) {
				xml.raiseError(tr("Number of lines, strips and polys are nonzero. This particle file parser can only read PolyData datasets containing vertices only."));
				break;
			}

			// Parse number of points.
			numParticles = xml.attributes().value("NumberOfPoints").toULongLong();
			// Parse number of vertices.
			size_t numVertices = xml.attributes().value("NumberOfVerts").toULongLong();
			if(numVertices != numParticles) {
				xml.raiseError(tr("Number of vertices does not match number of points. This file parser can only read datasets consisting of vertices only."));
				break;
			}
		}
		else if(xml.name() == "PointData" || xml.name() == "Points" || xml.name() == "Verts") {
			// Parse child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				if(xml.name() == "DataArray") {
					if(PropertyStorage* property = createParticlePropertyForDataArray(frameData->particles(), numParticles, xml)) {
						parseDataArray(property, xml);
					}
					if(xml.tokenType() != QXmlStreamReader::EndElement)
						xml.skipCurrentElement();
				}
				else {
					xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name()));
				}
			}
		}
		else if(xml.name() == "CellData" || xml.name() == "Lines" || xml.name() == "Strips" || xml.name() == "Polys") {
			// Do nothing. Ignore element contents.
			xml.skipCurrentElement();
		}
		else {
			xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name()));
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("VTP file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}

	// Report number of particles to the user.
	QString statusString = tr("Number of particles: %1").arg(numParticles);
	frameData->setStatus(std::move(statusString));

	return std::move(frameData);
}

/******************************************************************************
* Creates the right kind of OVITO property object that will receive the data 
* read from a <DataArray> element.
******************************************************************************/
PropertyStorage* VTPFileParticleImporter::FrameLoader::createParticlePropertyForDataArray(PropertyContainerImportData& container, size_t particleCount, QXmlStreamReader& xml)
{
	int numComponents = std::max(1, xml.attributes().value("NumberOfComponents").toInt());
	auto name = xml.attributes().value("Name");

	if(QStringRef::compare(name, "points", Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::PositionProperty, false);
	}
	else if(QStringRef::compare(name, "id", Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::IdentifierProperty, false);
	}
	else if(QStringRef::compare(name, "type", Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::TypeProperty, false);
	}
	else if(QStringRef::compare(name, "mass", Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::MassProperty, false);
	}
	else if(QStringRef::compare(name, "radius", Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::RadiusProperty, false);
	}
	else if(QStringRef::compare(name, "v", Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::VelocityProperty, false);
	}
	else if(QStringRef::compare(name, "omega", Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::AngularVelocityProperty, false);
	}
	else if(QStringRef::compare(name, "tq", Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return container.createStandardProperty<ParticlesObject>(particleCount, ParticlesObject::TorqueProperty, false);
	}
	else if(QStringRef::compare(name, "density", Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return container.addProperty(std::make_shared<PropertyStorage>(particleCount, PropertyStorage::Float, 1, 0, QStringLiteral("Density"), false));
	}
	return nullptr;
}

/******************************************************************************
* Reads a <DataArray> element and stores it in the given OVITO property.
******************************************************************************/
void VTPFileParticleImporter::FrameLoader::parseDataArray(PropertyStorage* property, QXmlStreamReader& xml)
{
	// Check value of the 'format' attribute.
	QString format = xml.attributes().value("format").toString();
	if(format.isEmpty()) {
		xml.raiseError(tr("Expected 'format' attribute in <%1> element.").arg(xml.name()));
		return;
	}
	if(format != "binary") {
		xml.raiseError(tr("Parser supports only binary data arrays. Please contact the OVITO developers to request an extension of the file parser."));
		return;
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
		xml.raiseError(tr("Parser supports only data arrays of type 'Int32', 'Int64', 'Float32' and 'Float64'. Please contact the OVITO developers to request an extension of the file parser."));
		return;
	}

	// Parse the contents of the XML element and convert binary data from base64 encoding.
	QString text = xml.readElementText();
	QByteArray byteArray = QByteArray::fromBase64(text.toLatin1());
	qint64 expectedBytes = property->size() * dataTypeSize * property->componentCount();
	// Note: Decoded binary data is prepended with array size information.
	if(byteArray.size() != expectedBytes + sizeof(qint64)) {
		xml.raiseError(tr("Data array size mismatch: Expected %1 bytes of base64 encoded data, but XML element contains %2 bytes.")
			.arg(expectedBytes + sizeof(qint64))
			.arg(byteArray.size()));
		return;
	}
	qint64 dataSize = *reinterpret_cast<const qint64*>(byteArray.constData());
	if(dataSize != expectedBytes) {
		xml.raiseError(tr("Data array size mismatch: Expected %1 bytes of payload, but XML element contains data array with %2 bytes.")
			.arg(expectedBytes)
			.arg(dataSize));
		return;
	}

	if(dataType == "Float32") {
		const float* begin = reinterpret_cast<const float*>(byteArray.constData() + sizeof(qint64));
		const float* end = begin + property->size() * property->componentCount();
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
		const double* end = begin + property->size() * property->componentCount();
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
		const qint32* end = begin + property->size() * property->componentCount();
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
		const qint64* end = begin + property->size() * property->componentCount();
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
		property->fillZero();
	}
}


}	// End of namespace
}	// End of namespace
