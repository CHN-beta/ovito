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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "LAMMPSBinaryDumpImporter.h"
#include "LAMMPSTextDumpImporter.h"

#include <QtEndian>
#include <QRegularExpression>

namespace Ovito::Particles {

SET_PROPERTY_FIELD_LABEL(LAMMPSBinaryDumpImporter, columnMapping, "File column mapping");

// Helper functions for converting binary floating-point representations used by
// different computing architectures. The Qt library provides similar conversion
// functions, but only for integer types.
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
	inline double fromBigEndian(double source) { return source; }
	inline double fromLittleEndian(double source) {
		quint64 i = qFromLittleEndian(reinterpret_cast<const quint64&>(source));
		return reinterpret_cast<const double&>(i);
	}
#else
	inline double fromBigEndian(double source) {
		quint64 i = qFromBigEndian(reinterpret_cast<const quint64&>(source));
		return reinterpret_cast<const double&>(i);
	}
	inline double fromLittleEndian(double source) { return source; }
#endif


struct LAMMPSBinaryDumpHeader
{
	LAMMPSBinaryDumpHeader() :
		ntimestep(-1), natoms(-1),
		size_one(-1), nchunk(-1) {
			memset(boundaryFlags, 0, sizeof(boundaryFlags));
			memset(tiltFactors, 0, sizeof(tiltFactors));
		}

	qlonglong ntimestep;
	int formatRevision = 0;
	qlonglong natoms;
	int boundaryFlags[3][2];
	double bbox[3][2];
	double tiltFactors[3];
	double simulationTime = std::numeric_limits<double>::lowest();
	QByteArray columnsString;
	int size_one;
	int nchunk;

	enum LAMMPSDataType {
		LAMMPS_SMALLBIG,
		LAMMPS_SMALLSMALL,
		LAMMPS_BIGBIG,
		// End of list marker:
		NUMBER_OF_LAMMPS_DATA_TYPES
	};
	LAMMPSDataType dataType;

	enum LAMMPSEndianess {
		LAMMPS_LITTLE_ENDIAN,
		LAMMPS_BIG_ENDIAN,
		// End of list marker:
		NUMBER_OF_LAMMPS_ENDIAN_TYPES
	};
	LAMMPSEndianess endianess;

	// Parses the LAMMPS dump file header.
	bool parse(QIODevice& input);

