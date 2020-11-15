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
#include "PropertyObject.h"

#include <boost/range/adaptor/strided.hpp>

namespace Ovito { namespace StdObj {

namespace detail {

// Base class that stores a pointer to an underlying PropertyObject.
template<class PointerType, bool Writable>
class PropertyAccessBase
{
protected:

	/// (Smart-)pointer to the PropertyObject whose data is being accessed.
	PointerType _property{};	

	/// Constructor that creates an invalid access object not associated with any property object.
	PropertyAccessBase() noexcept = default;

	/// Constructor that associates the access object with a property object (may be null).
	PropertyAccessBase(PointerType property) noexcept : _property(std::move(property)) {
#ifdef OVITO_DEBUG
		if(this->_property) {
			if(Writable) this->_property->prepareWriteAccess();
			else this->_property->prepareReadAccess();
		}
#endif
	}

	/// Copy construction (only available for read-only accessors).
	PropertyAccessBase(const PropertyAccessBase& other) : _property(other._property) {
#ifdef OVITO_DEBUG
		if(this->_property) {
			if(Writable) this->_property->prepareWriteAccess();
			else this->_property->prepareReadAccess();
		}
#endif
	}

	/// Copy assignment.
	PropertyAccessBase& operator=(const PropertyAccessBase& other) {
		this->_property = other._property;
#ifdef OVITO_DEBUG
		if(this->_property) {
			if(Writable) this->_property->prepareWriteAccess();
			else this->_property->prepareReadAccess();
		}
#endif
		return *this;
	}

	/// Move construction.
	PropertyAccessBase(PropertyAccessBase&& other) noexcept : _property(std::exchange(other._property, nullptr)) {}

	/// Move assignment.
	PropertyAccessBase& operator=(PropertyAccessBase&& other) noexcept {
		this->_property = std::exchange(other._property, nullptr);
		return *this;
	}

#ifdef OVITO_DEBUG
	/// Destructor sets the internal storage pointer to null to easier detect invalid memory access.
	~PropertyAccessBase() { reset(); }
#endif

public:

	/// \brief Returns the number of elements in the property array.
	size_t size() const { 
		OVITO_ASSERT(this->_property);
		return this->_property->size(); 
	}

	/// \brief Returns the number of vector components per element.
	size_t componentCount() const { 
		OVITO_ASSERT(this->_property);
		return this->_property->componentCount(); 
	}

	/// \brief Returns the number of bytes per element.
	size_t stride() const { 
		OVITO_ASSERT(this->_property);
		return this->_property->stride(); 
	}

	/// \brief Returns the number of bytes per vector component.
	size_t dataTypeSize() const { 
		OVITO_ASSERT(this->_property);
		return this->_property->dataTypeSize(); 
	}

	/// \brief Returns the data type of the property.
	int dataType() const { 
		OVITO_ASSERT(this->_property);
		return this->_property->dataType(); 
	}

	/// \brief Returns the type of this property.
	int type() const { 
		OVITO_ASSERT(this->_property);
		return this->_property->type(); 
	}

	/// \brief Returns whether this accessor object points to a valid PropertyStorage. 
	explicit operator bool() const noexcept {
		return (bool)this->_property;
	}

	/// \brief Returns the property object which is being accessed by this class.
	const PointerType& property() const {
		return this->_property;
	}

	/// \brief Detaches the accessor object from the underlying PropertyStorage.
	void reset() {
#ifdef OVITO_DEBUG
		if(this->_property) {
			if(Writable) this->_property->finishWriteAccess();
			else this->_property->finishReadAccess();
		}
#endif
		this->_property = nullptr;
	}
};

// Base class that allows read access to the data elements of the underlying PropertyObject.
template<typename T, class PointerType, bool Writable = false>
class ReadOnlyPropertyAccessBase : public PropertyAccessBase<PointerType, Writable>
{
public:

	using iterator = const T*;
	using const_iterator = const T*;

	/// \brief Returns the value of the i-th element from the array.
	const T& get(size_t i) const {
		OVITO_ASSERT(i < this->size());
		return *(this->cbegin() + i);
	}

