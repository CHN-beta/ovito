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
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/stdobj/properties/InputColumnMapping.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/NumberParsing.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "XSFImporter.h"

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(XSFImporter);

const char* XSFImporter::chemical_symbols[] = {
    // 0
    "X",
    // 1
    "H", "He",
    // 2
    "Li", "Be", "B", "C", "N", "O", "F", "Ne",
    // 3
    "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar",
    // 4
    "K", "Ca", "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn",
    "Ga", "Ge", "As", "Se", "Br", "Kr",
    // 5
    "Rb", "Sr", "Y", "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd",
    "In", "Sn", "Sb", "Te", "I", "Xe",
    // 6
    "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy",
    "Ho", "Er", "Tm", "Yb", "Lu",
    "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi",
    "Po", "At", "Rn",
    // 7
    "Fr", "Ra", "Ac", "Th", "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk",
    "Cf", "Es", "Fm", "Md", "No", "Lr",
    "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc",
    "Lv", "Ts", "Og"
};

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool XSFImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Look for 'ATOMS', 'BEGIN_BLOCK_DATAGRID' or other XSF-specific keywords.
	// One of them must appear within the first 40 lines of the file.
	for(int i = 0; i < 40 && !stream.eof(); i++) {
		const char* line = stream.readLineTrimLeft(1024);

		if(boost::algorithm::starts_with(line, "ATOMS")) {
			// Make sure the line following the keyword has the right format.
			return (sscanf(stream.readLineTrimLeft(1024), "%*s %*g %*g %*g") == 0);
		}
		else if(boost::algorithm::starts_with(line, "PRIMCOORD") || boost::algorithm::starts_with(line, "CONVCOORD")) {
			// Make sure the line following the keyword has the right format.
			return (sscanf(stream.readLineTrimLeft(1024), "%*ull %*i") == 0);
		}
		else if(boost::algorithm::starts_with(line, "BEGIN_BLOCK_DATAGRID")) {
			return true;
		}
	}
	return false;
}

