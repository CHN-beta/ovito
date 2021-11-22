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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/core/utilities/io/NumberParsing.h>
#include "PDBImporter.h"

#include <3rdparty/gemmi/pdb.hpp>

namespace gemmi { namespace pdb_impl {
template<>
inline size_t copy_line_from_stream<Ovito::CompressedTextReader&>(char* line, int size, Ovito::CompressedTextReader& stream) 
{
	// Return no line if end of file has been reached.
	if(stream.eof())
		return 0;

	// Read a single line form the input stream.
	const char* src_line = stream.readLine();

	// Stop reading the file when ENDMDL marker is reached. We don't want Gemmi to read all frames of a trajectory file.
	if(is_record_type(src_line, "ENDMDL")) {
		return 0;
	}

	// Copy line contents to output buffer.
	size_t len = qstrlen(src_line);
	qstrncpy(line, src_line, size);

	if(is_record_type(src_line, "ATOM") || is_record_type(src_line, "HETATM")) {
		// Some PDB files have an ATOM or HETATM line that is shorter than what Gemmi's parser expects.
		// Pad such lines by appending  additional whitespace.
		if(len < 66 && len >= 54 && size > 66) {
			while(len < 66)
				line[len++] = ' ';
			line[len] = '\0';
		}

		// Gemmi expects atom names to start at column index 12. Some files have one extra space at this positions and the 
		// name actually begins at position 13. Make the parser happy by moving the text by one positon to the left.
		// For example, turn " Au " into "Au  ", but preserve " CA " or " HE ".
		if(len >= 16 && size >= 16 && line[12] == ' ' && line[13] >= 'A' && line[13] <= 'Z' && line[14] >= 'a' && line[14] <= 'z' && line[15] == ' ') {
			line[12] = line[13];
			line[13] = line[14];
			line[14] = ' ';
			line[15] = ' ';
		}
		// Some files have 2 extra spaces at this positions and the name actually begins at position 14. Make the parser happy by moving the text by two characters to the left.
		// For example, turn "  O " into "O   ":
		else if(len >= 16 && size >= 16 && line[12] == ' ' && line[13] == ' ' && line[14] >= 'A' && line[14] <= 'Z') {
			line[12] = line[14];
			line[13] = line[15];
			line[14] = ' ';
			line[15] = ' ';
		}
		// Some files have a digit prepended to the element name. Remove it so that Gemmi can recognize the element correctly.
		// For example, turn "1HH1" into " HH1":
		else if(len >= 16 && size >= 16 && line[12] >= '1' && line[12] <= '9' && line[13] >= 'A' && line[13] <= 'Z') {
			line[12] = ' ';
		}
	}

	// Return line length (up to maximum) to caller.
	return std::min(len, (size_t)(size - 1));
}
}}

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(PDBImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool PDBImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Read up to 60 lines from the beginning of the file.
	for(int i = 0; i < 60 && !stream.eof(); i++) {
		stream.readLine(122);
		if(qstrlen(stream.line()) > 120 && !stream.lineStartsWithToken("TITLE"))
			return false;
		if(qstrlen(stream.line()) >= 7 && stream.line()[6] != ' ' && std::find(stream.line(), stream.line()+6, ' ') != stream.line()+6)
			return false;
		if(stream.lineStartsWithToken("HEADER") || stream.lineStartsWithToken("ATOM") || stream.lineStartsWith("HETATM"))
			return true;
	}

	return false;
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void PDBImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Scanning PDB file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	Frame frame(fileHandle());
	frame.byteOffset = stream.byteOffset();
	frame.lineNumber = stream.lineNumber();
	while(!stream.eof()) {

		if(isCanceled())
			return;

		stream.readLine();

		if(!setProgressValueIntermittent(stream.underlyingByteOffset()))
			return;

		if(stream.lineStartsWithToken("ENDMDL")) {
			frames.push_back(frame);
			frame.byteOffset = stream.byteOffset();
			frame.lineNumber = stream.lineNumber();
		}
	}

	if(frames.empty()) {
		// It's not a trajectory file. Report just a single frame.
		frames.push_back(Frame(fileHandle()));
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void PDBImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading PDB file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	try {
		// Parse the PDB file's contents.
		gemmi::Structure structure = gemmi::pdb_impl::read_pdb_from_line_input(stream, qPrintable(frame().sourceFile.path()), gemmi::PdbReadOptions());
		if(isCanceled()) return;

		// Import PDB metadata fields as global attributes. 
		for(const auto& m : structure.info)
			state().setAttribute(QString::fromStdString(m.first), QVariant::fromValue(QString::fromStdString(m.second)), dataSource());

		structure.merge_chain_parts();
		if(isCanceled()) return;

		if(structure.models.empty())
			throw Exception(tr("PDB parsing error: No structural models."));
		const gemmi::Model& model = structure.models.back();

		// Count total number of atoms.
		size_t natoms = 0;
		for(const gemmi::Chain& chain : model.chains) {
			for(const gemmi::Residue& residue : chain.residues) {
				natoms += residue.atoms.size();
			}
		}

		// Allocate property arrays for atoms.
		setParticleCount(natoms);
		PropertyAccess<Point3> posProperty = particles()->createProperty(ParticlesObject::PositionProperty, false, executionContext());
		PropertyAccess<int> typeProperty = particles()->createProperty(ParticlesObject::TypeProperty, false, executionContext());
		PropertyAccess<int> atomNameProperty = particles()->createProperty(QStringLiteral("Atom Name"), PropertyObject::Int, 1, 0, false);
		PropertyAccess<int> residueTypeProperty = particles()->createProperty(QStringLiteral("Residue Type"), PropertyObject::Int, 1, 0, false);

		// Give these particle properties new titles, which are displayed in the GUI under the file source.
		atomNameProperty.buffer()->setTitle(tr("Atom names"));
		residueTypeProperty.buffer()->setTitle(tr("Residue types"));

		Point3* posIter = posProperty.begin();
		int* typeIter = typeProperty.begin();
		int* atomNameIter = atomNameProperty.begin();
		int* residueTypeIter = residueTypeProperty.begin();

		// Transfer atomic data from Gemmi to OVITO data structures.
		bool hasOccupancy = false;
		for(const gemmi::Chain& chain : model.chains) {
			for(const gemmi::Residue& residue : chain.residues) {
				if(isCanceled()) return;
				int residueTypeId = (residue.name.empty() == false) ? addNamedType(ParticlesObject::OOClass(), residueTypeProperty.buffer(), QLatin1String(residue.name.c_str(), residue.name.size()))->numericId() : 0;
				for(const gemmi::Atom& atom : residue.atoms) {
					// Atomic position.
					*posIter++ = Point3(atom.pos.x, atom.pos.y, atom.pos.z);

					// Chemical type.
					*typeIter++ = atom.element.ordinal();
					addNumericType(ParticlesObject::OOClass(), typeProperty.buffer(), atom.element.ordinal(), QString::fromStdString(atom.element.name()));

					// Atom name.
					*atomNameIter++ = addNamedType(ParticlesObject::OOClass(), atomNameProperty.buffer(), QLatin1String(atom.name.c_str(), atom.name.size()))->numericId();

					// Residue type.
					*residueTypeIter++ = residueTypeId;

					// Check for presence of occupancy values.
					if(atom.occ != 0 && atom.occ != 1) hasOccupancy = true;
				}
			}
		}
		if(isCanceled()) return;

		// Parse the optional site occupancy information.
		if(hasOccupancy) {
			PropertyAccess<FloatType> occupancyProperty = particles()->createProperty(QStringLiteral("Occupancy"), PropertyObject::Float, 1, 0, false);
			FloatType* occupancyIter = occupancyProperty.begin();
			for(const gemmi::Chain& chain : model.chains) {
				for(const gemmi::Residue& residue : chain.residues) {
					for(const gemmi::Atom& atom : residue.atoms) {
						*occupancyIter++ = atom.occ;
					}
				}
			}
			OVITO_ASSERT(occupancyIter == occupancyProperty.end());
		}

		// Since we created particle types on the go while reading the particles, the assigned particle type IDs
		// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
		// why we sort them now.
		typeProperty.buffer()->sortElementTypesById();
		atomNameProperty.buffer()->sortElementTypesByName();
		residueTypeProperty.buffer()->sortElementTypesByName();
		typeProperty.reset();
		atomNameProperty.reset();
		residueTypeProperty.reset();

		// Parse unit cell.
		if(structure.cell.is_crystal()) {
			// Process periodic unit cell definition.
			AffineTransformation cell = AffineTransformation::Identity();
			if(structure.cell.alpha == 90 && structure.cell.beta == 90 && structure.cell.gamma == 90) {
				cell(0,0) = structure.cell.a;
				cell(1,1) = structure.cell.b;
				cell(2,2) = structure.cell.c;
			}
			else if(structure.cell.alpha == 90 && structure.cell.beta == 90) {
				FloatType gamma = qDegreesToRadians(structure.cell.gamma);
				cell(0,0) = structure.cell.a;
				cell(0,1) = structure.cell.b * std::cos(gamma);
				cell(1,1) = structure.cell.b * std::sin(gamma);
				cell(2,2) = structure.cell.c;
			}
			else {
				FloatType alpha = qDegreesToRadians(structure.cell.alpha);
				FloatType beta = qDegreesToRadians(structure.cell.beta);
				FloatType gamma = qDegreesToRadians(structure.cell.gamma);
				FloatType v = structure.cell.a * structure.cell.b * structure.cell.c * sqrt(1.0 - std::cos(alpha)*std::cos(alpha) - std::cos(beta)*std::cos(beta) - std::cos(gamma)*std::cos(gamma) + 2.0 * std::cos(alpha) * std::cos(beta) * std::cos(gamma));
				cell(0,0) = structure.cell.a;
				cell(0,1) = structure.cell.b * std::cos(gamma);
				cell(1,1) = structure.cell.b * std::sin(gamma);
				cell(0,2) = structure.cell.c * std::cos(beta);
				cell(1,2) = structure.cell.c * (std::cos(alpha) - std::cos(beta)*std::cos(gamma)) / std::sin(gamma);
				cell(2,2) = v / (structure.cell.a * structure.cell.b * std::sin(gamma));
			}
			simulationCell()->setCellMatrix(cell);
		}
		else if(posProperty.size() != 0) {
			// Use bounding box of atomic coordinates as non-periodic simulation cell.
			Box3 boundingBox;
			boundingBox.addPoints(posProperty);
			simulationCell()->setPbcFlags(false, false, false);
			simulationCell()->setCellMatrix(AffineTransformation(
					Vector3(boundingBox.sizeX(), 0, 0),
					Vector3(0, boundingBox.sizeY(), 0),
					Vector3(0, 0, boundingBox.sizeZ()),
					boundingBox.minc - Point3::Origin()));
		}
		state().setStatus(tr("Number of atoms: %1").arg(natoms));
	}
	catch(const std::exception& e) {
		throw Exception(tr("PDB file error: %1").arg(e.what()));
	}

	// Check if more frames are following in the trajectory file.
	if(!stream.eof()) {
		stream.readLine();
		if(!stream.eof()) {
			signalAdditionalFrames();
		}
	}

	// Generate ad-hoc bonds between atoms based on their van der Waals radii.
	if(_generateBonds)
		generateBonds();
	else
		setBondCount(0);

	// If the loaded particles are centered on the coordinate origin but the periodi simulation box corner is positioned at (0,0,0), 
	// then shift the cell to center it on (0,0,0) too, leaving the particle coordinates as is.
	// correctOffcenterCell();

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