	/// \brief Indexed access to the elements of the array.
	const T& operator[](size_t i) const {
		return this->get(i);
	}

	/// \brief Returns a range of const iterators over the elements stored in this array.
	boost::iterator_range<const T*> crange() const {
		return boost::make_iterator_range(cbegin(), cend());
	}

	/// \brief Returns a pointer to the first element of the property array.
	const T* begin() const {
		return cbegin();
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	const T* end() const {
		return cend();
	}

	/// \brief Returns a pointer to the first element of the property array.
	const T* cbegin() const {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(this->_property->dataType() == PropertyStoragePrimitiveDataType<T>::value);
		OVITO_ASSERT(this->stride() == sizeof(T));
		return reinterpret_cast<const T*>(this->_property->cbuffer());
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	const T* cend() const {
		return cbegin() + this->size();
	}

protected:

	/// Constructor that creates an invalid access object not associated with any PropertyObject.
	ReadOnlyPropertyAccessBase() {}

	/// Constructor that associates the access object with a PropertyObject (may be null).
	ReadOnlyPropertyAccessBase(PointerType property) : PropertyAccessBase<PointerType, Writable>(std::move(property)) {
		OVITO_ASSERT(!this->_property || this->_property->stride() == sizeof(T));
		OVITO_ASSERT(!this->_property || this->_property->dataType() == PropertyStoragePrimitiveDataType<T>::value);
	}
};

// Base class that allows read access to the individual components of vector elements of the underlying PropertyObject.
template<typename T, class PointerType, bool Writable = false>
class ReadOnlyPropertyAccessBaseTable : public PropertyAccessBase<PointerType, Writable>
{
public:

	using iterator = const T*;
	using const_iterator = const T*;

	/// \brief Returns the value of the i-th element from the array.
	const T& get(size_t i, size_t j) const {
		OVITO_ASSERT(i < this->size());
		OVITO_ASSERT(j < this->componentCount());
		return *(this->cbegin() + (i * this->componentCount()) + j);
	}

	/// \brief Returns a pointer to the beginning of the property array.
	const T* cbegin() const {
		return reinterpret_cast<const T*>(this->_property->cbuffer());
	}

	/// \brief Returns a pointer to the end of the property array.
	const T* cend() const {
		return this->cbegin() + (this->size() * this->componentCount());
	}

	/// \brief Returns a range of iterators over the i-th vector component of all elements stored in this array.
	auto componentRange(size_t componentIndex) const {
		OVITO_ASSERT(this->componentCount() > componentIndex);
		const T* begin = cbegin() + componentIndex;
		return boost::adaptors::stride(boost::make_iterator_range(begin, begin + (this->size() * this->componentCount())), this->componentCount());
	}

protected:

	/// Constructor that creates an invalid access object not associated with any PropertyObject.
	ReadOnlyPropertyAccessBaseTable() {}

	/// Constructor that associates the access object with a PropertyStorage (may be null).
	ReadOnlyPropertyAccessBaseTable(PointerType property) : PropertyAccessBase<PointerType, Writable>(std::move(property)) {
		OVITO_ASSERT(!this->_property || this->_property->stride() == sizeof(T) * this->_property->componentCount());
		OVITO_ASSERT(!this->_property || this->_property->dataType() == qMetaTypeId<T>());
		OVITO_ASSERT(!this->_property || this->_property->dataTypeSize() == sizeof(T));
	}
};

// Base class that allows read access to the raw data of the underlying PropertyObject.
template<class PointerType, bool Writable>
class ReadOnlyPropertyAccessBaseTable<void, PointerType, Writable> : public PropertyAccessBase<PointerType, Writable>
{
public:

	/// \brief Returns the j-th component of the i-th element in the array.
	template<typename U>
	U get(size_t i, size_t j) const {
		switch(this->dataType()) {
		case PropertyObject::Float:
			return static_cast<U>(*reinterpret_cast<const FloatType*>(this->cdata(j) + i * this->stride()));
		case PropertyObject::Int:
			return static_cast<U>(*reinterpret_cast<const int*>(this->cdata(j) + i * this->stride()));
		case PropertyObject::Int64:
			return static_cast<U>(*reinterpret_cast<const qlonglong*>(this->cdata(j) + i * this->stride()));
		default:
			OVITO_ASSERT(false);
			throw Exception(QStringLiteral("Cannot read value from property '%1', because it has a non-standard data type.").arg(this->_property->name()));
		}
	}

	/// \brief Returns a pointer to the raw data of the property array.
	const uint8_t* cdata(size_t component = 0) const {
		OVITO_ASSERT(this->_property);
		return this->_property->cbuffer() + (component * this->dataTypeSize());
	}

	/// \brief Returns a pointer to the raw data of the property array.
	const uint8_t* cdata(size_t index, size_t component) const {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(index < this->size());
		OVITO_ASSERT(component < this->componentCount());
		return this->_property->cbuffer() + (index * this->stride()) + (component * this->dataTypeSize());
	}

protected:

	// Inherit constructors from base class.
	using PropertyAccessBase<PointerType, Writable>::PropertyAccessBase;
};

// Base class that allows read/write access to the data elements of the underlying PropertyObject.
template<typename T, class PointerType>
class ReadWritePropertyAccessBase : public ReadOnlyPropertyAccessBase<T, PointerType, true>
{
public:

	using iterator = T*;
	using const_iterator = T*;

	/// \brief Sets the value of the i-th element in the array.
	void set(size_t i, const T& v) {
		OVITO_ASSERT(i < this->size());
		*(this->begin() + i) = v;
	}

	/// \brief Indexed access to the elements of the array.
	T& operator[](size_t i) {
		OVITO_ASSERT(i < this->size());
		return *(this->begin() + i);
	}

	/// \brief Indexed access to the elements of the array.
	const T& operator[](size_t i) const {
		OVITO_ASSERT(i < this->size());
		return *(this->cbegin() + i);
	}

	/// \brief Returns a pointer to the first element of the property array.
	T* begin() const {
		OVITO_ASSERT(this->_property);
		return reinterpret_cast<T*>(this->_property->buffer());
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	T* end() const {
		return this->begin() + this->size();
	}

	/// \brief Returns a range of iterators over the elements stored in this array.
	boost::iterator_range<T*> range() {
		return boost::make_iterator_range(begin(), end());
	}

	/// \brief Sets all array elements to the given uniform value.
	void fill(const T& value) {
		OVITO_ASSERT(this->_property);
		this->_property->template fill<T>(value);
	}

	/// \brief Sets all array elements for which the corresponding entries in the 
	///        selection array are non-zero to the given uniform value.
	void fillSelected(const T& value, const PropertyObject* selectionProperty) {
		OVITO_ASSERT(this->_property);
		this->_property->template fillSelected<T>(value, selectionProperty);
	}

	/// Copies the data from the given source array to this array. 
	/// The array size and data type of source and destination must match.
	template<class PointerType2>
	void copyFrom(const ReadOnlyPropertyAccessBase<T, PointerType2>& source) {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(source._property);
		this->_property->copyFrom(*source_property);
	}

protected:

	// Inherit constructors from base class.
	using ReadOnlyPropertyAccessBase<T, PointerType, true>::ReadOnlyPropertyAccessBase;
};

// Base class that allows read/write access to the individual components of the vector elements of the underlying PropertyObject.
template<typename T, class PointerType>
class ReadWritePropertyAccessBaseTable : public ReadOnlyPropertyAccessBaseTable<T, PointerType, true>
{
public:

	using iterator = T*;
	using const_iterator = T*;

	/// \brief Returns a pointer to the first element of the property array.
	T* begin() const {
		OVITO_ASSERT(this->_property);
		return reinterpret_cast<T*>(this->_property->buffer());
	}

	/// \brief Returns a pointer pointing to the end of the property array.
	T* end() const {
		OVITO_ASSERT(this->stride() == sizeof(T) * this->componentCount());
		return this->begin() + (this->size() * this->componentCount());
	}

	/// \brief Returns a range of iterators over the i-th vector component of all elements stored in this array.
	auto componentRange(size_t componentIndex) {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(this->_property->componentCount() > componentIndex);
		T* begin = this->begin() + componentIndex;
		return boost::adaptors::stride(boost::make_iterator_range(begin, begin + (this->size() * this->componentCount())), this->componentCount());
	}

	/// \brief Returns a range of iterators over the elements stored in this array.
	boost::iterator_range<T*> range() {
		return boost::make_iterator_range(begin(), end());
	}

	/// \brief Sets the j-th component of the i-th element of the array to a new value.
	void set(size_t i, size_t j, const T& value) {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(i < this->size());
		OVITO_ASSERT(j < this->componentCount());
		*(begin() + i * this->componentCount() + j) = value;
	}

	/// \brief Returns a modifiable reference to the j-th component of the i-th element of the array.
	T& value(size_t i, size_t j) {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(i < this->size());
		OVITO_ASSERT(j < this->componentCount());
		return *(begin() + i * this->componentCount() + j);
	}

protected:

	// Inherit constructors from base class.
	using ReadOnlyPropertyAccessBaseTable<T, PointerType, true>::ReadOnlyPropertyAccessBaseTable;
};

// Base class that allows read/write access to the raw data of the underlying PropertyObject.
template<class PointerType>
class ReadWritePropertyAccessBaseTable<void, PointerType> : public ReadOnlyPropertyAccessBaseTable<void, PointerType, true>
{
public:

	/// \brief Sets the j-th component of the i-th element of the array to a new value.
	template<typename U>
	void set(size_t i, size_t j, const U& value) {
		OVITO_ASSERT(this->_property);
		switch(this->_property->dataType()) {
		case PropertyObject::Float:
			*reinterpret_cast<FloatType*>(this->data(j) + i * this->stride()) = value;
			break;
		case PropertyObject::Int:
			*reinterpret_cast<int*>(this->data(j) + i * this->stride()) = value;
			break;
		case PropertyObject::Int64:
			*reinterpret_cast<qlonglong*>(this->data(j) + i * this->stride()) = value;
			break;
		default:
			OVITO_ASSERT(false);
			throw Exception(QStringLiteral("Cannot assign value to property '%1', because it has a non-standard data type.").arg(this->_property->name()));
		}
	}

	/// \brief Returns a pointer to the raw data of the property array.
	uint8_t* data(size_t component = 0) {
		OVITO_ASSERT(this->_property);
		return this->_property->buffer() + (component * this->dataTypeSize());
	}

	/// \brief Returns a pointer to the raw data of the property array.
	uint8_t* data(size_t index, size_t component) {
		OVITO_ASSERT(this->_property);
		OVITO_ASSERT(index < this->size());
		OVITO_ASSERT(component < this->componentCount());
		return this->_property->buffer() + (index * this->stride()) + (component * this->dataTypeSize());
	}

protected:

	// Inherit constructors from base class.
	using ReadOnlyPropertyAccessBaseTable<void, PointerType, true>::ReadOnlyPropertyAccessBaseTable;
};

} // End of namespace detail.

/**
 * \brief Helper class that provides read access to the data elements of a PropertyObject.
 * 
 * The TableMode template parameter should be set to true if access to the individual components
 * of a vector property array is desired or if the number of vector components of the property is unknown at compile time. 
 * If TableMode is set to false, the data elements can only be access as a whole and the number of components must
 * be a compile-time constant.
 */
template<typename T, bool TableMode = false>
class ConstPropertyAccess : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadOnlyPropertyAccessBaseTable<T, const PropertyObject*>, Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, const PropertyObject*>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadOnlyPropertyAccessBaseTable<T, const PropertyObject*>, Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, const PropertyObject*>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyObject.
	ConstPropertyAccess() = default;

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccess(const PropertyObject* propertyObj) 
		: ParentType(propertyObj) {}

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccess(const ConstPropertyPtr& property)
		: ParentType(property) {}

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccess(const PropertyPtr& property)
		: ParentType(property) {}
};

/**
 * \brief Helper class that provides read access to the data elements in a PropertyObject
 *        and which keeps a strong reference to the PropertyObject.
 */
template<typename T, bool TableMode = false>
class ConstPropertyAccessAndRef : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadOnlyPropertyAccessBaseTable<T, ConstPropertyPtr>, Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, ConstPropertyPtr>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadOnlyPropertyAccessBaseTable<T, ConstPropertyPtr>, Ovito::StdObj::detail::ReadOnlyPropertyAccessBase<T, ConstPropertyPtr>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyObject.
	ConstPropertyAccessAndRef() = default;

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccessAndRef(ConstPropertyPtr property)
		: ParentType(std::move(property)) {}

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccessAndRef(PropertyPtr property)
		: ParentType(std::move(property)) {}

