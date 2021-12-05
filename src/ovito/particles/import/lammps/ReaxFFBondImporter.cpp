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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/FileManager.h>
#include "ReaxFFBondImporter.h"

namespace Ovito::Particles {


/******************************************************************************
* Checks if the given file has a format that can be read by this importer.
******************************************************************************/
bool ReaxFFBondImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Parse the first couple of lines only.
	for(int i = 0; i < 20 && !stream.eof(); i++) {
		const char* line = stream.readLineTrimLeft(1024);

		// Skip comment lines starting with '#'.
		if(line[0] == '#')
			continue;

		// Parse atom id, atom type and number of bonds.
		qlonglong atomId;
		int atomType;
		int numBonds;
		int nread;
		if(sscanf(line, "%lld %d %d%n", &atomId, &atomType, &numBonds, &nread) != 3)
			return false;
		if(atomId < 1 || atomType < 1 || atomType > 1000 || numBonds < 0 || numBonds > 100)
			return false;
		line += nread;

		// Parse the neighbor atom id of each bond.
		for(int j = 0; j < numBonds; j++) {
			if(sscanf(line, "%lld%n", &atomId, &nread) != 1)
				return false;
			if(atomId < 1)
				return false;
			line += nread;
		}

		// Parse molecule id.
		int moleculeId;
		if(sscanf(line, "%d%n", &moleculeId, &nread) != 1)
			return false;
		if(moleculeId < 0)
			return false;
		line += nread;

		// Parse the bond order of each bond.
		for(int j = 0; j < numBonds; j++) {
			FloatType bondOrder;
			if(sscanf(line, FLOATTYPE_SCANF_STRING "%n", &bondOrder, &nread) != 1)
				return false;
			if(bondOrder < 0.0 || bondOrder > 100.0)
				return false;
			line += nread;
		}

		// Parse atom bond order, number of lone pairs and atomic charge.
		FloatType abo, nlp, q;
		if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING "%n", &abo, &nlp, &q, &nread) != 3)
			return false;
		if(abo < 0.0 || nlp < 0.0)
			return false;
		line += nread;

		// Nothing should follow on the same line.
		while(*line != '\0') {
			if(!isspace(*line)) return false;
			if(*line == '\n' || *line == '\r')
				return true;
			++line;
		}
		return false;
	}

	return false;
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void ReaxFFBondImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Scanning ReaxFF bond file %1").arg(fileHandle().toString()));
	setProgressMaximum(stream.underlyingSize());

	Frame frame(fileHandle());
	QString filename = fileHandle().sourceUrl().fileName();
	int frameNumber = 0;

	bool inCommentSection = true;
	while(!stream.eof() && !isCanceled()) {
		const char* line = stream.readLineTrimLeft();

		if(line[0] == '#') {
			if(!inCommentSection) {
				frame.byteOffset = stream.byteOffset();
				frame.lineNumber = stream.lineNumber();
				inCommentSection = true;
			}
		}
		else if(inCommentSection) {
			frames.push_back(frame);
			inCommentSection = false;
			setProgressValueIntermittent(stream.underlyingByteOffset());
		}
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void ReaxFFBondImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading ReaxFF bond file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Hide particles, because this importer loads non-particle data.
	particles()->setVisElement(nullptr);

	// Data structure representing a single atom.
	struct ReaxFFAtom {
		qlonglong id;
		qlonglong moleculeId;
		FloatType abo, nlp, q;
	};

	// Data structure representing a single bond.
	struct ReaxFFBond {
		ParticleIndexPair atoms;
		FloatType bondOrder;
	};

	std::vector<ReaxFFAtom> reaxAtoms;
	std::vector<ReaxFFBond> reaxBonds;

	bool inCommentSection = true;
	while(!stream.eof() && !isCanceled()) {
		const char* line = stream.readLineTrimLeft();

		if(line[0] == '#') {
			if(inCommentSection) {
				// Skip comment lines starting with '#'.
				continue;
			}
			else {
				// We've reached the comment section of the next frame. Stop parsing.
				signalAdditionalFrames();
				break;
			}
		}

		inCommentSection = false;

		// Parse atom id, atom type and number of bonds.
		ReaxFFAtom reaxAtom;
		int atomType;
		int numBonds;
		int nread;
		if(sscanf(line, "%lld %d %d%n", &reaxAtom.id, &atomType, &numBonds, &nread) != 3 || reaxAtom.id < 1 || atomType < 1 || numBonds < 0 || numBonds > 100)
			throw Exception(tr("Invalid atom id, atom type, or number of bonds in line %1 of ReaxFF bond file.").arg(stream.lineNumber()));
		line += nread;

		// Parse the neighbor atom id of each bond.
		for(int j = 0; j < numBonds; j++) {
			ReaxFFBond reaxBond;
			reaxBond.atoms[0] = reaxAtom.id;
			if(sscanf(line, "%lld%n", &reaxBond.atoms[1], &nread) != 1 || reaxBond.atoms[1] < 1)
				throw Exception(tr("Invalid neighbor atom id in line %1 of ReaxFF bond file (bond index %2).").arg(stream.lineNumber()).arg(j));
			line += nread;

			reaxBonds.push_back(reaxBond);
		}

		// Parse molecule id.
		if(sscanf(line, "%lld%n", &reaxAtom.moleculeId, &nread) != 1 || reaxAtom.moleculeId < 0)
			throw Exception(tr("Invalid molecule id in line %1 of ReaxFF bond file.").arg(stream.lineNumber()));
		line += nread;

		// Parse the bond order of each bond.
		for(ReaxFFBond& bond : boost::make_iterator_range(reaxBonds.end() - numBonds, reaxBonds.end())) {
			if(sscanf(line, FLOATTYPE_SCANF_STRING "%n", &bond.bondOrder, &nread) != 1)
				throw Exception(tr("Invalid bond order value in line %1 of ReaxFF bond file.").arg(stream.lineNumber()));
			line += nread;
		}

		// Parse atom bond order, number of lone pairs and atomic charge.
		if(sscanf(line, FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING "%n", &reaxAtom.abo, &reaxAtom.nlp, &reaxAtom.q, &nread) != 3 || reaxAtom.abo < 0.0)
			throw Exception(tr("Invalid atom information in line %1 of ReaxFF bond file.").arg(stream.lineNumber()));

		// Remove every other half-bond from the list.
		reaxBonds.erase(std::remove_if(reaxBonds.begin(), reaxBonds.end(), 
			[](const ReaxFFBond& bond) { return bond.atoms[0] >= bond.atoms[1]; }), 
			reaxBonds.end());

		reaxAtoms.push_back(reaxAtom);
	}

	{
		// Create bonds storage.
		setBondCount(reaxBonds.size());
		PropertyAccess<ParticleIndexPair> bondParticleIdentifiersProperty = bonds()->createProperty(BondsObject::ParticleIdentifiersProperty, false, initializationHints());
		std::transform(reaxBonds.cbegin(), reaxBonds.cend(), bondParticleIdentifiersProperty.begin(), [](const ReaxFFBond& bond) { return bond.atoms; });

		// Create bond property for the bond order.
		PropertyAccess<FloatType> bondOrderProperty = bonds()->createProperty(QStringLiteral("Bond Order"), PropertyObject::Float, 1, 0, false);
		std::transform(reaxBonds.cbegin(), reaxBonds.cend(), bondOrderProperty.begin(), [](const ReaxFFBond& bond) { return bond.bondOrder; });

		// Create particle properties.
		setParticleCount(reaxAtoms.size());
		PropertyAccess<qlonglong> identifierProperty = particles()->createProperty(ParticlesObject::IdentifierProperty, false, initializationHints());
		std::transform(reaxAtoms.cbegin(), reaxAtoms.cend(), identifierProperty.begin(), [](const ReaxFFAtom& atom) { return atom.id; });
		PropertyAccess<FloatType> chargeProperty = particles()->createProperty(ParticlesObject::ChargeProperty, false, initializationHints());
		std::transform(reaxAtoms.cbegin(), reaxAtoms.cend(), chargeProperty.begin(), [](const ReaxFFAtom& atom) { return atom.q; });
		PropertyAccess<FloatType> atomBondOrderProperty = particles()->createProperty(QStringLiteral("Atom Bond Order"), PropertyObject::Float, 1, 0, false);
		std::transform(reaxAtoms.cbegin(), reaxAtoms.cend(), atomBondOrderProperty.begin(), [](const ReaxFFAtom& atom) { return atom.abo; });
		PropertyAccess<FloatType> lonePairsProperty = particles()->createProperty(QStringLiteral("Lone Pairs"), PropertyObject::Float, 1, 0, false);
		std::transform(reaxAtoms.cbegin(), reaxAtoms.cend(), lonePairsProperty.begin(), [](const ReaxFFAtom& atom) { return atom.nlp; });
	}

	state().setStatus(tr("%1 atoms and %2 bonds").arg(reaxAtoms.size()).arg(reaxBonds.size()));

	// Call base implementation to finalize the loaded data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
