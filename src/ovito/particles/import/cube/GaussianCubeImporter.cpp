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
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/grid/objects/VoxelGrid.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/io/NumberParsing.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include "GaussianCubeImporter.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(GaussianCubeImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GaussianCubeImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Ignore two comment lines.
	stream.readLine(1024);
	stream.readLine(1024);

	// Read number of atoms and cell origin coordinates.
	int numAtoms;
	Vector3 cellOrigin;
	char c;
	if(sscanf(stream.readLine(), "%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %c", &numAtoms, &cellOrigin.x(), &cellOrigin.y(), &cellOrigin.z(), &c) != 4)
		return false;
	if(numAtoms == 0)
		return false;

	// Read voxel count and cell vectors.
	int gridSize[3];
	Vector3 cellVectors[3];
	for(size_t dim = 0; dim < 3; dim++) {
		if(sscanf(stream.readLine(), "%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %c", &gridSize[dim], &cellVectors[dim].x(), &cellVectors[dim].y(), &cellVectors[dim].z(), &c) != 4)
			return false;
		if(gridSize[dim] == 0)
			return false;
	}

	// Read first atom line.
	int atomType;
	FloatType val;
	Point3 pos;
	if(sscanf(stream.readLine(), "%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %c", &atomType, &val, &pos.x(), &pos.y(), &pos.z(), &c) != 5)
		return false;

	return true;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void GaussianCubeImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading Gaussian Cube file %1").arg(fileHandle().toString()));

	// Ignore two comment lines.
	stream.readLine();
	stream.readLine();

	// Read number of atoms and cell origin coordinates.
	qlonglong numAtoms;
	bool voxelFieldTablePresent = false;
	AffineTransformation cellMatrix;
	if(sscanf(stream.readLine(), "%lli " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &numAtoms, &cellMatrix.translation().x(), &cellMatrix.translation().y(), &cellMatrix.translation().z()) != 4)
		throw Exception(tr("Invalid number of atoms or origin coordinates in line %1 of Cube file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	if(numAtoms < 0) {
		numAtoms = -numAtoms;
		voxelFieldTablePresent = true;
	}

	// Read voxel counts and cell vectors.
	bool isBohrUnits = true;
	VoxelGrid::GridDimensions gridSize;
	for(size_t dim = 0; dim < 3; dim++) {
		int gs;
		if(sscanf(stream.readLine(), "%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &gs, &cellMatrix.column(dim).x(), &cellMatrix.column(dim).y(), &cellMatrix.column(dim).z()) != 4)
			throw Exception(tr("Invalid number of voxels or cell vector in line %1 of Cube file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		if(gs < 0) {
			gs = -gs;
			isBohrUnits = false;
		}
		if(gs == 0)
			throw Exception(tr("Number of grid voxels out of range in line %1 of Cube file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		gridSize[dim] = gs;
		cellMatrix.column(dim) *= gs;
	}
	// Automatically convert from Bohr units to Angstroms units.
	if(isBohrUnits)
		cellMatrix = cellMatrix * 0.52917721067;
	simulationCell()->setPbcFlags(true, true, true);
	simulationCell()->setCellMatrix(cellMatrix);

	// Create the particle properties.
	setParticleCount(numAtoms);
	PropertyAccess<Point3> posProperty = particles()->createProperty(ParticlesObject::PositionProperty, false, initializationHints());
	PropertyAccess<int> typeProperty = particles()->createProperty(ParticlesObject::TypeProperty, false, initializationHints());

	// Read atomic coordinates.
	Point3* p = posProperty.begin();
	int* a = typeProperty.begin();
	setProgressMaximum(numAtoms + gridSize[0]*gridSize[1]*gridSize[2]);
	for(qlonglong i = 0; i < numAtoms; i++, ++p, ++a) {
		if(!setProgressValueIntermittent(i)) return;
		FloatType secondColumn;
		if(sscanf(stream.readLine(), "%i " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING,
				a, &secondColumn, &p->x(), &p->y(), &p->z()) != 5)
			throw Exception(tr("Invalid atom information in line %1 of Cube file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
		// Automatically convert from Bohr units to Angstroms units.
		if(isBohrUnits)
			(*p) *= 0.52917721067;
	}

	// Translate atomic numbers into element names.
	for(int atomicNumber : typeProperty) {
		if(atomicNumber >= 0 && atomicNumber < ParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES)
			addNumericType(ParticlesObject::OOClass(), typeProperty.buffer(), atomicNumber, ParticleType::getPredefinedParticleTypeName(static_cast<ParticleType::PredefinedParticleType>(atomicNumber)));
		else
			addNumericType(ParticlesObject::OOClass(), typeProperty.buffer(), atomicNumber, {});
	}

	// Parse voxel field table.
	const char* s = stream.readLine();
	size_t nfields = 0;
	QStringList componentNames;
	if(voxelFieldTablePresent) {
		int m = -1;
		const char* token;
		for(;;) {
			for(;;) {
				while(*s == ' ' || *s == '\t') ++s;
				token = s;
				while(*s > ' ' || *s < 0) ++s;
				if(s != token) break;
				s = stream.readLine();
			}
			int value;
			if(!parseInt(token, s, value))
				throw Exception(tr("Invalid integer value in line %1 of Cube file: \"%2\"").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(token, s - token)));
			if(*s != '\0')
				s++;
			if(m == -1) {
				m = value;
				if(m <= 0) throw Exception(tr("Invalid field count in line %1 of Cube file: \"%2\"").arg(stream.lineNumber()).arg(value));
				nfields = m;
			}
			else if(m != 0) {
				componentNames.push_back(QStringLiteral("MO%1").arg(value));
				if(--m == 0) break;
			}
		}
	}
	else {
		// No field table present. Assume file contains a single field property.
		nfields = 1;
	}

	// Create the voxel grid data object.
	VoxelGrid* voxelGrid = state().getMutableObject<VoxelGrid>();
	if(!voxelGrid) {
		voxelGrid = state().createObject<VoxelGrid>(dataSource(), initializationHints());
		voxelGrid->visElement()->setEnabled(false);
		voxelGrid->visElement()->freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(ActiveObject::isEnabled)});
	}
	voxelGrid->setDomain(simulationCell());
	voxelGrid->setIdentifier(QStringLiteral("imported"));
	voxelGrid->setShape(gridSize);
	voxelGrid->setContent(gridSize[0] * gridSize[1] * gridSize[2], {});

	// Create the voxel grid property.
	PropertyAccess<FloatType, true> fieldQuantity = voxelGrid->createProperty(QStringLiteral("Property"), PropertyObject::Float, nfields, 0, false, std::move(componentNames));

	// Parse voxel data.
	for(size_t x = 0; x < gridSize[0]; x++) {
		for(size_t y = 0; y < gridSize[1]; y++) {
			for(size_t z = 0; z < gridSize[2]; z++) {
				for(size_t compnt = 0; compnt < fieldQuantity.componentCount(); compnt++) {
					const char* token;
					for(;;) {
						while(*s == ' ' || *s == '\t') ++s;
						token = s;
						while(*s > ' ' || *s < 0) ++s;
						if(s != token) break;
						s = stream.readLine();
					}
					FloatType value;
					if(!parseFloatType(token, s, value))
						throw Exception(tr("Invalid value in line %1 of Cube file: \"%2\"").arg(stream.lineNumber()).arg(QString::fromLocal8Bit(token, s - token)));
					fieldQuantity.set(z * gridSize[0] * gridSize[1] + y * gridSize[0] + x, compnt, value);
					if(*s != '\0')
						s++;
				}
				if(!setProgressValueIntermittent(progressValue() + 1))
					return;
			}
		}
	}
	voxelGrid->verifyIntegrity();

	state().setStatus(tr("%1 atoms\n%2 x %3 x %4 voxel grid").arg(numAtoms).arg(gridSize[0]).arg(gridSize[1]).arg(gridSize[2]));

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
