////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/stdobj/properties/PropertyStorage.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/**
 * \brief Defines the mapping between one column of an particle input file and
 *        one of OVITO's particle properties.
 *
 * An InputColumnMapping is composed of a list of these structures, one for each
 * column in the input file.
 */
class OVITO_PARTICLES_EXPORT InputColumnInfo
{
public:

	/// \brief Default constructor mapping the column to no property.
	InputColumnInfo() = default;

	/// \brief Constructor mapping the column to a standard property.
	InputColumnInfo(ParticlesObject::Type type, int vectorComponent = 0) {
		mapStandardColumn(type, vectorComponent);
	}

	/// \brief Constructor mapping the column to a user-defined property.
	InputColumnInfo(const QString& propertyName, int dataType, int vectorComponent = 0) {
		mapCustomColumn(propertyName, dataType, vectorComponent);
	}

	/// \brief Maps this column to a custom particle property.
	/// \param propertyName The name of target particle property.
	/// \param dataType The data type of the property to create.
	/// \param vectorComponent The component of the per-particle vector.
	void mapCustomColumn(const QString& propertyName, int dataType, int vectorComponent = 0) {
		this->property = ParticlePropertyReference(propertyName, vectorComponent);
		this->dataType = dataType;
	}

	/// \brief Maps this column to a standard particle property.
	/// \param type Specifies the standard property.
	/// \param vectorComponent The component in the per-particle vector.
	void mapStandardColumn(ParticlesObject::Type type, int vectorComponent = 0) {
		OVITO_ASSERT(type != ParticlesObject::UserProperty);
		this->property = ParticlePropertyReference(type, vectorComponent);
		this->dataType = ParticlesObject::OOClass().standardPropertyDataType(type);
	}

	/// \brief Returns true if the file column is mapped to a particle property; false otherwise (file column will be ignored during import).
	bool isMapped() const { return dataType != QMetaType::Void; }

	/// \brief Indicates whether this column is mapped to a particle type property.
	bool isTypeProperty() const {
		return (property.type() == ParticlesObject::TypeProperty)
				|| (property.type() == ParticlesObject::StructureTypeProperty);
	}

	/// The target particle property this column is mapped to.
	ParticlePropertyReference property;

	/// The data type of the particle property if this column is mapped to a user-defined property.
	/// This field can be set to QMetaType::Void to indicate that the column should be ignored during file import.
	int dataType = QMetaType::Void;

	/// The name of the column in the input file. This information is
	/// read from the input file (if available).
	QString columnName;
};

/**
 * \brief Defines a mapping between the columns in a column-based input particle file
 *        and OVITO's internal particle properties.
 */
class OVITO_PARTICLES_EXPORT InputColumnMapping : public std::vector<InputColumnInfo>
{
public:

	/// \brief Saves the mapping to a stream.
	void saveToStream(SaveStream& stream) const;

	/// \brief Loads the mapping from a stream.
	void loadFromStream(LoadStream& stream);

	/// \brief Saves the mapping into a byte array.
	QByteArray toByteArray() const;

	/// \brief Loads the mapping from a byte array.
	void fromByteArray(const QByteArray& array);

	/// \brief Checks if the mapping is valid; throws an exception if not.
	void validate() const;

	/// \brief Returns the first few lines of the file, which can help the user to figure out
	///        the column mapping.
	const QString& fileExcerpt() const { return _fileExcerpt; }

	/// \brief Stores the first few lines of the file, which can help the user to figure out
	///        the column mapping.
	void setFileExcerpt(const QString& text) { _fileExcerpt = text; }

	/// \brief Returns true if an input column has been mapped to the Position.Z property.
	///
	/// This method can be used to detect 2D datasets.
	bool hasZCoordinates() const {
		return std::any_of(begin(), end(), [](const InputColumnInfo& column) {
			return column.property.type() == ParticlesObject::PositionProperty && column.property.vectorComponent() == 2;
		});
	}

	/// \brief Returns whether at least some of the file columns have names.
	bool hasFileColumnNames() const {
		return std::any_of(begin(), end(), [](const InputColumnInfo& column) {
			return column.columnName.isEmpty() == false;
		});
	}

private:

	/// A string with the first few lines of the file, which is meant as a hint for the user to figure out
	/// the column mapping.
	QString _fileExcerpt;
};


/**
 * \brief Helper class that reads column-based data from an input file and
 *        stores the parsed values in particles properties according to an InputColumnMapping.
 */
class OVITO_PARTICLES_EXPORT InputColumnReader : public QObject
{
public:

	/// \brief Initializes the object.
	/// \param mapping Defines the mapping between the columns in the input file
	///        and the internal particle properties.
	/// \param destination The object where the read data will be stored in.
	/// \param particleCount The number of particles that will be read from the input file.
	/// \throws Exception if the mapping is not valid.
	///
	/// This constructor creates all necessary data channels in the destination object as defined
 	/// by the column to channel mapping.
	InputColumnReader(const InputColumnMapping& mapping, ParticleFrameData& destination, size_t particleCount);

	/// \brief Parses the string tokens from one line of the input file and stores the values
	///        in the property objects.
	/// \param particleIndex The line index starting at 0 that specifies the particle whose properties
	///                  are read from the input file.
	/// \param dataLine The text line read from the input file containing the field values.
	void readParticle(size_t particleIndex, const char* dataLine);

	/// \brief Parses the string tokens from one line of the input file and stores the values
	///        in the property objects.
	/// \param particleIndex The line index starting at 0 that specifies the particle whose properties
	///                  are read from the input file.
	/// \param dataLine The text line read from the input file containing the field values.
	const char* readParticle(size_t particleIndex, const char* dataLine, const char* dataLineEnd);

	/// \brief Processes the values from one line of the input file and stores them in the particle properties.
	void readParticle(size_t particleIndex, const double* values, int nvalues);

	/// \brief Sorts the created particle types either by numeric ID or by name, depending on how they were stored in the input file.
	void sortParticleTypes();

private:

	/// Parse a single field from a text line.
	void parseField(size_t particleIndex, int columnIndex, const char* token, const char* token_end);

	/// Determines which input data columns are stored in what properties.
	InputColumnMapping _mapping;

	/// The data container.
	ParticleFrameData& _destination;

	struct TargetPropertyRecord {
		PropertyPtr property;
		uint8_t* data;
		size_t stride;
		size_t count;
		int vectorComponent;
		int dataType;
		ParticleFrameData::TypeList* typeList = nullptr;
		bool numericParticleTypes;
	};

	/// Stores the destination particle properties.
	QVector<TargetPropertyRecord> _properties;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::InputColumnInfo);
Q_DECLARE_METATYPE(Ovito::Particles::InputColumnMapping);