/******************************************************************************
* Scans the data file and builds a list of source frames.
******************************************************************************/
void XSFImporter::FrameFinder::discoverFramesInFile(QVector<FileSourceImporter::Frame>& frames)
{
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Scanning XSF file %1").arg(stream.filename()));
	setProgressMaximum(stream.underlyingSize());

	int nFrames = 1;
	while(!stream.eof() && !isCanceled()) {
		const char* line = stream.readLineTrimLeft(1024);
		if(boost::algorithm::starts_with(line, "ANIMSTEPS")) {
			if(sscanf(line, "ANIMSTEPS %i", &nFrames) != 1 || nFrames < 1)
				throw Exception(tr("XSF file parsing error. Invalid ANIMSTEPS in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			break;
		}
		else if(line[0] != '#') {
			break;
		}
		setProgressValueIntermittent(stream.underlyingByteOffset());
	}

	Frame frame(fileHandle());
	QString filename = fileHandle().sourceUrl().fileName();
	for(int i = 0; i < nFrames; i++) {
		frame.lineNumber = i;
		frame.label = tr("%1 (Frame %2)").arg(filename).arg(i);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void XSFImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading XSF file %1").arg(fileHandle().toString()));

	// The animation frame number to load from the XSF file.
	int frameNumber = frame().lineNumber + 1;

	VoxelGrid* voxelGrid = nullptr;
	while(!stream.eof()) {
		if(isCanceled()) return;
		const char* line = stream.readLineTrimLeft(1024);
		if(boost::algorithm::starts_with(line, "ATOMS")) {

			int anim;
			if(sscanf(line, "ATOMS %i", &anim) == 1 && anim != frameNumber)
				continue;

			std::vector<Point3> coords;
			std::vector<QString> types;
			std::vector<Vector3> forces;
			while(!stream.eof()) {
				Point3 pos;
				Vector3 f;
				char atomTypeName[16];
				int nfields = sscanf(stream.readLine(), "%15s " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						atomTypeName, &pos.x(), &pos.y(), &pos.z(), &f.x(), &f.y(), &f.z());
				if(nfields != 4 && nfields != 7) break;
				coords.push_back(pos);
				int atomTypeId;
				if(sscanf(atomTypeName, "%i", &atomTypeId) == 1 && atomTypeId >= 0 && atomTypeId < sizeof(chemical_symbols)/sizeof(chemical_symbols[0])) {
					types.emplace_back(QLatin1String(chemical_symbols[atomTypeId]));
				}
				else {
					types.emplace_back(QLatin1String(atomTypeName));
				}
				if(nfields == 7) {
					forces.resize(coords.size(), Vector3::Zero());
					forces.back() = f;
				}
				if(isCanceled()) return;
			}
			if(coords.empty())
				throw Exception(tr("Invalid ATOMS section in line %1 of XSF file.").arg(stream.lineNumber()));

			// Will continue parsing subsequent lines from the file.
			line = stream.line();

			setParticleCount(coords.size());
			PropertyAccess<Point3> posProperty = particles()->createProperty(ParticlesObject::PositionProperty, false, executionContext());
			boost::copy(coords, posProperty.begin());

			PropertyAccess<int> typeProperty = particles()->createProperty(ParticlesObject::TypeProperty, false, executionContext());
			boost::transform(types, typeProperty.begin(), [&](const QString& typeName) {
				return addNamedType(typeProperty.property(), typeName, ParticleType::OOClass())->numericId();
			});
			// Since we created particle types on the go while reading the particles, the type ordering
			// depends on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
			// why we sort them now.
			typeProperty.property()->sortElementTypesByName();

			if(forces.size() == coords.size()) {
				PropertyAccess<Vector3> forceProperty = particles()->createProperty(ParticlesObject::ForceProperty, false, executionContext());
				boost::copy(forces, forceProperty.begin());
			}

			state().setStatus(tr("%1 atoms").arg(coords.size()));

			// If the input file does not contain simulation cell info,
			// Use bounding box of particles as simulation cell.
			Box3 boundingBox;
			boundingBox.addPoints(posProperty);
			simulationCell()->setCellMatrix(AffineTransformation(
					Vector3(boundingBox.sizeX(), 0, 0),
					Vector3(0, boundingBox.sizeY(), 0),
					Vector3(0, 0, boundingBox.sizeZ()),
					boundingBox.minc - Point3::Origin()));
			simulationCell()->setPbcFlags(false, false, false);
		}

		if(boost::algorithm::starts_with(line, "CRYSTAL")) {
			simulationCell()->setPbcFlags(true, true, true);
		}
		else if(boost::algorithm::starts_with(line, "SLAB")) {
			simulationCell()->setPbcFlags(true, true, false);
		}
		else if(boost::algorithm::starts_with(line, "POLYMER")) {
			simulationCell()->setPbcFlags(true, false, false);
		}
		else if(boost::algorithm::starts_with(line, "MOLECULE")) {
			simulationCell()->setPbcFlags(false, false, false);
		}
		else if(boost::algorithm::starts_with(line, "PRIMVEC")) {
			int anim;
			if(sscanf(line, "PRIMVEC %i", &anim) == 1 && anim != frameNumber)
				continue;
			AffineTransformation cell = AffineTransformation::Identity();
			for(size_t i = 0; i < 3; i++) {
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						&cell.column(i).x(), &cell.column(i).y(), &cell.column(i).z()) != 3)
					throw Exception(tr("Invalid cell vector in XSF file at line %1").arg(stream.lineNumber()));
			}
			simulationCell()->setCellMatrix(cell);
		}
		else if(boost::algorithm::starts_with(line, "PRIMCOORD")) {
			int anim;
			if(sscanf(line, "PRIMCOORD %i", &anim) == 1 && anim != frameNumber)
				continue;

			// Parse number of atoms.
			unsigned long long u;
			int ii;
			if(sscanf(stream.readLine(), "%llu %i", &u, &ii) != 2)
				throw Exception(tr("XSF file parsing error. Invalid number of atoms in line %1:\n%2").arg(stream.lineNumber()).arg(stream.lineString()));
			size_t natoms = (size_t)u;
			setParticleCount(natoms);

			qint64 atomsListOffset = stream.byteOffset();
			int atomsLineNumber = stream.lineNumber();

			// Detect number of columns.
			Point3 pos;
			Vector3 f;
			int nfields = sscanf(stream.readLine(), "%*s " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&pos.x(), &pos.y(), &pos.z(), &f.x(), &f.y(), &f.z());
			if(nfields != 3 && nfields != 6)
				throw Exception(tr("XSF file parsing error. Invalid number of data columns in line %1.").arg(stream.lineNumber()));

			// Prepare the file column to particle property mapping.
			ParticleInputColumnMapping columnMapping;
			columnMapping.resize(nfields + 1);
			columnMapping.mapStandardColumn(0, ParticlesObject::TypeProperty);
			columnMapping.mapStandardColumn(1, ParticlesObject::PositionProperty, 0);
			columnMapping.mapStandardColumn(2, ParticlesObject::PositionProperty, 1);
			columnMapping.mapStandardColumn(3, ParticlesObject::PositionProperty, 2);
			if(nfields == 6) {
				columnMapping.mapStandardColumn(4, ParticlesObject::ForceProperty, 0);
				columnMapping.mapStandardColumn(5, ParticlesObject::ForceProperty, 1);
				columnMapping.mapStandardColumn(6, ParticlesObject::ForceProperty, 2);
			}

			// Jump back to start of atoms list.
			stream.seek(atomsListOffset, atomsLineNumber);

			// Parse atoms data.
			InputColumnReader columnParser(columnMapping, particles(), executionContext());
			setProgressMaximum(natoms);
			for(size_t i = 0; i < natoms; i++) {
				if(!setProgressValueIntermittent(i)) return;
				try {
					columnParser.readElement(i, stream.readLine());
				}
				catch(Exception& ex) {
					throw ex.prependGeneralMessage(tr("Parsing error in line %1 of XSF file.").arg(atomsLineNumber + i));
				}
			}
			columnParser.sortElementTypes();

			// Give numeric atom types chemical names.
			if(PropertyObject* typeProperty = particles()->getMutableProperty(ParticlesObject::TypeProperty)) {
				for(int i = 0; i < typeProperty->elementTypes().size(); i++) {
					const ElementType* type = typeProperty->elementTypes()[i];
					int typeId = type->numericId();
					if(type->name().isEmpty() && typeId >= 0 && typeId < sizeof(chemical_symbols)/sizeof(chemical_symbols[0]))
						typeProperty->makeMutable(type)->setName(chemical_symbols[typeId]);
				}
			}
		}
		else if(boost::algorithm::starts_with(line, "BEGIN_BLOCK_DATAGRID_3D") || boost::algorithm::starts_with(line, "BLOCK_DATAGRID_3D") || boost::algorithm::starts_with(line, "BEGIN_BLOCK_DATAGRID3D")) {
			stream.readLine();
			QString gridId = stream.lineString().trimmed();
			if(gridId.isEmpty()) gridId = QStringLiteral("imported");

			// Create the voxel grid data object.
			voxelGrid = state().getMutableLeafObject<VoxelGrid>(VoxelGrid::OOClass(), gridId);
			if(!voxelGrid)
				voxelGrid = state().createObject<VoxelGrid>(dataSource(), executionContext(), gridId);
			voxelGrid->setDomain(simulationCell());
			voxelGrid->setIdentifier(gridId);
		}
		else if(boost::algorithm::starts_with(line, "BEGIN_DATAGRID_3D_") || boost::algorithm::starts_with(line, "DATAGRID_3D_")) {
			QString name = QString::fromLatin1(line + (boost::algorithm::starts_with(line, "BEGIN_DATAGRID_3D_") ? 18 : 12)).trimmed();

			// Parse grid dimensions.
			VoxelGrid::GridDimensions gridSize;
			if(sscanf(stream.readLine(), "%zu %zu %zu", &gridSize[0], &gridSize[1], &gridSize[2]) != 3 || !voxelGrid)
				throw Exception(tr("XSF file parsing error. Invalid data grid specification in line %1: %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(voxelGrid->shape() != gridSize) {
				voxelGrid->setShape(gridSize);
				voxelGrid->setContent(gridSize[0] * gridSize[1] * gridSize[2], {});
			}

			AffineTransformation cell = AffineTransformation::Identity();
			if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
					&cell.column(3).x(), &cell.column(3).y(), &cell.column(3).z()) != 3)
				throw Exception(tr("Invalid cell origin in XSF file at line %1").arg(stream.lineNumber()));
			for(size_t i = 0; i < 3; i++) {
				if(sscanf(stream.readLine(), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
						&cell.column(i).x(), &cell.column(i).y(), &cell.column(i).z()) != 3)
					throw Exception(tr("Invalid cell vector in XSF file at line %1").arg(stream.lineNumber()));
			}
			if(voxelGrid->domain()) {
				voxelGrid->mutableDomain()->setCellMatrix(cell);
			}
			else {
				DataOORef<SimulationCellObject> simCell = DataOORef<SimulationCellObject>::create(dataset(), executionContext(), cell, true, true, true, false);
				simCell->setDataSource(dataSource());
				voxelGrid->setDomain(std::move(simCell));
			}

			PropertyAccess<FloatType> fieldQuantity = voxelGrid->createProperty(name, PropertyObject::Float, 1, 0, false);
			FloatType* data = fieldQuantity.begin();
			setProgressMaximum(fieldQuantity.size());
			const char* s = "";
			for(size_t i = 0; i < fieldQuantity.size(); i++, ++data) {
				const char* token;
				for(;;) {
					while(*s == ' ' || *s == '\t') ++s;
					token = s;
					while(*s > ' ' || *s < 0) ++s;
					if(s != token) break;
					s = stream.readLine();
				}
				if(!parseFloatType(token, s, *data))
					throw Exception(tr("Invalid numeric value in data grid section in line %1: \"%2\"").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(token, s - token)));
				if(*s != '\0')
					s++;

				if(!setProgressValueIntermittent(i)) return;
			}
		}
	}

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
}	// End of namespace
