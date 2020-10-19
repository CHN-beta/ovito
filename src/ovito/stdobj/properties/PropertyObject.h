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


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/stdobj/properties/ElementType.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Stores a property data array.
 */
class OVITO_STDOBJ_EXPORT PropertyObject : public DataObject
{
	/// Define a new property metaclass for particle containers.
	class OVITO_STDOBJ_EXPORT OOMetaClass : public DataObject::OOMetaClass
	{
	public:
		/// Inherit constructor from base class.
		using DataObject::OOMetaClass::OOMetaClass;

		/// Generates a human-readable string representation of the data object reference.
		virtual QString formatDataObjectPath(const ConstDataObjectPath& path) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(PropertyObject, OOMetaClass);
	Q_CLASSINFO("DisplayName", "Property");

public:

	/// \brief The most commonly used data types. Note that, at least in principle,
	///        the PropertyStorage class supports any data type registered with the Qt meta type system.
	enum StandardDataType {
#ifndef Q_CC_MSVC
		Int = qMetaTypeId<int>(),
		Int64 = qMetaTypeId<qlonglong>(),
		Float = qMetaTypeId<FloatType>()
#else // MSVC compiler doesn't treat qMetaTypeId() function as constexpr. Workaround:
		Int = QMetaType::Int,
		Int64 = QMetaType::LongLong,
#ifdef FLOATTYPE_FLOAT
		Float = QMetaType::Float
#else
		Float = QMetaType::Double
#endif
#endif
	};

	/// \brief The standard property types defined by all property classes.
	enum GenericStandardType {
		GenericUserProperty = 0,	//< This is reserved for user-defined properties.
		GenericSelectionProperty = 1,
		GenericColorProperty = 2,
		GenericTypeProperty = 3,
		GenericIdentifierProperty = 4,

		// This is value at which type IDs of specific standard properties start:
		FirstSpecificProperty = 1000
	};

public:

	/// \brief Creates an empty property.
	Q_INVOKABLE PropertyObject(DataSet* dataset);

	/// \brief Constructor that creates and initializes a new property array.
	PropertyObject(DataSet* dataset, size_t elementCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, int type = 0, QStringList componentNames = QStringList());

	/// \brief Gets the property's name.
	/// \return The name of property.
	const QString& name() const { return _name; }

	/// \brief Sets the property's name.
	/// \param name The new name string.
	void setName(const QString& name);

	/// \brief Returns the number of elements stored in the property array.
	size_t size() const { return _numElements; }

	/// \brief Resizes the property storage.
	/// \param newSize The new number of elements.
	/// \param preserveData Controls whether the existing per-element data is preserved.
	///                     This also determines whether newly allocated memory is initialized to zero.
	void resize(size_t newSize, bool preserveData);

	/// \brief Grows the number of data elements while preserving the exiting data.
	/// Newly added elements are *not* initialized to zero by this method.
	/// \return True if the memory buffer was reallocated, because the current capacity was insufficient
	/// to accommodate the new elements.
	bool grow(size_t numAdditionalElements) {
		size_t newSize = _numElements + numAdditionalElements;
		OVITO_ASSERT(newSize >= _numElements);
		bool needToGrow;
		if((needToGrow = (newSize > _capacity)))
			growCapacity(newSize);
		_numElements = newSize;
		return needToGrow;
	}

	/// \brief Reduces the number of data elements while preserving the exiting data.
	/// Note: This method never reallocates the memory buffer. Thus, the capacity of the array remains unchanged and the
	/// memory of the truncated elements is not released by the method.
	void truncate(size_t numElementsToRemove) {
		OVITO_ASSERT(numElementsToRemove <= _numElements);
		_numElements -= numElementsToRemove;
	}	

	/// \brief Returns the type of this property.
	int type() const { return _type; }

	/// \brief Changes the type of this property. Note that this method is only for internal use.
	///        Normally, you should not change the type of a property once it was created.
	void setType(int newType) { _type = newType; }

	/// \brief Returns the data type of the property.
	/// \return The identifier of the data type used for the elements stored in
	///         this property storage according to the Qt meta type system.
	int dataType() const { return _dataType; }

