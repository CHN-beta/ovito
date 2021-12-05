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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "GALAMOSTImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(GALAMOSTImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GALAMOSTImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = file.createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QXmlStreamReader xml(device.get());

	// Parse XML. First element must be <galamost_xml version="...">.
	if(xml.readNext() != QXmlStreamReader::StartDocument)
		return false;
	if(xml.readNext() != QXmlStreamReader::StartElement)
		return false;
	if(xml.name().compare(QStringLiteral("galamost_xml")) != 0)
		return false;
	if(!xml.attributes().hasAttribute("version"))
		return false;

	return !xml.hasError();
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void GALAMOSTImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading GALAMOST file %1").arg(fileHandle().toString()));

	// Initialize XML reader and open input file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception(tr("Failed to open VTP file: %1").arg(device->errorString()));
	QXmlStreamReader xml(device.get());

	/// The dimensionality of the dataset.
	int dimensions = 3;

	/// The number of atoms.
	size_t natoms = 0;

	/// The number of bonds.
	size_t nbonds = 0;

	/// Expect <galamost_xml> root element.
	if(!xml.readNextStartElement() || xml.name().compare(QStringLiteral("galamost_xml")) != 0)
		xml.raiseError(tr("Expected <galamost_xml> XML element."));
	else {
		if(!xml.readNextStartElement() || xml.name().compare(QStringLiteral("configuration")) != 0)
			xml.raiseError(tr("Expected <configuration> XML element."));
		else {

			// Parse simulation timestep.
			auto timeStepStr = xml.attributes().value(QStringLiteral("time_step"));
			if(!timeStepStr.isEmpty()) {
				bool ok;
				state().setAttribute(QStringLiteral("Timestep"), QVariant::fromValue(timeStepStr.toLongLong(&ok)), dataSource());
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'time_step' attribute value in <%1> element: %2").arg(xml.name()).arg(timeStepStr));
			}

			// Parse dimensionality.
			auto dimensionsStr = xml.attributes().value(QStringLiteral("dimensions"));
			if(!dimensionsStr.isEmpty()) {
				dimensions = dimensionsStr.toInt();
				if(dimensions != 2 && dimensions != 3)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'dimensions' attribute value in <%1> element: %2").arg(xml.name()).arg(dimensionsStr));
			}

			// Parse number of atoms.
			auto natomsStr = xml.attributes().value(QStringLiteral("natoms"));
			if(!natomsStr.isEmpty()) {
				bool ok;
				natoms = natomsStr.toULongLong(&ok);
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'natoms' attribute value in <%1> element: %2").arg(xml.name()).arg(natomsStr));
				setParticleCount(natoms);
			}
			else {
				throw Exception(tr("GALAMOST file parsing error. Expected 'natoms' attribute in <%1> element.").arg(xml.name()));
			}

			// Parse the child elements.
			while(xml.readNextStartElement()) {
				if(isCanceled())
					return;

				if(xml.name().compare(QStringLiteral("box")) == 0) {
					// Parse box dimensions.
					AffineTransformation cellMatrix = simulationCell()->cellMatrix();
					auto lxStr = xml.attributes().value(QStringLiteral("lx"));
					if(!lxStr.isEmpty()) {
						bool ok;
						cellMatrix(0,0) = (FloatType)lxStr.toDouble(&ok);
						if(!ok)
							throw Exception(tr("GALAMOST file parsing error. Invalid 'lx' attribute value in <%1> element: %2").arg(xml.name()).arg(lxStr));
					}
					auto lyStr = xml.attributes().value(QStringLiteral("ly"));
					if(!lyStr.isEmpty()) {
						bool ok;
						cellMatrix(1,1) = (FloatType)lyStr.toDouble(&ok);
						if(!ok)
							throw Exception(tr("GALAMOST file parsing error. Invalid 'ly' attribute value in <%1> element: %2").arg(xml.name()).arg(lyStr));
					}
					auto lzStr = xml.attributes().value(QStringLiteral("lz"));
					if(!lzStr.isEmpty()) {
						bool ok;
						cellMatrix(2,2) = (FloatType)lzStr.toDouble(&ok);
						if(!ok)
							throw Exception(tr("GALAMOST file parsing error. Invalid 'lz' attribute value in <%1> element: %2").arg(xml.name()).arg(lzStr));
					}
					if(dimensions == 2)
						simulationCell()->setIs2D(true);
					cellMatrix.translation() = cellMatrix * Vector3(-0.5, -0.5, -0.5);
					simulationCell()->setCellMatrix(cellMatrix);
					xml.skipCurrentElement();
				}
				else if(xml.name().compare(QStringLiteral("position")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::PositionProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("velocity")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::VelocityProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("image")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::PeriodicImageProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("mass")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::MassProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("diameter")) == 0) {
					PropertyObject* property = parsePropertyData(xml, particles()->createProperty(ParticlesObject::RadiusProperty, false, initializationHints()));
					// Convert diamater values into radii.
					for(FloatType& radius : PropertyAccess<FloatType>(property))
						radius /= 2;
				}
				else if(xml.name().compare(QStringLiteral("charge")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::ChargeProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("quaternion")) == 0) {
					PropertyObject* property = parsePropertyData(xml, particles()->createProperty(ParticlesObject::OrientationProperty, false, initializationHints()));
					// Convert quaternion representation to OVITO's internal format.
					// Left-shift all quaternion components by one: (W,X,Y,Z) -> (X,Y,Z,W).
					for(Quaternion& q : PropertyAccess<Quaternion>(property))
						std::rotate(q.begin(), q.begin() + 1, q.end());
				}
				else if(xml.name().compare(QStringLiteral("orientation")) == 0) {
					DataOORef<PropertyObject> directions = ParticlesObject::OOClass().createUserProperty(dataset(), natoms, PropertyObject::Float, 3, 0, QStringLiteral("Direction"), false);
					parsePropertyData(xml, directions);
					ConstPropertyAccess<Vector3> directionsAccess(directions);
					const Vector3* dir = directionsAccess.cbegin();
					for(Quaternion& q : PropertyAccess<Quaternion>(particles()->createProperty(ParticlesObject::OrientationProperty, false, initializationHints()))) {
						if(!dir->isZero()) {
							Rotation r(Vector3(0,0,1), *dir);
							q = Quaternion(r);
						}
						else {
							q = Quaternion::Identity();
						}
						++dir;
					}
					OVITO_ASSERT(dir == directionsAccess.cend());
				}
				else if(xml.name().compare(QStringLiteral("type")) == 0) {
					QString text = xml.readElementText();
					QTextStream stream(&text, QIODevice::ReadOnly | QIODevice::Text);
					PropertyObject* property = particles()->createProperty(ParticlesObject::TypeProperty, false, initializationHints());
					QString typeName;
					for(int& type : PropertyAccess<int>(property)) {
						stream >> typeName;
						type = addNamedType(ParticlesObject::OOClass(), property, typeName)->numericId();
					}
					property->sortElementTypesByName();
				}
				else if(xml.name().compare(QStringLiteral("molecule")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::MoleculeProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("body")) == 0) {
					parsePropertyData(xml, particles()->createProperty(QStringLiteral("Body"), PropertyObject::Int64, 1, 0, false));
				}
				else if(xml.name().compare(QStringLiteral("Aspheres")) == 0) {
					QString text = xml.readElementText();
					QTextStream stream(&text, QIODevice::ReadOnly | QIODevice::Text);
					ConstPropertyAccess<int> typeProperty = particles()->getProperty(ParticlesObject::TypeProperty);
					if(!typeProperty)
						throw Exception(tr("GALAMOST file parsing error. <%1> element must appear after <type> element.").arg(xml.name()));
					std::vector<Vector3> typesAsphericalShape;
					while(!stream.atEnd()) {
						QString typeName;
						FloatType a,b,c;
						FloatType eps_a, eps_b, eps_c;
						stream >> typeName >> a >> b >> c >> eps_a >> eps_b >> eps_c;
						stream.skipWhiteSpace();
						for(const ElementType* type : typeProperty.buffer()->elementTypes()) {
							if(type->name() == typeName) {
								if(typesAsphericalShape.size() <= type->numericId()) typesAsphericalShape.resize(type->numericId()+1, Vector3::Zero());
								typesAsphericalShape[type->numericId()] = Vector3(a/2,b/2,c/2);
								break;
							}
						}
						const int* typeIndex = typeProperty.cbegin();
						for(Vector3& shape : PropertyAccess<Vector3>(particles()->createProperty(ParticlesObject::AsphericalShapeProperty, false, initializationHints()))) {
							if(*typeIndex < typesAsphericalShape.size())
								shape = typesAsphericalShape[*typeIndex];
							++typeIndex;
						}
					}
				}
				else if(xml.name().compare(QStringLiteral("rotation")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::AngularVelocityProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("inert")) == 0) {
					parsePropertyData(xml, particles()->createProperty(ParticlesObject::AngularMomentumProperty, false, initializationHints()));
				}
				else if(xml.name().compare(QStringLiteral("bond")) == 0) {
					// Parse number of bonds.
					auto nbondsStr = xml.attributes().value(QStringLiteral("num"));
					if(!nbondsStr.isEmpty()) {
						bool ok;
						nbonds = nbondsStr.toULongLong(&ok);
						if(!ok)
							throw Exception(tr("GALAMOST file parsing error. Invalid 'num' attribute value in <%1> element: %2").arg(xml.name()).arg(nbondsStr));
						setBondCount(nbonds);
					}
					else {
						throw Exception(tr("GALAMOST file parsing error. Expected 'num' attribute in <%1> element.").arg(xml.name()));
					}
					QString text = xml.readElementText();
					QTextStream stream(&text, QIODevice::ReadOnly | QIODevice::Text);
					PropertyAccess<ParticleIndexPair> topologyProperty(bonds()->createProperty(BondsObject::TopologyProperty, false, initializationHints()));
					PropertyAccess<int> typeProperty = bonds()->createProperty(BondsObject::TypeProperty, false, initializationHints());
					QString typeName;
					for(size_t i = 0; i < nbonds; i++) {
						stream >> typeName >> topologyProperty[i][0] >> topologyProperty[i][1];
						typeProperty[i] = addNamedType(ParticlesObject::OOClass(), typeProperty.buffer(), typeName)->numericId();
						stream.skipWhiteSpace();
					}
					typeProperty.buffer()->sortElementTypesByName();
					typeProperty.reset();
					topologyProperty.reset();
					// Make sure bonds that cross a periodic cell boundary are correctly wrapped around.
					generateBondPeriodicImageProperty();
				}
				else {
					xml.raiseError(tr("Unexpected XML element <%1>.").arg(xml.name().toString()));
				}
			}
		}
	}

	// Handle XML parsing errors.
	if(xml.hasError()) {
		throw Exception(tr("GALAMOST file parsing error on line %1, column %2: %3")
			.arg(xml.lineNumber()).arg(xml.columnNumber()).arg(xml.errorString()));
	}

	// Report number of particles and bonds to the user.
	QString statusString = tr("Number of particles: %1").arg(natoms);
	if(nbonds != 0)
		statusString += tr("\nNumber of bonds: %1").arg(nbonds);
	state().setStatus(statusString);

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