	/// Constructs a read-only accessor for the data in a PropertyObject.
	ConstPropertyAccessAndRef(const PropertyObject* property)
		: ParentType(ConstPropertyPtr(property)) {}

	/// \brief Moves the internal PropertyPtr out of this object.
	ConstPropertyPtr take() {
		return std::move(this->_property);
	}
};

/**
 * \brief Helper class that provides read/write access to the data elements in a PropertyObject.
 * 
 * The TableMode template parameter should be set to true if access to the individual components
 * of a vector property array is desired or if the number of vector components of the property is unknown at compile time. 
 * If TableMode is set to false, the data elements can only be access as a whole and the number of components must
 * be a compile-time constant.
 * 
 * If the PropertyAccess object is initialized from a PropertyObject pointer, the property object's notifyTargetChanged()
 * method will be automatically called when the PropertyAccess object goes out of scope to inform the system about
 * a modification of the stored property values.
 */
template<typename T, bool TableMode = false>
class PropertyAccess : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyObject*>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyObject*>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyObject*>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyObject*>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyObject.
	PropertyAccess() = default;

	/// Constructs a read/write accessor for the data in a PropertyObject.
	PropertyAccess(const PropertyPtr& property) 
		: ParentType(property.get()) {}

	/// Constructs a read/write accessor for the data in a PropertyObject.
	PropertyAccess(PropertyObject* property) 
		: ParentType(property) {}

