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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/grid/io/ParaViewVTIGridImporter.h>
#include "ParaViewVTPParticleImporter.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParaViewVTPParticleImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParaViewVTPParticleImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
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
	if(xml.name().compare(QLatin1String("VTKFile")) != 0)
		return false;
	if(xml.attributes().value("type").compare(QLatin1String("PolyData")) != 0)
		return false;

	// Continue until we reach the <Piece> element. 
	while(xml.readNextStartElement()) {
		if(xml.name().compare(QLatin1String("Piece")) == 0) {
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
void ParaViewVTPParticleImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading ParaView VTP particles file %1").arg(fileHandle().toString()));

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTP file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	// Parse the elements of the XML file.
	while(xml.readNextStartElement()) {
		if(isCanceled())
			return;

		if(xml.name().compare(QLatin1String("VTKFile")) == 0) {
			if(xml.attributes().value("type").compare(QLatin1String("PolyData")) != 0)
				xml.raiseError(tr("VTK file is not of type PolyData."));
			else if(xml.attributes().value("byte_order").compare(QLatin1String("LittleEndian")) != 0)
				xml.raiseError(tr("Byte order must be 'LittleEndian'. Please contact the OVITO developers to request an extension of the file parser."));
			else if(xml.attributes().value("compressor").compare(QLatin1String("")) != 0)
				xml.raiseError(tr("The parser does not support compressed data arrays. Please contact the OVITO developers to request an extension of the file parser."));
		}
		else if(xml.name().compare(QLatin1String("PolyData")) == 0) {
			// Do nothing. Parse child elements.
		}
		else if(xml.name().compare(QLatin1String("Piece")) == 0) {

			// Parse number of lines, triangle strips and polygons.
			if(xml.attributes().value("NumberOfLines").toULongLong() != 0
					|| xml.attributes().value("NumberOfStrips").toULongLong() != 0
					|| xml.attributes().value("NumberOfPolys").toULongLong() != 0) {
				xml.raiseError(tr("Number of lines, strips and polys are nonzero. This particle file parser can only read PolyData datasets containing vertices only."));
				break;
			}

			// Parse number of points.
			size_t numParticles = xml.attributes().value("NumberOfPoints").toULongLong();
			// Parse number of vertices.
			size_t numVertices = xml.attributes().value("NumberOfVerts").toULongLong();
			if(numVertices != numParticles) {
				xml.raiseError(tr("Number of vertices does not match number of points. This file parser can only read datasets consisting of vertices only."));
				break;
			}
			setParticleCount(numParticles);
		}
		else if(xml.name().compare(QLatin1String("PointData")) == 0 || xml.name().compare(QLatin1String("Points")) == 0 || xml.name().compare(QLatin1String("Verts")) == 0) {
			// Parse child elements.
			while(xml.readNextStartElement() && !isCanceled()) {
				if(xml.name().compare(QLatin1String("DataArray")) == 0) {
					int vectorComponent = -1;
					if(PropertyObject* property = createParticlePropertyForDataArray(xml, vectorComponent)) {
						ParaViewVTIGridImporter::parseVTKDataArray(property, vectorComponent, xml);
					}
					if(xml.tokenType() != QXmlStreamReader::EndElement)
						xml.skipCurrentElement();
				}
				else {
					xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
				}
			}
		}
		else if(xml.name().compare(QLatin1String("CellData")) == 0 || xml.name().compare(QLatin1String("Lines")) == 0 || xml.name().compare(QLatin1String("Strips")) == 0 || xml.name().compare(QLatin1String("Polys")) == 0) {
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
		
	// Convert 3x3 'Tensor' property into particle orientation.
	if(const PropertyObject* tensorProperty = particles()->getProperty(QStringLiteral("Tensor"))) {
		if(tensorProperty->dataType() == PropertyObject::Float && tensorProperty->componentCount() == 9) {
			PropertyAccess<Quaternion> orientations = particles()->createProperty(ParticlesObject::OrientationProperty, false, executionContext());
			Quaternion* q = orientations.begin();
			for(const Matrix3& tensor : ConstPropertyAccess<Matrix3>(tensorProperty)) {
				*q++ = Quaternion(tensor.transposed());
			}
		}
	}
	if(isCanceled())
		return;

	// Convert superquadric 'Blockiness' values from the Aspherix simulation to 'Roundness' values used by OVITO particle visualization.
	if(PropertyObject* roundnessProperty = particles()->getMutableProperty(ParticlesObject::SuperquadricRoundnessProperty)) {
		for(Vector2& v : PropertyAccess<Vector2>(roundnessProperty)) {
    		// Roundness = 2.0 / Blockiness
			if(v.x() != 0) v.x() = FloatType(2) / v.x();
			if(v.y() != 0) v.y() = FloatType(2) / v.y();
		}
	}

	// Report number of particles to the user.
	QString statusString = tr("Number of particles: %1").arg(particles()->elementCount());
	state().setStatus(std::move(statusString));

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

/******************************************************************************
* Creates the right kind of OVITO property object that will receive the data 
* read from a <DataArray> element.
******************************************************************************/
PropertyObject* ParaViewVTPParticleImporter::FrameLoader::createParticlePropertyForDataArray(QXmlStreamReader& xml, int& vectorComponent)
{
	int numComponents = std::max(1, xml.attributes().value("NumberOfComponents").toInt());
	auto name = xml.attributes().value("Name");

	if(name.compare(QLatin1String("points"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::PositionProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("id"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::IdentifierProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("type"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::TypeProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("mass"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::MassProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("radius"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(ParticlesObject::RadiusProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("v"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::VelocityProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("omega"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::AngularVelocityProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("tq"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::TorqueProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("f"), Qt::CaseInsensitive) == 0 && numComponents == 3) {
		return particles()->createProperty(ParticlesObject::ForceProperty, false, executionContext());
	}
	else if(name.compare(QLatin1String("density"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		return particles()->createProperty(QStringLiteral("Density"), PropertyObject::Float, 1, 0, false);
	}
	else if(name.compare(QLatin1String("tensor"), Qt::CaseInsensitive) == 0 && numComponents == 9) {
		return particles()->createProperty(QStringLiteral("Tensor"), PropertyObject::Float, 9, 0, false);
	}
	else if(name.compare(QLatin1String("shapex"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 0;
		return particles()->createProperty(ParticlesObject::AsphericalShapeProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("shapey"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 1;
		return particles()->createProperty(ParticlesObject::AsphericalShapeProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("shapez"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 2;
		return particles()->createProperty(ParticlesObject::AsphericalShapeProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("blockiness1"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 0;
		return particles()->createProperty(ParticlesObject::SuperquadricRoundnessProperty, true, executionContext());
	}
	else if(name.compare(QLatin1String("blockiness2"), Qt::CaseInsensitive) == 0 && numComponents == 1) {
		vectorComponent = 1;
		return particles()->createProperty(ParticlesObject::SuperquadricRoundnessProperty, true, executionContext());
	}
	else {
		return particles()->createProperty(name.toString(), PropertyObject::Float, numComponents, 0, false);
	}
	return nullptr;
}

}	// End of namespace
}	// End of namespace
