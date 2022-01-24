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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/NumberParsing.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "POSCARImporter.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(POSCARImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool POSCARImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Skip comment line
	stream.readLine();

	// Read global scaling factor
	double scaling_factor;
	stream.readLine();
	if(stream.eof() || sscanf(stream.line(), "%lg", &scaling_factor) != 1 || scaling_factor <= 0)
		return false;

	// Read cell matrix
	char c;
	for(int i = 0; i < 3; i++) {
		double x,y,z;
		if(sscanf(stream.readLine(), "%lg %lg %lg %c", &x, &y, &z, &c) != 3 || stream.eof())
			return false;
	}

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	// Parse number of atoms per type.
	int nAtomTypes = 0;
	for(int i = 0; i < 2; i++) {
		stream.readLine();
		QStringList tokens = FileImporter::splitString(stream.lineString());
		if(i == 0) nAtomTypes = tokens.size();
		else if(nAtomTypes != tokens.size())
			return false;
		int n = 0;
		for(const QString& token : tokens) {
			bool ok;
			n += token.toInt(&ok);
		}
		if(n > 0)
			return true;
	}

	return false;
}

/******************************************************************************
* Determines whether the input file should be scanned to discover all contained frames.
******************************************************************************/
bool POSCARImporter::shouldScanFileForFrames(const QUrl& sourceUrl) const
{
	return sourceUrl.fileName().contains(QStringLiteral("XDATCAR"));
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void POSCARImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Scanning file %1").arg(fileHandle().toString()));
	setProgressMaximum(stream.underlyingSize());

	int frameNumber = 0;
	QStringList atomTypeNames;
	QVector<int> atomCounts;
	QString filename = fileHandle().sourceUrl().fileName();

	// Read frames.
	Frame frame(fileHandle());
	while(!stream.eof() && !isCanceled()) {
		frame.byteOffset = stream.byteOffset();
		frame.lineNumber = stream.lineNumber();
		frame.parserData = 1;
		frame.label = QString("%1 (Frame %2)").arg(filename).arg(frameNumber++);

		// Read comment line
		stream.readLine();
		if(frameNumber == 1 || !stream.lineStartsWith("Direct configuration=")) {
			for(int headerIndex = 0; headerIndex < 2; headerIndex++) {

				// Read scaling factor
				FloatType scaling_factor = 0;
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING, &scaling_factor) != 1 || scaling_factor <= 0)
					throw Exception(tr("Invalid scaling factor in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

				// Read cell matrix
				AffineTransformation cell;
				for(size_t i = 0; i < 3; i++) {
					if(sscanf(stream.readLine(),
							FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
							&cell(0,i), &cell(1,i), &cell(2,i)) != 3 || cell.column(i) == Vector3::Zero())
						throw Exception(tr("Invalid cell vector in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
				}

				// Parse atom type names and atom type counts.
				atomTypeNames.clear();
				atomCounts.clear();
				parseAtomTypeNamesAndCounts(stream, atomTypeNames, atomCounts);

				auto byteOffset = stream.byteOffset();
				auto lineNumber = stream.lineNumber();

				// Read in 'Selective dynamics' flag
				// and coordinate system type.
				stream.readLine();

				if(frameNumber == 1 && headerIndex == 0 && stream.lineStartsWith("energy calculation")) {
					frame.byteOffset = byteOffset;
					frame.lineNumber = lineNumber;
					continue;
				}

				if(stream.line()[0] == 'S' || stream.line()[0] == 's')
					stream.readLine();

				break;
			}
		}

		// Read atoms coordinates list.
		for(int acount : atomCounts) {
			for(int i = 0; i < acount; i++) {
				Point3 p;
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						&p.x(), &p.y(), &p.z()) != 3)
					throw Exception(tr("Invalid atomic coordinates in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			}
		}
		frames.push_back(frame);

		if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
			return;
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void POSCARImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading VASP file %1").arg(fileHandle().toString()));

	// Jump to requested animation frame.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Read comment line.
	stream.readLine();
	QString trimmedComment = stream.lineString().trimmed();
	bool singleHeaderFile = false;
	if(frame().byteOffset != 0 && trimmedComment.startsWith(QStringLiteral("Direct configuration="))) {
		// Jump back to beginning of file.
		stream.seek(0);
		singleHeaderFile = true;
		stream.readLine();
		trimmedComment = stream.lineString().trimmed();
	}
	if(!trimmedComment.isEmpty())
		state().setAttribute(QStringLiteral("Comment"), QVariant::fromValue(trimmedComment), dataSource());

	// Read global scaling factor
	FloatType scaling_factor = 0;
	if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING, &scaling_factor) != 1 || scaling_factor <= 0)
		throw Exception(tr("Invalid scaling factor in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

	// Read cell matrix
	AffineTransformation cell = AffineTransformation::Identity();
	for(size_t i = 0; i < 3; i++) {
		if(sscanf(stream.readLine(),
				FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
				&cell(0,i), &cell(1,i), &cell(2,i)) != 3 || cell.column(i) == Vector3::Zero())
			throw Exception(tr("Invalid cell vector in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	}
	cell = cell * scaling_factor;
	simulationCell()->setCellMatrix(cell);

	// Parse atom type names and atom type counts.
	QStringList atomTypeNames;
	QVector<int> atomCounts;
	parseAtomTypeNamesAndCounts(stream, atomTypeNames, atomCounts);
	int totalAtomCount = std::accumulate(atomCounts.begin(), atomCounts.end(), 0);
	if(totalAtomCount <= 0)
		throw Exception(tr("Invalid atom counts in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	setParticleCount(totalAtomCount);

	if(atomTypeNames.empty() && atomCounts.size() >= 1) {
		// The file might be in VASP 4.x format, which is the format written by ASE's write_vasp() function.
		// Files of this format contain the chemical element names in the comment line (very first line of the file).
		QRegularExpression ws_re(QStringLiteral("\\s+"));
		QStringList tokens = FileImporter::splitString(trimmedComment);
		// Number of tokens must match the number of atom types.
		if(tokens.size() == atomCounts.size()) {
			// Each token should be a one- or two-letter chemical symbol.
			if(std::all_of(tokens.cbegin(), tokens.cend(), [](const QString& s) { return s.size() <= 2 && s.at(0).isLetter(); })) {
				atomTypeNames = std::move(tokens);
			}
		}
	}

	if(frame().byteOffset != 0 && singleHeaderFile)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Read in 'Selective dynamics' flag
	stream.readLine();
	if(stream.line()[0] == 'S' || stream.line()[0] == 's')
		stream.readLine();

	// Parse coordinate system.
	bool isCartesian = false;
	if(stream.line()[0] == 'C' || stream.line()[0] == 'c' || stream.line()[0] == 'K' || stream.line()[0] == 'k')
		isCartesian = true;

	// Create the particle properties.
	PropertyAccess<Point3> posProperty = particles()->createProperty(ParticlesObject::PositionProperty);
	PropertyAccess<int> typeProperty = particles()->createProperty(ParticlesObject::TypeProperty);

	// Read atom coordinates.
	Point3* p = posProperty.begin();
	int* a = typeProperty.begin();
	for(int atype = 1; atype <= atomCounts.size(); atype++) {
		int typeId = atype;
		if(atomTypeNames.size() == atomCounts.size() && atomTypeNames[atype-1].isEmpty() == false)
			typeId = addNamedType(ParticlesObject::OOClass(), typeProperty.buffer(), atomTypeNames[atype-1])->numericId();
		else
			addNumericType(ParticlesObject::OOClass(), typeProperty.buffer(), atype, {});
		for(int i = 0; i < atomCounts[atype-1]; i++, ++p, ++a) {
			*a = typeId;
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&p->x(), &p->y(), &p->z()) != 3)
				throw Exception(tr("Invalid atomic coordinates in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(!isCartesian)
				*p = cell * (*p);
			else
				*p = (*p) * scaling_factor;
		}
	}

	QString statusString = tr("%1 atoms").arg(totalAtomCount);

	// Parse optional atomic velocity vectors or CHGCAR electron density data.
	// Do this only for the first frame and if it is not a XDATCAR file.
	if(frame().byteOffset == 0 && frame().parserData == 0) {
		if(!stream.eof())
			stream.readLineTrimLeft();
		if(!stream.eof() && stream.line()[0] > ' ') {
			isCartesian = false;
			if(stream.line()[0] == 'C' || stream.line()[0] == 'c' || stream.line()[0] == 'K' || stream.line()[0] == 'k')
				isCartesian = true;

			// Read atomic velocities.
			PropertyAccess<Vector3> velocityProperty = particles()->createProperty(ParticlesObject::VelocityProperty);
			Vector3* v = velocityProperty.begin();
			for(int atype = 1; atype <= atomCounts.size(); atype++) {
				for(int i = 0; i < atomCounts[atype-1]; i++, ++v) {
					if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
							&v->x(), &v->y(), &v->z()) != 3)
						throw Exception(tr("Invalid atomic velocity vector in line %1 of VASP file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					if(!isCartesian)
						*v = cell * (*v);
				}
			}
		}
		else if(!stream.eof()) {
			// Parse charge density grid.
			statusString += readDensityGrid(stream);
		}
	}
	posProperty.reset();
	typeProperty.reset();

	state().setStatus(statusString);

	// Generate ad-hoc bonds between atoms based on their van der Waals radii.
	if(_generateBonds)
		generateBonds();
	else
		setBondCount(0);

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

/******************************************************************************
* Parses the list of atom types from the POSCAR file.
******************************************************************************/
void POSCARImporter::parseAtomTypeNamesAndCounts(CompressedTextReader& stream, QStringList& atomTypeNames, QVector<int>& atomCounts)
{
	for(int i = 0; i < 2; i++) {
		stream.readLine();
		QStringList tokens = FileImporter::splitString(stream.lineString());
		// Try to convert string tokens to integers.
		atomCounts.clear();
		bool ok = true;
		for(const QString& token : tokens) {
			atomCounts.push_back(token.toInt(&ok));
			if(!ok) {
				// If the casting to integer fails, then the current line contains the element names.
				// Try it again with the next line.
				atomTypeNames = tokens;
				break;
			}
		}
		if(ok)
			break;
		if(i == 1)
			throw Exception(tr("Invalid atom counts (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
	}
}

/******************************************************************************
* Parses a charge density grid.
******************************************************************************/
QString POSCARImporter::FrameLoader::readDensityGrid(CompressedTextReader& stream)
{
	QString statusString;

	// Parse grid dimensions.
	VoxelGrid::GridDimensions gridSize;
	if(sscanf(stream.readLine(), "%zu %zu %zu", &gridSize[0], &gridSize[1], &gridSize[2]) != 3 || gridSize[0] == 0 || gridSize[1] == 0 || gridSize[2] == 0)
		return {};

	// Create the voxel grid data object.
	VoxelGrid* voxelGrid = state().getMutableObject<VoxelGrid>();
	if(!voxelGrid) {
		voxelGrid = state().createObject<VoxelGrid>(dataSource(), tr("Charge density"));
		voxelGrid->visElement()->setEnabled(false);
		voxelGrid->visElement()->setTitle(voxelGrid->title());
		voxelGrid->visElement()->freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(ActiveObject::isEnabled), SHADOW_PROPERTY_FIELD(ActiveObject::title)});
	}
	voxelGrid->setDomain(simulationCell());
	voxelGrid->setIdentifier(QStringLiteral("charge-density"));
	voxelGrid->setShape(gridSize);
	voxelGrid->setContent(gridSize[0] * gridSize[1] * gridSize[2], {});

	// Parse spin up + spin down density.
	if(!readFieldQuantity(stream, voxelGrid, tr("Charge density")))
		return {};
	statusString += tr("\nCharge density grid: %1 x %2 x %3").arg(gridSize[0]).arg(gridSize[1]).arg(gridSize[2]);

	// Look for spin up - spin down density.
	PropertyPtr magnetizationDensityX;
	while(!stream.eof()) {
		if(sscanf(stream.readLine(), "%zu %zu %zu", &gridSize[0], &gridSize[1], &gridSize[2]) == 3) {
			if(gridSize != voxelGrid->shape())
				throw Exception(tr("Inconsistent voxel grid dimensions in line %1").arg(stream.lineNumber()));
			magnetizationDensityX = readFieldQuantity(stream, voxelGrid, tr("Magnetization density"));
			if(!magnetizationDensityX) return {};
			statusString += tr("\nMagnetization density grid: %1 x %2 x %3").arg(gridSize[0]).arg(gridSize[1]).arg(gridSize[2]);
			break;
		}
	}

	// Look for more vector components in case file contains vectorial magnetization.
	PropertyPtr magnetizationDensityY;
	PropertyPtr magnetizationDensityZ;
	while(!stream.eof()) {
		if(sscanf(stream.readLine(), "%zu %zu %zu", &gridSize[0], &gridSize[1], &gridSize[2]) == 3) {
			if(gridSize != voxelGrid->shape())
				throw Exception(tr("Inconsistent voxel grid dimensions in line %1").arg(stream.lineNumber()));
			magnetizationDensityY = readFieldQuantity(stream, voxelGrid, tr("Magnetization density"));
			if(!magnetizationDensityY) return {};
			break;
		}
	}
	while(!stream.eof()) {
		if(sscanf(stream.readLine(), "%zu %zu %zu", &gridSize[0], &gridSize[1], &gridSize[2]) == 3) {
			if(gridSize != voxelGrid->shape())
				throw Exception(tr("Inconsistent voxel grid dimensions in line %1").arg(stream.lineNumber()));
			magnetizationDensityZ = readFieldQuantity(stream, voxelGrid, tr("Magnetization density"));
			if(!magnetizationDensityZ) return {};
			break;
		}
	}

	if(magnetizationDensityX && magnetizationDensityY && magnetizationDensityZ) {
		PropertyAccess<FloatType,true> vectorMagnetization = voxelGrid->createProperty(tr("Magnetization density"), PropertyObject::Float, 3, DataBuffer::NoFlags, QStringList() << "X" << "Y" << "Z");
		boost::copy(ConstPropertyAccess<FloatType>(magnetizationDensityX), vectorMagnetization.componentRange(0).begin());
		boost::copy(ConstPropertyAccess<FloatType>(magnetizationDensityY), vectorMagnetization.componentRange(1).begin());
		boost::copy(ConstPropertyAccess<FloatType>(magnetizationDensityZ), vectorMagnetization.componentRange(2).begin());
	}

	voxelGrid->verifyIntegrity();
	return statusString;
}

/******************************************************************************
* Parses the values of one field quantity.
******************************************************************************/
PropertyObject* POSCARImporter::FrameLoader::readFieldQuantity(CompressedTextReader& stream, VoxelGrid* grid, const QString& name)
{
	PropertyAccess<FloatType,true> fieldArray = grid->createProperty(name, PropertyObject::Float);
	const char* s = stream.readLine();
	FloatType* data = fieldArray.begin();
	setProgressMaximum(fieldArray.size());
	FloatType cellVolume = std::abs(simulationCell()->cellMatrix().determinant());
	for(size_t i = 0; i < fieldArray.size(); i++, ++data) {
		const char* token;
		for(;;) {
			while(*s == ' ' || *s == '\t') ++s;
			token = s;
			while(*s > ' ' || *s < 0) ++s;
			if(s != token) break;
			s = stream.readLine();
		}
		if(!parseFloatType(token, s, *data))
			throw Exception(tr("Invalid value in charge density section of VASP file (line %1): \"%2\"").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(token, s - token)));
		*data /= cellVolume;
		if(*s != '\0')
			s++;

		if(!setProgressValueIntermittent(i)) return nullptr;
	}
	return fieldArray.buffer();
}

}	// End of namespace