	/// Forbid copy construction.
	PropertyAccess(const PropertyAccess& other) = delete;

	/// Allow move construction.
	PropertyAccess(PropertyAccess&& other) = default;

	/// Forbid copy assignment.
	PropertyAccess& operator=(const PropertyAccess& other) = delete;

	/// Allow move assignment.
	PropertyAccess& operator=(PropertyAccess&& other) = default;

	/// When the PropertyAccess object goes out of scope, an automatic change message is sent by the
	/// the PropertyObject, assuming that its contents have been modified by the user of the PropertyAccess object.
	~PropertyAccess() {
		if(property())
			property()->notifyTargetChanged();
	}
};

/**
 * \brief Helper class that provides read/write access to the data elements in a PropertyObject object
 *        and which keeps a strong reference to the PropertyObject.
 */
template<typename T, bool TableMode = false>
class PropertyAccessAndRef : public std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyPtr>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyPtr>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::StdObj::detail::ReadWritePropertyAccessBaseTable<T, PropertyPtr>, Ovito::StdObj::detail::ReadWritePropertyAccessBase<T, PropertyPtr>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyStorage.
	PropertyAccessAndRef() = default;

	/// Constructs a read/write accessor for the data in a PropertyStorage.
	PropertyAccessAndRef(PropertyPtr property) 
		: ParentType(std::move(property)) {}

	/// Forbid copy construction.
	PropertyAccessAndRef(const PropertyAccessAndRef& other) = delete;

	/// Allow move construction.
	PropertyAccessAndRef(PropertyAccessAndRef&& other) = default;

	/// Forbid copy assignment.
	PropertyAccessAndRef& operator=(const PropertyAccessAndRef& other) = delete;

	/// Allow move assignment.
	PropertyAccessAndRef& operator=(PropertyAccessAndRef&& other) = default;

	/// When the PropertyAccessAndRef object goes out of scope, an automatic change message is sent by the
	/// the PropertyObject, assuming that its contents have been modified by the user of the PropertyAccessAndRef object.
	~PropertyAccessAndRef() {
		if(property())
			property()->notifyTargetChanged();
	}

	/// \brief Closes read-write access to the property and moves it out of this access object.
	PropertyPtr take() {
		if(this->_property)
			this->_property->notifyTargetChanged();
		return std::move(this->_property);
	}
};

}	// End of namespace
}	// End of namespace