	/// \brief Returns the number of bytes per value.
	/// \return Number of bytes used to store a single value of the data type
	///         specified by type().
	size_t dataTypeSize() const { return _dataTypeSize; }

	/// \brief Returns the number of bytes used per element.
	size_t stride() const { return _stride; }

	/// \brief Returns the number of vector components per element.
	/// \return The number of data values stored per element in this storage object.
	size_t componentCount() const { return _componentCount; }

	/// \brief Returns the human-readable names for the vector components if this is a vector property.
	/// \return The names of the vector components if this property contains more than one value per element.
	///         If this is only a single-valued property then an empty list is returned by this method.
	const QStringList& componentNames() const { return _componentNames; }

	/// \brief Sets the human-readable names for the vector components if this is a vector property.
	void setComponentNames(QStringList names) {
		OVITO_ASSERT(names.empty() || names.size() == componentCount());
		_componentNames = std::move(names);
	}

	/// \brief Returns the display name of the property including the name of the given
	///        vector component.
	QString nameWithComponent(int vectorComponent) const {
		if(componentCount() <= 1 || vectorComponent < 0)
			return name();
		else if(vectorComponent < componentNames().size())
			return QStringLiteral("%1.%2").arg(name()).arg(componentNames()[vectorComponent]);
		else
			return QStringLiteral("%1.%2").arg(name()).arg(vectorComponent + 1);
	}

	/// Changes the data type of the property in place and convert the values stored in the property.
	void convertDataType(int newDataType);

	/// \brief Returns a read-only pointer to the raw element data stored in this property array.
	const uint8_t* cbuffer() const {
		return _data.get();
	}

	/// \brief Returns a read-write pointer to the raw element data stored in this property array.
	uint8_t* buffer() {
		return _data.get();
	}

