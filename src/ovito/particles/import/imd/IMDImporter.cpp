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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/InputColumnMapping.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "IMDImporter.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(IMDImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool IMDImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Read first header line.
	stream.readLine(1024);

	// Read first line.
	return stream.lineStartsWith("#F A ");
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void IMDImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading IMD file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Read first header line.
	stream.readLine();
	if(!stream.lineStartsWith("#F"))
		throw Exception(tr("Not an IMD atom file."));
	QStringList tokens = FileImporter::splitString(stream.lineString());
	if(tokens.size() < 2 || tokens[1] != "A")
		throw Exception(tr("Not an IMD atom file in ASCII format."));

	ParticleInputColumnMapping columnMapping;
	AffineTransformation cell = AffineTransformation::Identity();

	// Read remaining header lines
	for(;;) {
		stream.readLine();
		if(stream.line()[0] != '#')
			throw Exception(tr("Invalid header in IMD atom file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
		if(stream.line()[1] == '#') continue;
		else if(stream.line()[1] == 'E') break;
		else if(stream.line()[1] == 'C') {
			QStringList tokens = FileImporter::splitString(stream.lineString());
			columnMapping.resize(qMax(0, tokens.size() - 1));
			for(int t = 1; t < tokens.size(); t++) {
				const QString& token = tokens[t];
				int columnIndex = t - 1;
				columnMapping[columnIndex].columnName = token;
				if(token == "mass") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::MassProperty);
				else if(token == "type") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::TypeProperty);
				else if(token == "number") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::IdentifierProperty);
				else if(token == "x") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::PositionProperty, 0);
				else if(token == "y") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::PositionProperty, 1);
				else if(token == "z") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::PositionProperty, 2);
				else if(token == "vx") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::VelocityProperty, 0);
				else if(token == "vy") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::VelocityProperty, 1);
				else if(token == "vz") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::VelocityProperty, 2);
				else if(token == "Epot") columnMapping.mapStandardColumn(columnIndex, ParticlesObject::PotentialEnergyProperty);
				else {
					bool isStandardProperty = false;
					const auto& standardPropertyList = ParticlesObject::OOClass().standardPropertyIds();
					QRegularExpression specialCharacters(QStringLiteral("[^A-Za-z\\d_]"));
					for(int id : standardPropertyList) {
						for(size_t component = 0; component < ParticlesObject::OOClass().standardPropertyComponentCount(id); component++) {
							QString columnName = ParticlesObject::OOClass().standardPropertyName(id);
							columnName.remove(specialCharacters);
							const QStringList& componentNames = ParticlesObject::OOClass().standardPropertyComponentNames(id);
							if(!componentNames.empty()) {
								QString componentName = componentNames[component];
								componentName.remove(specialCharacters);
								columnName += QChar('.');
								columnName += componentName;
							}
							if(columnName == token) {
								columnMapping.mapStandardColumn(columnIndex, (ParticlesObject::Type)id, component);
								isStandardProperty = true;
								break;
							}
						}
						if(isStandardProperty) break;
					}
					if(!isStandardProperty)
						columnMapping.mapCustomColumn(columnIndex, token, PropertyObject::Float);
				}
			}
		}
		else if(stream.line()[1] == 'X') {
			if(sscanf(stream.line() + 2, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,0), &cell(1,0), &cell(2,0)) != 3)
				throw Exception(tr("Invalid simulation cell bounds in line %1 of IMD file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		}
		else if(stream.line()[1] == 'Y') {
			if(sscanf(stream.line() + 2, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,1), &cell(1,1), &cell(2,1)) != 3)
				throw Exception(tr("Invalid simulation cell bounds in line %1 of IMD file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		}
		else if(stream.line()[1] == 'Z') {
			if(sscanf(stream.line() + 2, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,2), &cell(1,2), &cell(2,2)) != 3)
				throw Exception(tr("Invalid simulation cell bounds in line %1 of IMD file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		}
		else throw Exception(tr("Invalid header line key in IMD atom file (line %2).").arg(stream.lineNumber()));
	}
	simulationCell()->setCellMatrix(cell);

	// Save file position.
	qint64 headerOffset = stream.byteOffset();
	int headerLineNumber = stream.lineNumber();

	// Count the number of atoms (=lines) in the input file.
	size_t numAtoms = 0;
	while(!stream.eof()) {
		if(stream.readLine()[0] == '\0') break;
		numAtoms++;

		if(isCanceled())
			return;
	}
	setParticleCount(numAtoms);
	setProgressMaximum(numAtoms);

	// Jump back to beginning of atom list.
	stream.seek(headerOffset, headerLineNumber);

	// Parse data columns.
	InputColumnReader columnParser(columnMapping, particles());
	for(size_t i = 0; i < numAtoms; i++) {
		if(!setProgressValueIntermittent(i)) return;
		try {
			columnParser.readElement(i, stream.readLine());
		}
		catch(Exception& ex) {
			throw ex.prependGeneralMessage(tr("Parsing error in line %1 of IMD file.").arg(headerLineNumber + i));
		}
	}
	columnParser.reset();

	// Sort particles by ID if requested.
	if(_sortParticles)
		particles()->sortById();

	state().setStatus(tr("Number of particles: %1").arg(numAtoms));

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
