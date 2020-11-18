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

#pragma once

#include <ovito/core/Core.h>
#include "DataObject.h"

namespace Ovito {

/// Utility class that is used to reference a particular data object in a DataCollection
/// as a path through the hierarchy of nested data objects.
class OVITO_CORE_EXPORT ConstDataObjectPath : public QVarLengthArray<const DataObject*, 3>
{
public:

	/// Inherit constructors from base class.
	using QVarLengthArray::QVarLengthArray;

	/// Converts the path to a string representation.
	QString toString() const;

	/// Returns a string representation of the object path that is suitable for display in the user interface.
	QString toUIString() const;
};

/// Utility class that is used to reference a particular data object in a DataCollection
/// as a path through the hierarchy of nested data objects.
class OVITO_CORE_EXPORT DataObjectPath : public QVarLengthArray<DataObject*, 3>
{
public:

	/// Inherit constructors from base class.
	using QVarLengthArray::QVarLengthArray;

	/// A path to a mutable object can be implicitly converted to a path to a constant object.
	operator const ConstDataObjectPath&() const {
		return *reinterpret_cast<const ConstDataObjectPath*>(this);
	}

	/// Converts the path to a string representation.
	QString toString() const { return static_cast<const ConstDataObjectPath&>(*this).toString(); }

	/// Returns a string representation of the object path that is suitable for display in the user interface.
	QString toUIString() const { return static_cast<const ConstDataObjectPath&>(*this).toUIString(); }
};

/**
 * \brief A reference to a DataObject in a PipelineFlowState.
 */
class OVITO_CORE_EXPORT DataObjectReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	DataObjectReference() = default;

	/// \brief Constructs a reference to a data object.
	DataObjectReference(DataObjectClassPtr dataClass, const QString& dataPath = QString(), const QString& dataTitle = QString()) :
		_dataClass(dataClass), _dataPath(dataPath), _dataTitle(dataTitle) {}

	/// \brief Constructs a reference to a data object from a data object path.
	DataObjectReference(const ConstDataObjectPath& path) : DataObjectReference(path.empty() ? nullptr : &path.back()->getOOMetaClass(), path.toString(), path.toUIString()) {}

	/// Returns the DataObject subclass being referenced.
	DataObjectClassPtr dataClass() const { return _dataClass; }

	/// Returns the identifier and path of the data object being referenced.
	const QString& dataPath() const { return _dataPath; }

	/// Returns the title of the data object used in the user interface.
	const QString& dataTitle() const { return _dataTitle; }

	/// \brief Compares two references for equality.
	bool operator==(const DataObjectReference& other) const {
		return dataClass() == other.dataClass() && (dataPath() == other.dataPath() || dataPath().isEmpty() || other.dataPath().isEmpty());
	}

	/// \brief Compares two references for inequality.
	bool operator!=(const DataObjectReference& other) const { return !(*this == other); }

	/// \brief Returns whether this reference points to any data object.
	explicit operator bool() const {
		return dataClass() != nullptr;
	}

private:

	/// The DataObject subclass being referenced.
	DataObjectClassPtr _dataClass = nullptr;

	/// The identifier and path of the data object being referenced.
	QString _dataPath;

	/// The title of the data object used in the user interface (optional).
	QString _dataTitle;

	friend OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const DataObjectReference& r);
	friend OVITO_CORE_EXPORT LoadStream& operator>>(LoadStream& stream, DataObjectReference& r);
};

/// Writes a DataObjectReference to an output stream.
/// \relates DataObjectReference
inline OVITO_CORE_EXPORT SaveStream& operator<<(SaveStream& stream, const DataObjectReference& r)
{
	stream.beginChunk(0x02);
	stream << r.dataClass();
	stream << r.dataPath();
	stream << r.dataTitle();
	stream.endChunk();
	return stream;
}

/// Reads a DataObjectReference from an input stream.
/// \relates DataObjectReference
inline OVITO_CORE_EXPORT LoadStream& operator>>(LoadStream& stream, DataObjectReference& r)
{
	stream.expectChunk(0x02);
	stream >> r._dataClass;
	stream >> r._dataPath;
	stream >> r._dataTitle;
	if(!r._dataClass)
		r._dataPath.clear();
	stream.closeChunk();
	// For backward compatibility with OVITO 3.2.1: The SpatialBinningModifier used to generate a VoxelGrid
	// with ID of the form "binning[<PROPERTY>]", but now the grid's ID is just "binning". We automatically
	// update references to the voxel grid when loading an OVITO file written by an old program version.
	if(stream.formatVersion() < 30006 && r._dataPath.startsWith(QStringLiteral("binning[")))
		r._dataPath = QStringLiteral("binning");
	return stream;
}

/**
 * A reference to a DataObject subclass.
 */
template<class DataObjectType>
class TypedDataObjectReference : public DataObjectReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	TypedDataObjectReference() = default;

	/// \brief Conversion copy constructor.
	TypedDataObjectReference(const DataObjectReference& other) : DataObjectReference(other) {
		OVITO_ASSERT(!DataObjectReference::dataClass() || DataObjectReference::dataClass()->isDerivedFrom(DataObjectType::OOClass()));
	}

	/// \brief Conversion move constructor.
	TypedDataObjectReference(DataObjectReference&& other) : DataObjectReference(std::move(other)) {
		OVITO_ASSERT(!DataObjectReference::dataClass() || DataObjectReference::dataClass()->isDerivedFrom(DataObjectType::OOClass()));
	}

	/// \brief Constructs a reference to a data object.
	TypedDataObjectReference(const typename DataObjectType::OOMetaClass* dataClass, const QString& dataPath = QString(), const QString& dataTitle = QString()) : DataObjectReference(dataClass, dataPath, dataTitle) {}

	/// \brief Constructs a reference to a data object from an existing data path.
	TypedDataObjectReference(const ConstDataObjectPath& path) : DataObjectReference(path) {
		OVITO_ASSERT(!dataClass() || dataClass()->isDerivedFrom(DataObjectType::OOClass()));
	}

	/// Returns the DataObject subclass being referenced.
	const typename DataObjectType::OOMetaClass* dataClass() const {
		return static_cast<const typename DataObjectType::OOMetaClass*>(DataObjectReference::dataClass());
	}

	friend SaveStream& operator<<(SaveStream& stream, const TypedDataObjectReference& r) {
		return stream << static_cast<const DataObjectReference&>(r);
	}

	friend LoadStream& operator>>(LoadStream& stream, TypedDataObjectReference& r) {
		return stream >> static_cast<DataObjectReference&>(r);
	}
};

}	// End of namespace

Q_DECLARE_METATYPE(Ovito::DataObjectReference);