	/// \brief Sets all array elements to the given uniform value.
	template<typename T>
	void fill(const T value) {
		OVITO_ASSERT(stride() == sizeof(T));
		T* begin = reinterpret_cast<T*>(buffer());
		T* end = begin + this->size();
		std::fill(begin, end, value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T value, const PropertyObject& selectionProperty) {
		OVITO_ASSERT(selectionProperty.size() == this->size());
		OVITO_ASSERT(selectionProperty.dataType() == Int);
		OVITO_ASSERT(selectionProperty.componentCount() == 1);
		const int* selectionIter = reinterpret_cast<const int*>(selectionProperty.cbuffer());
		for(T* v = reinterpret_cast<T*>(buffer()), *end = v + this->size(); v != end; ++v)
			if(*selectionIter++) *v = value;
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T& value, const PropertyObject* selectionProperty) {
		if(selectionProperty)
			fillSelected(value, *selectionProperty);
		else
			fill(value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	template<typename T>
	void fillSelected(const T& value, const ConstPropertyPtr& selectionProperty) {
		fillSelected(value, selectionProperty.get());
	}

	// Set all property values to zeros.
	void fillZero() {
		std::memset(_data.get(), 0, this->size() * this->stride());
	}

	/// Extends the data array and replicates the existing data N times.
	void replicate(size_t n, bool replicateValues = true);

	/// Reduces the size of the storage array, removing elements for which
	/// the corresponding bits in the bit array are set.
	void filterResize(const boost::dynamic_bitset<>& mask);

	/// Creates a copy of the array, not containing those elements for which
	/// the corresponding bits in the given bit array were set.
	OORef<PropertyObject> filterCopy(const boost::dynamic_bitset<>& mask) const;

	/// Copies the contents from the given source into this storage using a element mapping.
	void mappedCopyFrom(const PropertyObject& source, const std::vector<size_t>& mapping);

	/// Copies the elements from this storage array into the given destination array using an index mapping.
	void mappedCopyTo(PropertyObject& destination, const std::vector<size_t>& mapping) const;

	/// Copies the data elements from the given source array into this array. 
	/// Array size, component count and data type of source and destination must match exactly.
	void copyFrom(const PropertyObject& source);

	/// Copies a range of data elements from the given source array into this array. 
	/// Component count and data type of source and destination must be compatible.
	void copyRangeFrom(const PropertyObject& source, size_t sourceIndex, size_t destIndex, size_t count);

	/// Copies the values in this property array to the given output iterator if it is compatible.
	/// Returns false if copying was not possible, because the data type of the array and the output iterator
	/// are not compatible.
	template<typename Iter>
	bool copyTo(Iter iter, size_t component = 0) const {
		size_t cmpntCount = componentCount();
		if(component >= cmpntCount) return false;
		if(size() == 0) return true;
		if(dataType() == PropertyObject::Int) {
			for(auto v = reinterpret_cast<const int*>(cbuffer()) + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		else if(dataType() == PropertyObject::Int64) {
			for(auto v = reinterpret_cast<const qlonglong*>(cbuffer()) + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		else if(dataType() == PropertyObject::Float) {
			for(auto v = reinterpret_cast<const FloatType*>(cbuffer()) + component, v_end = v + size()*cmpntCount; v != v_end; v += cmpntCount)
				*iter++ = *v;
			return true;
		}
		return false;
	}

	/// Calls a functor provided by the caller for every value of the given vector component.
	template<typename F>
	bool forEach(size_t component, F f) const {
		size_t cmpntCount = componentCount();
		size_t s = size();
		if(component >= cmpntCount) return false;
		if(s == 0) return true;
		if(dataType() == PropertyObject::Int) {
			auto v = reinterpret_cast<const int*>(cbuffer()) + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		else if(dataType() == PropertyObject::Int64) {
			auto v = reinterpret_cast<const qlonglong*>(cbuffer()) + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		else if(dataType() == PropertyObject::Float) {
			auto v = reinterpret_cast<const FloatType*>(cbuffer()) + component;
			for(size_t i = 0; i < s; i++, v += cmpntCount)
				f(i, *v);
			return true;
		}
		return false;
	}

	/// Checks if this property storage and its contents exactly match those of another property storage.
	bool equals(const PropertyObject& other) const;
	
	//////////////////////////////// Element types //////////////////////////////

	/// Appends an element type to the list of types.
	void addElementType(const ElementType* type) {
		OVITO_ASSERT(elementTypes().contains(const_cast<ElementType*>(type)) == false);
		_elementTypes.push_back(this, PROPERTY_FIELD(elementTypes), type);
	}

	/// Inserts an element type into the list of types.
	void insertElementType(int index, const ElementType* type) {
		OVITO_ASSERT(elementTypes().contains(const_cast<ElementType*>(type)) == false);
		_elementTypes.insert(this, PROPERTY_FIELD(elementTypes), index, type);
	}

	/// Returns the element type with the given ID, or NULL if no such type exists.
	ElementType* elementType(int id) const {
		for(ElementType* type : elementTypes())
			if(type->numericId() == id)
				return type;
		return nullptr;
	}

	/// Returns the element type with the given human-readable name, or NULL if no such type exists.
	ElementType* elementType(const QString& name) const {
		OVITO_ASSERT(!name.isEmpty());
		for(ElementType* type : elementTypes())
			if(type->name() == name)
				return type;
		return nullptr;
	}

	/// Removes a single element type from this object.
	void removeElementType(int index) {
		_elementTypes.remove(this, PROPERTY_FIELD(elementTypes), index);
	}

	/// Removes all elements types from this object.
	void clearElementTypes() {
		_elementTypes.clear(this, PROPERTY_FIELD(elementTypes));
	}

	/// Builds a mapping from numeric IDs to type colors.
	std::map<int,Color> typeColorMap() const {
		std::map<int,Color> m;
		for(ElementType* type : elementTypes())
			m.insert({type->numericId(), type->color()});
		return m;
	}

	/// Returns an numeric type ID that is not yet used by any of the existing element types.
	int generateUniqueElementTypeId(int startAt = 1) const {
		int maxId = startAt;
		for(ElementType* type : elementTypes())
			maxId = std::max(maxId, type->numericId() + 1);
		return maxId;
	}

	/// Helper method that remaps the existing type IDs to a contiguous range starting at the given
	/// base ID. This method is mainly used for file output, because some file formats
	/// work with numeric particle types only, which must form a contiguous range.
	/// The method returns the mapping of output type IDs to original type IDs
	/// and a copy of the property array in which the original type ID values have
	/// been remapped to the output IDs.
	std::tuple<std::map<int,int>, ConstPropertyPtr> generateContiguousTypeIdMapping(int baseId = 1) const;

	////////////////////////////// Support functions for the Python bindings //////////////////////////////

	/// Indicates to the Python binding layer that this property object has been temporarily put into a
	/// writable state. In this state, the binding layer will allow write access to the property's internal data.
	bool isWritableFromPython() const { return _isWritableFromPython != 0; }

	/// Puts the property array into a writable state.
	/// In the writable state, the Python binding layer will allow write access to the property's internal data.
	void makeWritableFromPython();

	/// Puts the property array back into the default read-only state.
	/// In the read-only state, the Python binding layer will not permit write access to the property's internal data.
	void makeReadOnlyFromPython() {
		OVITO_ASSERT(_isWritableFromPython > 0);
		_isWritableFromPython--;
	}

	/// Returns whether this data object wants to be shown in the pipeline editor
	/// under the data source section.
	/// This implementation returns true only it this is a typed property, i.e. if the 'elementTypes' list contains
	/// some elements. In this case we want the property to appear in the pipeline editor so that the user can
	/// edit the individual types.
	virtual bool showInPipelineEditor() const override {
		return !elementTypes().empty();
	}

	/// Returns the display title of this property object in the user interface.
	virtual QString objectTitle() const override;

protected:

	/// Grows the storage buffer to accomodate at least the given number of data elements.
	void growCapacity(size_t newSize);

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) const override;

private:

	/// Contains the list of defined "types" if this is a typed property.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ElementType, elementTypes, setElementTypes);

	/// The user-interface title of this property.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// The type of this property.
	int _type = 0;

	/// The name of the property.
	QString _name;

	/// The data type of the property (a Qt metadata type identifier).
	int _dataType = QMetaType::Void;

	/// The number of bytes per data type value.
	size_t _dataTypeSize = 0;

	/// The number of elements in the property storage.
	size_t _numElements = 0;

	/// The capacity of the allocated buffer.
	size_t _capacity = 0;

	/// The number of bytes per element.
	size_t _stride = 0;

	/// The number of vector components per element.
	size_t _componentCount = 0;

	/// The names of the vector components if this property consists of more than one value per element.
	QStringList _componentNames;

	/// The internal memory buffer holding the data elements.
	std::unique_ptr<uint8_t[]> _data;

	/// This is a special flag used by the Python bindings to indicate that
	/// this property object has been temporarily put into a writable state.
	int _isWritableFromPython = 0;
};

/// Smart-pointer to a PropertyObject.
using PropertyPtr = DataOORef<PropertyObject>;

/// Smart-pointer to a PropertyObject providing read-only access to the property data.
using ConstPropertyPtr = DataOORef<const PropertyObject>;

/// Class template returning the Qt data type identifier for the components in the given C++ array structure.
template<typename T, typename = void> struct PropertyStoragePrimitiveDataType { static constexpr int value = qMetaTypeId<T>(); };
#ifdef Q_CC_MSVC // MSVC compiler doesn't treat qMetaTypeId() function as constexpr. Workaround:
template<> struct PropertyStoragePrimitiveDataType<int> { static constexpr PropertyObject::StandardDataType value = PropertyObject::Int; };
template<> struct PropertyStoragePrimitiveDataType<qlonglong> { static constexpr PropertyObject::StandardDataType value = PropertyObject::Int64; };
template<> struct PropertyStoragePrimitiveDataType<FloatType> { static constexpr PropertyObject::StandardDataType value = PropertyObject::Float; };
#endif
template<typename T, std::size_t N> struct PropertyStoragePrimitiveDataType<std::array<T,N>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Point_3<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Vector_3<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Point_2<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Vector_2<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<Matrix_3<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<QuaternionT<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<ColorT<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<SymmetricTensor2T<T>> : public PropertyStoragePrimitiveDataType<T> {};
template<typename T> struct PropertyStoragePrimitiveDataType<T, typename std::enable_if<std::is_enum<T>::value>::type> : public PropertyStoragePrimitiveDataType<std::make_signed_t<std::underlying_type_t<T>>> {};


}	// End of namespace
}	// End of namespace
