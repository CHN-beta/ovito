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
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "mmCIFImporter.h"

#include <3rdparty/gemmi/cif.hpp>
#include <3rdparty/gemmi/mmread.hpp>

namespace Ovito { namespace Particles {

namespace cif = gemmi::cif;

IMPLEMENT_OVITO_CLASS(mmCIFImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool mmCIFImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// First, determine if it is a CIF file.
	// Read the first N lines of the file which are not comments.
	int maxLines = 12;
	bool foundBlockHeader = false;
	bool foundItem = false;
	for(int i = 0; i < maxLines && !stream.eof(); i++) {
		// Note: Maximum line length of CIF files is 2048 characters.
		stream.readLine(2048);

		if(stream.lineStartsWith("#", true)) {
			maxLines++;
			continue;
		}
		else if(stream.lineStartsWith("data_", false)) {
			// Make sure the "data_XXX" block header appears.
			if(foundBlockHeader) return false;
			foundBlockHeader = true;
		}
		else if(stream.lineStartsWith("_", false)) {
			// Make sure at least one "_XXX" item appears.
			foundItem = true;
			break;
		}
	}

	// Make sure it is a CIF file.
	if(!foundBlockHeader || !foundItem)
		return false;

	// Continue reading the entire file until at least one "_atom_site.XXX" entry is found.
	// These entries are specific to the mmCIF format and do not occur in CIF files (small molecule files).
	for(;;) {
		if(stream.lineStartsWith("_atom_site.", false))
			return true;
		if(stream.eof())
			return false;
		stream.readLine();
	}

	return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void mmCIFImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading mmCIF file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Map the whole file into memory for parsing.
	const char* buffer_start;
	const char* buffer_end;
	QByteArray fileContents;
	std::tie(buffer_start, buffer_end) = stream.mmap();
	if(!buffer_start) {
		// Could not map mmCIF file into memory. Read it into a in-memory buffer instead.
		fileContents = stream.readAll();
		buffer_start = fileContents.constData();
		buffer_end = buffer_start + fileContents.size();
	}

	try {
		// Parse the mmCIF file's contents.
		cif::Document doc = cif::read_memory(buffer_start, buffer_end - buffer_start, qPrintable(frame().sourceFile.path()));

		// Unmap the input file from memory.
		if(fileContents.isEmpty())
			stream.munmap();
		if(isCanceled()) return;

		// Parse the mmCIF data into an molecular structure representation.
		gemmi::Structure structure = gemmi::make_structure(doc);
		structure.merge_chain_parts();
		if(isCanceled()) return;

		const gemmi::Model& model = structure.first_model();

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
		Point3* posIter = posProperty.begin();
		int* typeIter = typeProperty.begin();

		// Transfer atomic data to OVITO data structures.
		bool hasOccupancy = false;
		for(const gemmi::Chain& chain : model.chains) {
			for(const gemmi::Residue& residue : chain.residues) {
				if(isCanceled()) return;
				for(const gemmi::Atom& atom : residue.atoms) {
					// Atomic position.
					*posIter++ = Point3(atom.pos.x, atom.pos.y, atom.pos.z);

					// Atomic type.
					*typeIter++ = atom.element.ordinal();
					addNumericType(ParticlesObject::OOClass(), typeProperty.property(), atom.element.ordinal(), QString::fromStdString(atom.element.name()));

					// Check for presence of occupancy values.
					if(atom.occ != 1) hasOccupancy = true;
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
		// depend on the storage order of particles in the file We rather want a well-defined particle type ordering, that's
		// why we sort them now.
		typeProperty.property()->sortElementTypesById();

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
		throw Exception(tr("mmCIF file reader error: %1").arg(e.what()));
	}

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
}	// End of namespace