/******************************************************************************
* Parses the contents of an XML element and stores the parsed values in a target property.
******************************************************************************/
PropertyObject* GALAMOSTImporter::FrameLoader::parsePropertyData(QXmlStreamReader& xml, PropertyObject* property)
{
	// Parse number of data elements.
	qlonglong numElements = xml.attributes().value("num").toLongLong();
	if(numElements != property->size()) {
		xml.raiseError(tr("Element count mismatch. Attribute 'num' is %1 but expected %2 data elements.").arg(numElements).arg(property->size()));
		return property;
	}
	
	QString text = xml.readElementText();
	QTextStream stream(&text, QIODevice::ReadOnly | QIODevice::Text);

	if(property->dataType() == PropertyObject::Float) {
		PropertyAccess<FloatType, true> array(property);
		for(FloatType& v : array.range())
			stream >> v;
	}
	else if(property->dataType() == PropertyObject::Int) {
		PropertyAccess<int, true> array(property);
		for(int& v : array.range())
			stream >> v;
	}
	else if(property->dataType() == PropertyObject::Int64) {
		PropertyAccess<qlonglong, true> array(property);
		for(qlonglong& v : array.range())
			stream >> v;
	}
	else OVITO_ASSERT(false);
	return property;
}

}	// End of namespace