	// Parses a 32-bit integer with automatic conversion of endianess depending on current setting.
	int parseInt(QIODevice& input) {
		int val = -1;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));
		if(endianess == LAMMPS_LITTLE_ENDIAN)
			return qFromLittleEndian(val);
		else
			return qFromBigEndian(val);
	}

	// Parses a "big" LAMMPS integer (may be 32 or 64 bit, depending on currently selected data type).
	// A return value of -1 indicates a number overflow.
	qlonglong readBigInt(QIODevice& input) {
		if(dataType == LAMMPS_SMALLSMALL) {
			return parseInt(input);
		}
		else {
			qint64 val;
			input.read(reinterpret_cast<char*>(&val), sizeof(val));
			if(endianess == LAMMPS_LITTLE_ENDIAN)
				val = qFromLittleEndian(val);
			else
				val = qFromBigEndian(val);
			if(val > (qint64)std::numeric_limits<qlonglong>::max())
				return -1;
			else
				return val;
		}
	}

	// Parses a floating-point with automatic conversion of endianess depending on current setting.
	double readDouble(QIODevice& input) {
		double val;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));
		if(endianess == LAMMPS_LITTLE_ENDIAN)
			return fromLittleEndian(val);
		else
			return fromBigEndian(val);
	}
};

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool LAMMPSBinaryDumpImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file for reading.
	std::unique_ptr<QIODevice> device = file.createIODevice();
	if(!device->open(QIODevice::ReadOnly))
		return false;

	LAMMPSBinaryDumpHeader header;
	return header.parse(*device);
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void LAMMPSBinaryDumpImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	// Open input file in binary mode for reading.
	std::unique_ptr<QIODevice> file = fileHandle().createIODevice();
	if(!file->open(QIODevice::ReadOnly))
		throw Exception(tr("Failed to open binary LAMMPS dump file: %1.").arg(file->errorString()));

	setProgressText(tr("Scanning binary LAMMPS dump file %1").arg(fileHandle().toString()));
	setProgressMaximum(file->size());

	Frame frame(fileHandle());
	while(!file->atEnd() && !isCanceled()) {
		frame.byteOffset = file->pos();

		// Parse file header.
		LAMMPSBinaryDumpHeader header;
		if(!header.parse(*file))
			throw Exception(tr("Failed to read binary LAMMPS dump file: Invalid file header."));

		// Skip particle data.
		qint64 filePos = file->pos();
		for(int chunki = 0; chunki < header.nchunk; chunki++) {

			// Read chunk size.
			int n = header.parseInt(*file);
			if(n < 0 || n > header.natoms * header.size_one)
				throw Exception(tr("Invalid data chunk size: %1").arg(n));

			// Skip chunk data.
			filePos += sizeof(n) + n * sizeof(double);
			if(!file->seek(filePos))
				throw Exception(tr("Unexpected end of file."));

			if(!setProgressValue(filePos))
				return;
		}

		// Create a new record for the timestep.
		frame.label = tr("Timestep %1").arg(header.ntimestep);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the file header of a binary LAMMPS dump file.
******************************************************************************/
bool LAMMPSBinaryDumpHeader::parse(QIODevice& input)
{
	// Auto-detection of the LAMMPS data type and architecture used by the dump file:
	// The computer architecture that wrote the file may have been based on little or big endian encoding.
	// Furthermore, LAMMPS may have been configured to use 32-bit or 64-bit integer numbers.
	// We repeatedly try to parse the LAMMPS dump file header with all possible combinations
	// of the data type and endianess settings until we find a combination that
	// leads to reasonable values. These settings will subsequently be used to parse the
	// rest of the dump file.
	qint64 headerPos = input.pos();
	for(int endianessIndex = 0; endianessIndex < NUMBER_OF_LAMMPS_ENDIAN_TYPES; endianessIndex++) {
		endianess = (LAMMPSEndianess)endianessIndex;
		for(int dataTypeIndex = 0; dataTypeIndex < NUMBER_OF_LAMMPS_DATA_TYPES; dataTypeIndex++) {
			dataType = (LAMMPSDataType)dataTypeIndex;
			input.seek(headerPos);

			ntimestep = readBigInt(input);
			if(ntimestep < 0) {

				// Detect newer file format, which is indicated by a negative number of timesteps followed by the magic strings "DUMPATOM" or "DUMPCUSTOM".
				const char MAGIC_STRING_ATOM[] = "DUMPATOM";
				const char MAGIC_STRING_CUSTOM[] = "DUMPCUSTOM";
				if(-ntimestep != sizeof(MAGIC_STRING_ATOM)-1 && -ntimestep != sizeof(MAGIC_STRING_CUSTOM)-1)
					continue;
				
				// Read magic string.
				QByteArray magicString = input.read(-ntimestep);
				if(magicString != MAGIC_STRING_ATOM && magicString != MAGIC_STRING_CUSTOM)
					continue;

				// Read endianess indicator and check if we assumed the right endianess for this file.
				const int ENDIAN = 0x0001;
				if(parseInt(input) != ENDIAN)
					continue;

				// Read format revision number.
				const int FORMAT_REVISION = 0x0002;
				formatRevision = parseInt(input);
				if(formatRevision != FORMAT_REVISION) {
					formatRevision = 0;
					continue;
				}

				// Now read actual number of timesteps.			
				ntimestep = readBigInt(input);
				if(ntimestep < 0)
					continue;
			}
			if(input.atEnd()) return false;

			natoms = readBigInt(input);
			if(natoms < 0 || input.atEnd()) continue;

			qint64 startPos = input.pos();

			// Try parsing the new bounding box format first.
			// It starts with the boundary condition flags.
			int triclinic = -1;
			if(input.read(reinterpret_cast<char*>(&triclinic), sizeof(triclinic)) != sizeof(triclinic))
				continue;
			if(input.read(reinterpret_cast<char*>(boundaryFlags), sizeof(boundaryFlags)) != sizeof(boundaryFlags))
				continue;

			bool isValid = true;
			if(formatRevision < 2) {
				for(int i = 0; i < 3; i++) {
					for(int j = 0; j < 2; j++) {
						if(boundaryFlags[i][j] < 0 || boundaryFlags[i][j] > 3)
							isValid = false;
					}
				}

				if(!isValid) {
					// Try parsing the old bounding box format now.
					input.seek(startPos);
					isValid = true;
					triclinic = -1;
				}
			}

			// Read bounding box.
			for(int i = 0; i < 3; i++) {
				bbox[i][0] = readDouble(input);
				bbox[i][1] = readDouble(input);
				if(bbox[i][0] > bbox[i][1])
					isValid = false;
				for(int j = 0; j < 2; j++) {
					if(!std::isfinite(bbox[i][j]) || bbox[i][j] < -1e9 || bbox[i][j] > 1e9)
						isValid = false;
				}
			}
			if(!isValid || input.atEnd())
				continue;

			// Try parsing shear parameters of triclinic cell.
			if(triclinic != 0) {
				startPos = input.pos();
				if(input.read(reinterpret_cast<char*>(tiltFactors), sizeof(tiltFactors)) != sizeof(tiltFactors))
					continue;
				for(int i = 0; i < 3; i++) {
					if(!std::isfinite(tiltFactors[i]) || tiltFactors[i] < bbox[i][0] - bbox[i][1] || tiltFactors[i] > bbox[i][1] - bbox[i][0])
						isValid = false;
				}
				if(!isValid) {
					input.seek(startPos);
					tiltFactors[0] = tiltFactors[1] = tiltFactors[2] = 0;
				}
			}

			size_one = parseInt(input);
			if(size_one <= 0 || size_one > 40) continue;

			// Newer file format includes units string, columns string and time.
			columnsString.clear();
			if(formatRevision >= 2) {
				// Skip reading unit style.
				int unitStyleLen = parseInt(input);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
				input.skip(unitStyleLen);
#else
				(void)input.read(unitStyleLen);
#endif

				// Parse simulation time.
				char time_flag = 0;
				input.getChar(&time_flag);
				if(time_flag)
					simulationTime = readDouble(input);

				// Parse data columns string.
				int columnsLen = parseInt(input);
				columnsString = input.read(columnsLen);
			}

			nchunk = parseInt(input);
			if(nchunk <= 0 || nchunk > natoms) continue;

			if(!input.atEnd())
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void LAMMPSBinaryDumpImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading binary LAMMPS dump file %1").arg(fileHandle().toString()));

	// Open input file for reading.
	std::unique_ptr<QIODevice> file = fileHandle().createIODevice();
	if(!file->open(QIODevice::ReadOnly))
		throw Exception(tr("Failed to open binary LAMMPS dump file: %1.").arg(file->errorString()));

	// Seek to byte offset.
	if(frame().byteOffset != 0 && !file->seek(frame().byteOffset))
		throw Exception(tr("Failed to read binary LAMMPS dump file: Could not jump to start byte offset."));

	// Parse file header.
	LAMMPSBinaryDumpHeader header;
	if(!header.parse(*file))
		throw Exception(tr("Failed to read binary LAMMPS dump file: Invalid file header."));

	state().setAttribute(QStringLiteral("Timestep"), QVariant::fromValue(header.ntimestep), dataSource());
	if(header.simulationTime != std::numeric_limits<double>::lowest())
		state().setAttribute(QStringLiteral("Time"), QVariant::fromValue(header.simulationTime), dataSource());

	setProgressMaximum(header.natoms);
	setParticleCount(header.natoms);

	// LAMMPS only stores the outer bounding box dimensions of the simulation cell in the dump file.
	// Now calculate the size of the actual triclinic cell.
	Box3 simBox;
	simBox.minc = Point3(header.bbox[0][0], header.bbox[1][0], header.bbox[2][0]);
	simBox.maxc = Point3(header.bbox[0][1], header.bbox[1][1], header.bbox[2][1]);
	simBox.minc.x() -= std::min(std::min(std::min(header.tiltFactors[0], header.tiltFactors[1]), header.tiltFactors[0]+header.tiltFactors[1]), 0.0);
	simBox.maxc.x() -= std::max(std::max(std::max(header.tiltFactors[0], header.tiltFactors[1]), header.tiltFactors[0]+header.tiltFactors[1]), 0.0);
	simBox.minc.y() -= std::min(header.tiltFactors[2], 0.0);
	simBox.maxc.y() -= std::max(header.tiltFactors[2], 0.0);
	simulationCell()->setCellMatrix(AffineTransformation(
			Vector3(simBox.sizeX(), 0, 0),
			Vector3(header.tiltFactors[0], simBox.sizeY(), 0),
			Vector3(header.tiltFactors[1], header.tiltFactors[2], simBox.sizeZ()),
			simBox.minc - Point3::Origin()));
	simulationCell()->setPbcFlags(header.boundaryFlags[0][0] == 0, header.boundaryFlags[1][0] == 0, header.boundaryFlags[2][0] == 0);

	// Set up column-to-property mapping.
	QStringList fileColumnNames;
	if(_columnMapping.empty() && !header.columnsString.isEmpty()) {
		fileColumnNames = FileImporter::splitString(QString::fromLatin1(header.columnsString));
		_columnMapping = LAMMPSTextDumpImporter::generateAutomaticColumnMapping(fileColumnNames);
	}

	// Parse particle data.
	InputColumnReader columnParser(_columnMapping, particles(), initializationHints());
	try {
		QVector<double> chunkData;
		int i = 0;
		for(int chunki = 0; chunki < header.nchunk; chunki++) {

			// Read chunk size.
			int n = header.parseInt(*file);
			if(n < 0 || n > header.natoms * header.size_one)
				throw Exception(tr("Invalid data chunk size: %1").arg(n));
			if(n == 0)
				continue;

			// Read chunk data.
			chunkData.resize(n);
			if(file->read(reinterpret_cast<char*>(chunkData.data()), n * sizeof(double)) != n * sizeof(double))
				throw Exception(tr("Unexpected end of file."));

			// If necessary, convert endianess of floating-point values.
			if(Q_BYTE_ORDER != (header.endianess == LAMMPSBinaryDumpHeader::LAMMPS_BIG_ENDIAN ? Q_BIG_ENDIAN : Q_LITTLE_ENDIAN)) {
				for(double& d : chunkData) {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
					d = fromLittleEndian(d);
#else
					d = fromBigEndian(d);
#endif
				}
			}

			const double* iter = chunkData.constData();
			for(int nChunkAtoms = n / header.size_one; nChunkAtoms--; ++i, iter += header.size_one) {

				// Update progress indicator.
				if(!setProgressValueIntermittent(i)) return;

				try {
					columnParser.readElement(i, iter, header.size_one);
				}
				catch(Exception& ex) {
					throw ex.prependGeneralMessage(tr("Parsing error in LAMMPS binary dump file."));
				}
			}
		}
	}
	catch(Exception& ex) {
		throw ex.prependGeneralMessage(tr("Parsing error at byte offset %1 of binary LAMMPS dump file.").arg(file->pos()));
	}

	// Sort the particle type list since we created particles on the go and their order depends on the occurrence of types in the file.
	columnParser.sortElementTypes();
	columnParser.reset();

	// Determine if particle coordinates are given in reduced form and need to be rescaled to absolute form.
	bool reducedCoordinates = false;
	if(!fileColumnNames.empty()) {
		// If the dump file contains column names, then we can use them to detect 
		// the type of particle coordinates. Reduced coordinates are found in columns
		// "xs, ys, zs" or "xsu, ysu, zsu".
		for(int i = 0; i < (int)_columnMapping.size() && i < fileColumnNames.size(); i++) {
			if(_columnMapping[i].property.type() == ParticlesObject::PositionProperty) {
				reducedCoordinates = (
						fileColumnNames[i] == "xs" || fileColumnNames[i] == "xsu" ||
						fileColumnNames[i] == "ys" || fileColumnNames[i] == "ysu" ||
						fileColumnNames[i] == "zs" || fileColumnNames[i] == "zsu");
				// break; Note: Do not stop the loop here, because the 'Position' particle 
				// property may be associated with several file columns, and it's the last column that 
				// ends up getting imported into OVITO. 
			}
		}
	}
	else {
		// If no column names are available, use the following heuristic:
		// Assume reduced coordinates if all particle coordinates are within the [-0.02,1.02] interval.
		// We allow coordinates to be slightly outside the [0,1] interval, because LAMMPS
		// wraps around particles at the periodic boundaries only occasionally.
		if(ConstPropertyAccess<Point3> posProperty = particles()->getProperty(ParticlesObject::PositionProperty)) {
			// Compute bound box of particle positions.
			Box3 boundingBox;
			boundingBox.addPoints(posProperty);
			// Check if bounding box is inside the (slightly extended) unit cube.
			if(Box3(Point3(FloatType(-0.02)), Point3(FloatType(1.02))).containsBox(boundingBox))
				reducedCoordinates = true;
		}
	}

	if(reducedCoordinates) {
		// Convert all atom coordinates from reduced to absolute (Cartesian) format.
		if(PropertyAccess<Point3> posProperty = particles()->getMutableProperty(ParticlesObject::PositionProperty)) {
			const AffineTransformation simCell = simulationCell()->cellMatrix();
			for(Point3& p : posProperty)
				p = simCell * p;
		}
	}

	// If a "diameter" column was loaded and stored in the "Radius" particle property,
	// we need to divide values by two.
	if(!fileColumnNames.empty()) {
		for(int i = 0; i < (int)_columnMapping.size() && i < fileColumnNames.size(); i++) {
			if(_columnMapping[i].property.type() == ParticlesObject::RadiusProperty && fileColumnNames[i] == "diameter") {
				if(PropertyAccess<FloatType> radiusProperty = particles()->getMutableProperty(ParticlesObject::RadiusProperty)) {
					for(FloatType& r : radiusProperty)
						r /= 2;
				}
				break;
			}
		}
	}

	// Detect when there are more simulation frames following in the file.
	if(!file->atEnd())
		signalAdditionalFrames();

	// Sort particles by ID.
	if(_sortParticles)
		particles()->sortById();
		
	state().setStatus(tr("%1 particles at timestep %2").arg(header.natoms).arg(header.ntimestep));

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

/******************************************************************************
* Inspects the header of the given file and returns the number of file columns.
******************************************************************************/
Future<ParticleInputColumnMapping> LAMMPSBinaryDumpImporter::inspectFileHeader(const Frame& frame)
{
	// Retrieve file.
	return Application::instance()->fileManager()->fetchUrl(dataset()->taskManager(), frame.sourceFile)
		.then([](const FileHandle& fileHandle) {

			// Open input file for reading.
			std::unique_ptr<QIODevice> file = fileHandle.createIODevice();
			if(!file->open(QIODevice::ReadOnly))
				throw Exception(tr("Failed to open binary LAMMPS dump file: %1.").arg(file->errorString()));

			// Parse file header.
			LAMMPSBinaryDumpHeader header;
			if(!header.parse(*file))
				throw Exception(tr("Failed to parse binary LAMMPS dump file: Invalid file header."));

			// Parse column names if it is a modern format file.
			if(!header.columnsString.isEmpty()) {
				QStringList fileColumnNames = FileImporter::splitString(QString::fromLatin1(header.columnsString));
				return LAMMPSTextDumpImporter::generateAutomaticColumnMapping(fileColumnNames);
			}
			else {
				// Return the number of file columns.
				ParticleInputColumnMapping mapping;
				mapping.resize(header.size_one);
				return mapping;
			}
		});
}

/******************************************************************************
 * Saves the class' contents to the given stream.
 *****************************************************************************/
void LAMMPSBinaryDumpImporter::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const
{
	ParticleImporter::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x02);
	stream.endChunk();
}

/******************************************************************************
 * Loads the class' contents from the given stream.
 *****************************************************************************/
void LAMMPSBinaryDumpImporter::loadFromStream(ObjectLoadStream& stream)
{
	ParticleImporter::loadFromStream(stream);

	// For backward compatibility with OVITO 3.1:
	if(stream.expectChunkRange(0x00, 0x02) == 0x01) {
		stream >> _columnMapping.mutableValue();
	}
	stream.closeChunk();
}

}	// End of namespace
