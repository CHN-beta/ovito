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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include "ParcasFileImporter.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(ParcasFileImporter);

// Byte-swaps a 32 bit word.
#define SWAP32(x) (((uint32_t)(x) >> 24) |			\
				   (((uint32_t)(x) & 0x00ff0000) >> 8) |	\
				   (((uint32_t)(x) & 0x0000ff00) << 8) |	\
				   ((uint32_t)(x) << 24))

// Byte-swaps a 64 bit word.
#define SWAP64(x) (((uint64_t)(x) >> 56) |				\
				   (((uint64_t)(x) & UINT64_C(0x00ff000000000000)) >> 40) |	\
				   (((uint64_t)(x) & UINT64_C(0x0000ff0000000000)) << 24) |	\
				   (((uint64_t)(x) & UINT64_C(0x000000ff00000000)) << 8) |	\
				   (((uint64_t)(x) & UINT64_C(0x00000000ff000000)) << 8) |	\
				   (((uint64_t)(x) & UINT64_C(0x0000000000ff0000)) << 24) |	\
				   (((uint64_t)(x) & UINT64_C(0x000000000000ff00)) << 40) |	\
				   ((uint64_t)(x) << 56))

/**
 * Helper class that parses numeric values from a file stream.
 * Takes care of byte swapping and I/O error handling.
 */
class ParcaseFileParserStream {
public:
	/// Constructor.
	ParcaseFileParserStream(QIODevice& device) : _device(device), _swap(false) {}

	/// Enables/disables automatic byte swapping.
	void setSwap(bool enable) { _swap = enable; }
	bool swap() const { return _swap; }

	/// Parses a single 32-bit integer. Performs byte swapping if necessary.
	int32_t get_int32() {
		int32_t i;
		if(_device.read(reinterpret_cast<char*>(&i), sizeof(i)) != sizeof(i))
			throw Exception(ParcasFileImporter::tr("PARCAS file parsing error: I/O error."));
		if(_swap) return (int32_t)SWAP32(i);
		else return i;
	}

	/// Parses a single 64-bit integer. Performs byte swapping if necessary.
	int64_t get_int64() {
		int64_t i;
		if(_device.read(reinterpret_cast<char*>(&i), sizeof(i)) != sizeof(i))
			throw Exception(ParcasFileImporter::tr("PARCAS file parsing error: I/O error."));
		if(_swap) return (int64_t)SWAP64(i);
		else return i;
	}

	/// Parses a single 32-bit floating point number. Performs byte swapping if necessary.
	float get_real32() {
		int32_t i = get_int32();
		return *reinterpret_cast<float*>(&i);
	}

	/// Parses a single 64-bit floating point number. Performs byte swapping if necessary.
	double get_real64() {
		int64_t i = get_int64();
		return *reinterpret_cast<double*>(&i);
	}

	/// Reads raw chunk of data (no byte swapping).
	void read(void* buf, size_t size) {
		if(_device.read(static_cast<char*>(buf), size) != size)
			throw Exception(ParcasFileImporter::tr("PARCAS file parsing error: I/O error."));
	}

private:
	QIODevice& _device;
	bool _swap;
};

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool ParcasFileImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	std::unique_ptr<QIODevice> input = file.createIODevice();
	if(!input->open(QIODevice::ReadOnly))
		return false;

	int32_t prot_real = 0;
	int32_t prot_int = 0;

	// Read prototypes.
	input->read(reinterpret_cast<char*>(&prot_real), sizeof(prot_real));
	input->read(reinterpret_cast<char*>(&prot_int), sizeof(prot_int));

	if(prot_int == 0x11223344 || SWAP32(prot_int) == 0x11223344)
		return true;
	else
		return false;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
void ParcasFileImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading Parcas file %1").arg(fileHandle().toString()));

	// Open input file for reading.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	if(!device->open(QIODevice::ReadOnly))
		throw Exception(tr("Failed to open PARCAS file: %1.").arg(device->errorString()));

	// Read in the static part of the header.
	ParcaseFileParserStream stream(*device);

	int32_t prot_real = stream.get_int32();
	int32_t prot_int  = stream.get_int32();
    if(SWAP32(prot_int) == 0x11223344)
    	stream.setSwap(true);

    int32_t fileversion = stream.get_int32();
    int32_t realsize    = stream.get_int32();
    int64_t desc_off    = stream.get_int64();
    int64_t atom_off    = stream.get_int64();
    int32_t frame_num   = stream.get_int32();
    int32_t part_num    = stream.get_int32();
    int32_t total_parts = stream.get_int32();
    int32_t fields      = stream.get_int32();
    int64_t natoms      = stream.get_int64();
    int32_t mintype     = stream.get_int32();
    int32_t maxtype     = stream.get_int32();
    int32_t cpus        = stream.get_int32();
    double simu_time   = stream.get_real64();
    double timescale   = stream.get_real64();
    double box_x       = stream.get_real64();
    double box_y       = stream.get_real64();
    double box_z       = stream.get_real64();

	if(prot_int != 0x11223344 && SWAP32(prot_int) != 0x11223344)
		throw Exception(tr("PARCAS file parsing error: Unknown input byte order."));

    // Do some sanity checking for the fixed header before continuing.
    if(realsize != 4 && realsize != 8)
    	throw Exception(tr("PARCAS file parsing error: Bad real size: %1. Should be either 4 or 8.").arg(realsize));

#if 0
    //qDebug() << "PARCAS file byte order:" << (((machine_order == BO_BIG_ENDIAN && stream.swap() == 0) || (machine_order == BO_LITTLE_ENDIAN && stream.swap() == 1)) ? "big endian" : "little endian");
    qDebug() << "Prototype real:" << prot_real << "(should be" << 0x3f302010 << ")";
    qDebug() << "Prototype integer:" << prot_int << "(should be" << 0x11223344 << ")";
    qDebug() << "File version:" << (int)fileversion;
    qDebug() << "Real size:" << (int)realsize;
    qDebug() << "Description offset:" << desc_off;
    qDebug() << "Atom data offset:" << atom_off;
    qDebug() << "Frame num:" << (int)frame_num;
    qDebug() << "Part num:" << (int)part_num;
    qDebug() << "Total parts:" << (int)total_parts;
    qDebug() << "Number of per-atom fields:" << (int)fields;
    qDebug() << "Number of atoms:" << natoms;
    qDebug() << "itypelow:" << (int)mintype;
    qDebug() << "itypehigh:" << (int)maxtype;
    qDebug() << "Cpus:" << (int)cpus;
    qDebug() << "Simu time:" << simu_time;
    qDebug() << "Timescale:" << timescale;
    qDebug() << "Box-x:" << box_x;
    qDebug() << "Box-y:" << box_y;
    qDebug() << "Box-z:" << box_z;
