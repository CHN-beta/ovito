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
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/InputColumnMapping.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/FileManager.h>
#include <ovito/core/app/Application.h>
#include "OXDNAImporter.h"
#include "NucleotidesVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(OXDNAImporter);
DEFINE_PROPERTY_FIELD(OXDNAImporter, topologyFileUrl);
SET_PROPERTY_FIELD_LABEL(OXDNAImporter, topologyFileUrl, "Topology file");

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool OXDNAImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file for reading.
	CompressedTextReader stream(file);

	// Check for a valid "t = ..." line.
	FloatType t;
	if(sscanf(stream.readLineTrimLeft(128), "t = " FLOATTYPE_SCANF_STRING, &t) != 1) 
		return false; 

	// Check for a valid "b = ..." line.
	Vector3 b;
	if(sscanf(stream.readLineTrimLeft(128), "b = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &b.x(), &b.y(), &b.z()) != 3) 
		return false; 

	// Check for a valid "E = ..." line.
	FloatType Etot, U, K;
	if(sscanf(stream.readLineTrimLeft(128), "E = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &Etot, &U, &K) != 3) 
		return false; 

	return true;
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void OXDNAImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Scanning file %1").arg(fileHandle().toString()));
	setProgressMaximum(stream.underlyingSize());

	Frame frame(fileHandle());
	QString filename = fileHandle().sourceUrl().fileName();
	int frameNumber = 0;

	frame.byteOffset = stream.byteOffset();
	frame.lineNumber = stream.lineNumber();
	while(!stream.eof() && !isCanceled()) {

		// Check for a valid "t = ..." line.
		FloatType t;
		if(frameNumber == 0) stream.readLine();
		if(sscanf(stream.line(), " t = " FLOATTYPE_SCANF_STRING, &t) != 1) 
			break; 

		// Check for a valid "b = ..." line.
		Vector3 b;
		if(sscanf(stream.readLineTrimLeft(), "b = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &b.x(), &b.y(), &b.z()) != 3) 
			break; 

		// Check for a valid "E = ..." line.
		FloatType Etot, U, K;
		if(sscanf(stream.readLineTrimLeft(), "E = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &Etot, &U, &K) != 3) 
			break; 

		// Create a new record for the time step.
		frame.label = tr("%1 (Frame %2)").arg(filename).arg(frameNumber++);
		frames.push_back(frame);

		// Skip nucleotide lines.
		while(!stream.eof()) {
			frame.byteOffset = stream.byteOffset();
			frame.lineNumber = stream.lineNumber();
			stream.readLine();
			if(stream.lineStartsWith("t", true))
				break;
			if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
				return;
		}
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void OXDNAImporter::FrameLoader::loadFile()
{
	// Locate the topology file. 
	QUrl topoFileUrl = _userSpecifiedTopologyUrl;
	if(!topoFileUrl.isValid()) {
		// If no explicit path for the topology file was specified by the user, try to infer it from the
		// base name of the configuration file. Replace original suffix of configuration file name with ".top".
		topoFileUrl = frame().sourceFile;
		QFileInfo filepath(topoFileUrl.path());
		topoFileUrl.setPath(filepath.path() + QStringLiteral("/") + filepath.completeBaseName() + QStringLiteral(".top"));

		// Check if the topology file exists.
		if(!topoFileUrl.isValid() || (topoFileUrl.isLocalFile() && !QFileInfo::exists(topoFileUrl.toLocalFile()))) {
			if(executionContext() == ExecutionContext::Interactive) {
				throw Exception(tr("Could not locate corresponding topology file for oxDNA configuration file '%1'.\n"
					"Tried automatically inferred path:\n\n%2\n\nBut the path does not exist. Please pick the topology file manually.")
						.arg(frame().sourceFile.fileName())
						.arg(topoFileUrl.toLocalFile()));
			}
			else {
				throw Exception(tr("Could not locate corresponding topology file for oxDNA configuration file '%1'. "
					"Tried inferred path '%2', but the file does not exist. Please specify the path of the topology file explicitly.")
						.arg(frame().sourceFile.fileName())
						.arg(topoFileUrl.toLocalFile()));
			}
		}
	}

	// Fetch the oxDNA topology file if it is stored on a remote location.
	SharedFuture<FileHandle> localTopologyFileFuture = Application::instance()->fileManager()->fetchUrl(*taskManager(), topoFileUrl);
	if(!waitForFuture(localTopologyFileFuture))
		return;

	// Open oxDNA topology file for reading.
	CompressedTextReader topoStream(localTopologyFileFuture.result());
	beginProgressSubSteps(2);
	setProgressText(tr("Reading oxDNA topology file %1").arg(localTopologyFileFuture.result().toString()));

	// Parse number of nucleotides and number of strands.
	unsigned long long numNucleotidesLong;
	int numStrands;
	if(sscanf(topoStream.readLine(), "%llu %i", &numNucleotidesLong, &numStrands) != 2)
		throw Exception(tr("Invalid number of nucleotides or strands in line %1 of oxDNA topology file: %2").arg(topoStream.lineNumber()).arg(topoStream.lineString().trimmed()));
	setParticleCount(numNucleotidesLong);

	// Create a special visual element for rendering the nuclotides.
	if(!dynamic_object_cast<NucleotidesVis>(particles()->visElement()))
		particles()->setVisElement(OORef<NucleotidesVis>::create(dataset(), executionContext()));

	// Define nucleobase types.
	PropertyAccess<int> baseProperty = particles()->createProperty(ParticlesObject::NucleobaseTypeProperty, false, executionContext());
	addNumericType(ParticlesObject::OOClass(), baseProperty.buffer(), 1, QStringLiteral("T"));
	addNumericType(ParticlesObject::OOClass(), baseProperty.buffer(), 2, QStringLiteral("C"));
	addNumericType(ParticlesObject::OOClass(), baseProperty.buffer(), 3, QStringLiteral("G"));
	addNumericType(ParticlesObject::OOClass(), baseProperty.buffer(), 4, QStringLiteral("A"));

	// Define strands list.
	PropertyAccess<int> strandsProperty = particles()->createProperty(ParticlesObject::DNAStrandProperty, false, executionContext());
	for(int i = 1; i <= numStrands; i++)
		addNumericType(ParticlesObject::OOClass(), strandsProperty.buffer(), i, {});

	// The list of bonds between nucleotides.
	std::vector<ParticleIndexPair> bonds;
	bonds.reserve(numNucleotidesLong);

	// Parse the nucleotides list in the topology file.
	setProgressMaximum(numNucleotidesLong);
	int* baseTypeIter = baseProperty.begin();
	int* strandId = strandsProperty.begin();
	for(size_t i = 0; i < numNucleotidesLong; i++, ++strandId) {
		if(!setProgressValueIntermittent(i)) return;

		char baseName[32];
		qlonglong neighbor1, neighbor2;
		if(sscanf(topoStream.readLine(), "%u %31s %lld %lld", strandId, baseName, &neighbor1, &neighbor2) != 4)
			throw Exception(tr("Invalid nucleotide specification in line %1 of oxDNA topology file: %2").arg(topoStream.lineNumber()).arg(topoStream.lineString()));

		if(*strandId < 1 || *strandId > numStrands)
			throw Exception(tr("Strand ID %2 in line %1 of oxDNA topology file is out of range.").arg(topoStream.lineNumber()).arg(*strandId));
	
		if(neighbor1 < -1 || neighbor1 >= (qlonglong)numNucleotidesLong)
			throw Exception(tr("3' neighbor %2 in line %1 of oxDNA topology file is out of range.").arg(topoStream.lineNumber()).arg(neighbor1));

		if(neighbor2 < -1 || neighbor2 >= (qlonglong)numNucleotidesLong)
			throw Exception(tr("5' neighbor %2 in line %1 of oxDNA topology file is out of range.").arg(topoStream.lineNumber()).arg(neighbor2));

		if(neighbor2 != -1)
			bonds.push_back({(qlonglong)i, neighbor2});

		*baseTypeIter++ = addNamedType(ParticlesObject::OOClass(), baseProperty.buffer(), QLatin1String(baseName))->numericId();
	}

	// Create and fill bonds topology storage.
	setBondCount(bonds.size());
	PropertyAccess<ParticleIndexPair> bondTopologyProperty = this->bonds()->createProperty(BondsObject::TopologyProperty, false, executionContext());
	boost::copy(bonds, bondTopologyProperty.begin());

	// Open oxDNA configuration file for reading.
	CompressedTextReader stream(fileHandle());
	nextProgressSubStep();
	setProgressText(tr("Reading oxDNA file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Parse the 1st line: "t = T".
	FloatType simulationTime;
	if(sscanf(stream.readLineTrimLeft(), "t = " FLOATTYPE_SCANF_STRING, &simulationTime) != 1) 
		throw Exception(tr("Invalid header format encountered in line %1 of oxDNA configuration file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	state().setAttribute(QStringLiteral("Time"), QVariant::fromValue(simulationTime), dataSource());

	// Parse the 2nd line: "b = Lx Ly Lz".
	AffineTransformation cellMatrix = AffineTransformation::Identity();
	if(sscanf(stream.readLineTrimLeft(), "b = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cellMatrix(0,0), &cellMatrix(1,1), &cellMatrix(2,2)) != 3) 
		throw Exception(tr("Invalid header format encountered in line %1 of oxDNA configuration file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	cellMatrix.translation() = Vector3(-0.5 * cellMatrix(0,0), -0.5 * cellMatrix(1,1), -0.5 * cellMatrix(2,2));
	simulationCell()->setCellMatrix(cellMatrix);

	// Parse the 3rd line: "E = Etot U K".
	FloatType Etot, U, K;
	if(sscanf(stream.readLineTrimLeft(), "E = " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &Etot, &U, &K) != 3) 
		throw Exception(tr("Invalid header format encountered in line %1 of oxDNA configuration file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	state().setAttribute(QStringLiteral("Etot"), QVariant::fromValue(Etot), dataSource());
	state().setAttribute(QStringLiteral("U"), QVariant::fromValue(U), dataSource());
	state().setAttribute(QStringLiteral("K"), QVariant::fromValue(K), dataSource());

	// Define the column data layout in the input file to be parsed.
	ParticleInputColumnMapping columnMapping;
	columnMapping.resize(15);
	columnMapping.mapStandardColumn(0, ParticlesObject::PositionProperty, 0);
	columnMapping.mapStandardColumn(1, ParticlesObject::PositionProperty, 1);
	columnMapping.mapStandardColumn(2, ParticlesObject::PositionProperty, 2);
	columnMapping.mapStandardColumn(3, ParticlesObject::NucleotideAxisProperty, 0);
	columnMapping.mapStandardColumn(4, ParticlesObject::NucleotideAxisProperty, 1);
	columnMapping.mapStandardColumn(5, ParticlesObject::NucleotideAxisProperty, 2);
	columnMapping.mapStandardColumn(6, ParticlesObject::NucleotideNormalProperty, 0);
	columnMapping.mapStandardColumn(7, ParticlesObject::NucleotideNormalProperty, 1);
	columnMapping.mapStandardColumn(8, ParticlesObject::NucleotideNormalProperty, 2);
	columnMapping.mapStandardColumn(9, ParticlesObject::VelocityProperty, 0);
	columnMapping.mapStandardColumn(10, ParticlesObject::VelocityProperty, 1);
	columnMapping.mapStandardColumn(11, ParticlesObject::VelocityProperty, 2);
	columnMapping.mapStandardColumn(12, ParticlesObject::AngularVelocityProperty, 0);
	columnMapping.mapStandardColumn(13, ParticlesObject::AngularVelocityProperty, 1);
	columnMapping.mapStandardColumn(14, ParticlesObject::AngularVelocityProperty, 2);

	// Parse data table.
	InputColumnReader columnParser(columnMapping, particles(), executionContext(), false);
	for(size_t i = 0; i < numNucleotidesLong; i++) {
		if(!setProgressValueIntermittent(i)) return;
		try {
			columnParser.readElement(i, stream.readLine());
		}
		catch(Exception& ex) {
			throw ex.prependGeneralMessage(tr("Parsing error in line %1 of oxDNA configuration file (nucleotide index %2).").arg(stream.lineNumber()).arg(i));
		}
	}
	columnParser.reset();

	// Detect if there are more simulation frames following in the file.
	if(!stream.eof())
		signalAdditionalFrames();

	// Displace particle positions. oxDNA stores center of mass coordinates, but OVITO expects particle coordinates to be backbone sphere centers.
	PropertyAccess<Point3> centerOfMassPositionsArray = particles()->createProperty(QStringLiteral("Center Of Mass"), PropertyObject::Float, 3, 0, false, QStringList() << QStringLiteral("X") << QStringLiteral("Y") << QStringLiteral("Z"));
	PropertyAccess<Point3> basePositionsArray = particles()->createProperty(QStringLiteral("Base Position"), PropertyObject::Float, 3, 0, false, QStringList() << QStringLiteral("X") << QStringLiteral("Y") << QStringLiteral("Z"));
	PropertyAccess<Point3> positionsArray = particles()->getMutableProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<Vector3> axisVectorArray = particles()->expectProperty(ParticlesObject::NucleotideAxisProperty);
	for(size_t i = 0; i < numNucleotidesLong; i++) {
		centerOfMassPositionsArray[i] = positionsArray[i];
		positionsArray[i] -= 0.4 * axisVectorArray[i];
		basePositionsArray[i] = centerOfMassPositionsArray[i] + 0.4 * axisVectorArray[i];
	}

	state().setStatus(tr("%1 nucleotides\n%2 strands").arg(numNucleotidesLong).arg(numStrands));

	endProgressSubSteps();

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
}	// End of namespace