#endif

    if(natoms > std::numeric_limits<int>::max())
    	throw Exception(tr("PARCAS file parsing error: File contains %1 atoms. OVITO can handle only %2 atoms.").arg(natoms).arg(std::numeric_limits<int>::max()));
    size_t numAtoms = (size_t)natoms;
	setParticleCount(numAtoms);

	state().setAttribute(QStringLiteral("Timestep"), QVariant::fromValue((int)frame_num), dataSource());
	state().setAttribute(QStringLiteral("Time"), QVariant::fromValue(simu_time), dataSource());

	// Create particle properties for extra fields.
	std::vector<PropertyAccess<FloatType>> extraProperties;
    for(int i = 0; i < fields; i++) {
    	char field_name[5], field_unit[5];
    	stream.read(field_name, 4);
    	stream.read(field_unit, 4);
    	field_name[4] = '\0';
    	field_unit[4] = '\0';
#if 0
    	qDebug() << "Field-" << (i+1) << " name: " << field_name << " unit: " << field_unit;
#endif

    	ParticlesObject::Type propertyType = ParticlesObject::UserProperty;
    	QString propertyName = QString(field_name).trimmed();
    	if(propertyName == "Epot") propertyType = ParticlesObject::PotentialEnergyProperty;
    	else if(propertyName == "Ekin") propertyType = ParticlesObject::KineticEnergyProperty;

    	PropertyObject* property;
		if(propertyType != ParticlesObject::UserProperty)
			property = particles()->createProperty(propertyType, DataBuffer::InitializeMemory);
		else
			property = particles()->createProperty(propertyName, PropertyObject::Float, 1, DataBuffer::InitializeMemory);
		extraProperties.emplace_back(property);
    }

    // Set up simulation cell and periodic boundary flags.
    FloatType boxDim[3] = {
    		std::abs((FloatType)box_x),
    		std::abs((FloatType)box_y),
    		std::abs((FloatType)box_z)
    };
	simulationCell()->setCellMatrix(AffineTransformation(
			Vector3(boxDim[0], 0, 0), Vector3(0, boxDim[1], 0), Vector3(0, 0, boxDim[2]),
			Vector3(-boxDim[0]/2, -boxDim[1]/2, -boxDim[2]/2)));
	simulationCell()->setPbcFlags(box_x < 0, box_y < 0, box_z < 0);

	// Create the required standard properties.
	PropertyAccess<Point3> posProperty = particles()->createProperty(ParticlesObject::PositionProperty);
	PropertyAccess<int> typeProperty = particles()->createProperty(ParticlesObject::TypeProperty);
	PropertyAccess<qlonglong> identifierProperty = particles()->createProperty(ParticlesObject::IdentifierProperty);

	// Create particle types list.
    std::vector<std::array<char,5>> types(maxtype - mintype + 1);
    for(int i = mintype; i <= maxtype; i++) {
    	stream.read(types[i - mintype].data(), 4);
    	types[i - mintype][4] = '\0';
		addNumericType(ParticlesObject::OOClass(), typeProperty.buffer(), i, QString::fromUtf8(types[i - mintype].data()).trimmed());
    }

	// The actual header is now parsed. Check the offsets.
    qint64 file_off = device->pos();
    if(file_off > (qint64)desc_off || file_off > (qint64)atom_off || desc_off > atom_off)
    	throw Exception(tr("PARCAS file parsing error: Corrupt offsets"));

    // Seek to the start of atom data.
    if(!device->seek((qint64)atom_off))
    	throw Exception(tr("PARCAS file parsing error: Seek error: %1").arg(device->errorString()));

	setProgressMaximum(numAtoms);

	// Parse atoms.
	for(size_t i = 0; i < numAtoms; i++) {

		// Parse atom id.
		identifierProperty[i] = stream.get_int64();

		// Parse atom type.
		int32_t atomType = std::abs(stream.get_int32());
		OVITO_ASSERT(atomType >= mintype && atomType <= maxtype);
		typeProperty[i] = atomType;

		// Parse atom coordinates.
		Point3& pos = posProperty[i];
		if(realsize == 4) {
			for(int k = 0; k < 3; k++)
				pos[k] = (FloatType)stream.get_real32();
		}
		else {
			for(int k = 0; k < 3; k++)
				pos[k] = (FloatType)stream.get_real64();
		}

		// Parse extra fields.
		if(realsize == 4) {
			for(PropertyAccess<FloatType>& prop : extraProperties)
				prop[i] = (FloatType)stream.get_real32();
		}
		else {
			for(PropertyAccess<FloatType>& prop : extraProperties)
				prop[i] = (FloatType)stream.get_real64();
		}

		// Update progress indicator.
		if(!setProgressValueIntermittent(i)) return;
	}
	posProperty.reset();
	typeProperty.reset();
	identifierProperty.reset();

	// Sort particles by ID if requested.
	if(_sortParticles)
		particles()->sortById();

	state().setStatus(tr("%1 atoms at simulation time %2").arg(numAtoms).arg(simu_time));

	// Call base implementation to finalize the loaded particle data.
	ParticleImporter::FrameLoader::loadFile();
}

}	// End of namespace
