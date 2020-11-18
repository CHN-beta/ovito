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

#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/DataSet.h>
#include "PropertyObject.h"
#include "PropertyAccess.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyObject);
DEFINE_VECTOR_REFERENCE_FIELD(PropertyObject, elementTypes);
DEFINE_PROPERTY_FIELD(PropertyObject, title);
SET_PROPERTY_FIELD_LABEL(PropertyObject, elementTypes, "Element types");
SET_PROPERTY_FIELD_LABEL(PropertyObject, title, "Title");
SET_PROPERTY_FIELD_CHANGE_EVENT(PropertyObject, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor creating an empty property array.
******************************************************************************/
PropertyObject::PropertyObject(DataSet* dataset) : DataObject(dataset)
{
}

/******************************************************************************
* Constructor allocating a property array with given size and data layout.
******************************************************************************/
PropertyObject::PropertyObject(DataSet* dataset, size_t elementCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory, int type, QStringList componentNames) :
	DataObject(dataset),
	_dataType(dataType),
	_dataTypeSize(QMetaType::sizeOf(dataType)),
	_stride(stride),
	_componentCount(componentCount),
	_componentNames(std::move(componentNames)),
	_name(name),
	_type(type)
{
	setIdentifier(name);
	OVITO_ASSERT(dataType == Int || dataType == Int64 || dataType == Float);
	OVITO_ASSERT(_dataTypeSize > 0);
	OVITO_ASSERT(_componentCount > 0);
	if(_stride == 0) 
		_stride = _dataTypeSize * _componentCount;
	OVITO_ASSERT(_stride >= _dataTypeSize * _componentCount);
	OVITO_ASSERT((_stride % _dataTypeSize) == 0);
	if(componentCount > 1) {
		for(size_t i = _componentNames.size(); i < componentCount; i++)
			_componentNames << QString::number(i + 1);
	}
	resize(elementCount, initializeMemory);
}

/******************************************************************************
* Creates a copy of a property object.
******************************************************************************/
OORef<RefTarget> PropertyObject::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<PropertyObject> clone = static_object_cast<PropertyObject>(DataObject::clone(deepCopy, cloneHelper));

	// Copy internal data.
	prepareReadAccess();
	clone->_type = _type;
	clone->_name = _name;
	clone->_dataType = _dataType;
	clone->_dataTypeSize = _dataTypeSize;
	clone->_numElements = _numElements;
	clone->_capacity = _numElements;
	clone->_stride = _stride;
	clone->_componentCount = _componentCount;
	clone->_componentNames = _componentNames;
	clone->_data.reset(new uint8_t[_numElements * _stride]);
	std::memcpy(clone->_data.get(), _data.get(), _numElements * _stride);
	OVITO_ASSERT(clone->identifier() == clone->name());
	finishReadAccess();

	return clone;
}

/******************************************************************************
* Resizes the property storage.
******************************************************************************/
void PropertyObject::resize(size_t newSize, bool preserveData)
{
	prepareWriteAccess();
	if(newSize > _capacity || newSize < _capacity * 3 / 4 || !_data) {
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[newSize * _stride]);
		if(preserveData)
			std::memcpy(newBuffer.get(), _data.get(), _stride * std::min(_numElements, newSize));
		_data.swap(newBuffer);
		_capacity = newSize;
	}
	// Initialize new elements to zero.
	if(newSize > _numElements && preserveData) {
		std::memset(_data.get() + _numElements * _stride, 0, (newSize - _numElements) * _stride);
	}
	_numElements = newSize;
	finishWriteAccess();
}

/******************************************************************************
* Grows the storage buffer to accomodate at least the given number of data elements
******************************************************************************/
void PropertyObject::growCapacity(size_t newSize)
{
	prepareWriteAccess();
	OVITO_ASSERT(newSize >= _numElements);
	OVITO_ASSERT(newSize > _capacity);
	size_t newCapacity = (newSize < 1024)
		? std::max(newSize * 2, (size_t)256)
		: (newSize * 3 / 2);
	std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[newCapacity * _stride]);
	std::memcpy(newBuffer.get(), _data.get(), _stride * _numElements);
	_data.swap(newBuffer);
	_capacity = newCapacity;
	finishWriteAccess();
}

/******************************************************************************
* Sets the property's name.
******************************************************************************/
void PropertyObject::setName(const QString& newName)
{
	if(newName == name())
		return;

	_name = newName;
	setIdentifier(newName);
	notifyTargetChanged(&PROPERTY_FIELD(title));
}

/******************************************************************************
* Returns the display title of this property object in the user interface.
******************************************************************************/
QString PropertyObject::objectTitle() const
{
	return title().isEmpty() ? name() : title();
}

/******************************************************************************
* Generates a human-readable string representation of the data object reference.
******************************************************************************/
QString PropertyObject::OOMetaClass::formatDataObjectPath(const ConstDataObjectPath& path) const
{
	QString str;
	for(auto obj = path.begin(); obj != path.end(); ++obj) {
		if(obj != path.begin())
			str += QStringLiteral(u" \u2192 ");  // Unicode arrow
		if(obj != path.end() - 1)
			str += (*obj)->objectTitle();
		else
			str += static_object_cast<PropertyObject>(*obj)->name();
	}
	return str;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void PropertyObject::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData)
{
	DataObject::saveToStream(stream, excludeRecomputableData);

	prepareReadAccess();
	try {
		stream.beginChunk(0x01);
		stream.beginChunk(0x02);
		stream << _name;
		stream << _type;
		stream << QByteArray(QMetaType::typeName(_dataType));
		stream.writeSizeT(_dataTypeSize);
		stream.writeSizeT(_stride);
		stream.writeSizeT(_componentCount);
		stream << _componentNames;
		if(excludeRecomputableData) {
			stream.writeSizeT(0);
		}
		else {
			stream.writeSizeT(_numElements);
			stream.write(_data.get(), _stride * _numElements);
		}
		stream.endChunk();
		stream.endChunk();
		finishReadAccess();
	}
	catch(...) {
		finishReadAccess();
		throw;
	}
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void PropertyObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	stream.expectChunk(0x02);
	stream >> _name;
	stream >> _type;
	QByteArray dataTypeName;
	stream >> dataTypeName;
	_dataType = QMetaType::type(dataTypeName.constData());
	OVITO_ASSERT_MSG(_dataType != 0, "PropertyObject::loadFromStream()", QString("The metadata type '%1' seems to be no longer defined.").arg(QString(dataTypeName)).toLocal8Bit().constData());
	OVITO_ASSERT(dataTypeName == QMetaType::typeName(_dataType));
	stream.readSizeT(_dataTypeSize);
	stream.readSizeT(_stride);
	stream.readSizeT(_componentCount);
	stream >> _componentNames;
	stream.readSizeT(_numElements);
	_capacity = _numElements;
	_data.reset(new uint8_t[_numElements * _stride]);
	stream.read(_data.get(), _stride * _numElements);
	stream.closeChunk();

	// Do floating-point precision conversion from single to double precision.
	if(_dataType == qMetaTypeId<float>() && PropertyObject::Float == qMetaTypeId<double>()) {
		OVITO_ASSERT(sizeof(FloatType) == sizeof(double));
		OVITO_ASSERT(_dataTypeSize == sizeof(float));
		_stride *= sizeof(double) / sizeof(float);
		_dataTypeSize = sizeof(double);
		_dataType = PropertyObject::Float;
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[_stride * _numElements]);
		double* dst = reinterpret_cast<double*>(newBuffer.get());
		const float* src = reinterpret_cast<const float*>(_data.get());
		for(size_t c = _numElements * _componentCount; c--; )
			*dst++ = (double)*src++;
		_data.swap(newBuffer);
	}

	// Do floating-point precision conversion from double to single precision.
	if(_dataType == qMetaTypeId<double>() && PropertyObject::Float == qMetaTypeId<float>()) {
		OVITO_ASSERT(sizeof(FloatType) == sizeof(float));
		OVITO_ASSERT(_dataTypeSize == sizeof(double));
		_stride /= sizeof(double) / sizeof(float);
		_dataTypeSize = sizeof(float);
		_dataType = PropertyObject::Float;
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[_stride * _numElements]);
		float* dst = reinterpret_cast<float*>(newBuffer.get());
		const double* src = reinterpret_cast<const double*>(_data.get());
		for(size_t c = _numElements * _componentCount; c--; )
			*dst++ = (float)*src++;
		_data.swap(newBuffer);
	}

	stream.closeChunk();
	setIdentifier(name());
}

/******************************************************************************
* Extends the data array and replicates the old data N times.
******************************************************************************/
void PropertyObject::replicate(size_t n, bool replicateValues)
{
	OVITO_ASSERT(n >= 1);
	if(n <= 1) return;

	prepareWriteAccess();
	size_t oldSize = _numElements;
	std::unique_ptr<uint8_t[]> oldData = std::move(_data);

	_numElements *= n;
	_capacity = _numElements;
	_data = std::make_unique<uint8_t[]>(_capacity * _stride);
	if(replicateValues) {
		// Replicate data values N times.
		uint8_t* dest = _data.get();
		for(size_t i = 0; i < n; i++, dest += oldSize * stride()) {
			std::memcpy(dest, oldData.get(), oldSize * stride());
		}
	}
	else {
		// Copy just one replica of the data from the old memory buffer to the new one.
		std::memcpy(_data.get(), oldData.get(), oldSize * stride());
	}
	finishWriteAccess();
}

/******************************************************************************
* Reduces the size of the storage array, removing elements for which
* the corresponding bits in the bit array are set.
******************************************************************************/
void PropertyObject::filterResize(const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(size() == mask.size());
	size_t s = size();

	// Optimize filter operation for the most common property types.
	if(dataType() == PropertyObject::Float && stride() == sizeof(FloatType)) {
		prepareWriteAccess();
		// Single float
		auto src = reinterpret_cast<const FloatType*>(cbuffer());
		auto dst = reinterpret_cast<FloatType*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		finishWriteAccess();
		resize(dst - reinterpret_cast<FloatType*>(buffer()), true);
	}
	else if(dataType() == PropertyObject::Int && stride() == sizeof(int)) {
		// Single integer
		prepareWriteAccess();
		auto src = reinterpret_cast<const int*>(cbuffer());
		auto dst = reinterpret_cast<int*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		finishWriteAccess();
		resize(dst - reinterpret_cast<int*>(buffer()), true);
	}
	else if(dataType() == PropertyObject::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		prepareWriteAccess();
		auto src = reinterpret_cast<const qlonglong*>(cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		finishWriteAccess();
		resize(dst - reinterpret_cast<qlonglong*>(buffer()), true);
	}
	else if(dataType() == PropertyObject::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		prepareWriteAccess();
		auto src = reinterpret_cast<const Point3*>(cbuffer());
		auto dst = reinterpret_cast<Point3*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		finishWriteAccess();
		resize(dst - reinterpret_cast<Point3*>(buffer()), true);
	}
	else if(dataType() == PropertyObject::Float && stride() == sizeof(Color)) {
		// Triple float
		prepareWriteAccess();
		auto src = reinterpret_cast<const Color*>(cbuffer());
		auto dst = reinterpret_cast<Color*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		finishWriteAccess();
		resize(dst - reinterpret_cast<Color*>(buffer()), true);
	}
	else if(dataType() == PropertyObject::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		prepareWriteAccess();
		auto src = reinterpret_cast<const Point3I*>(cbuffer());
		auto dst = reinterpret_cast<Point3I*>(buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		finishWriteAccess();
		resize(dst - reinterpret_cast<Point3I*>(buffer()), true);
	}
	else {
		// Generic case:
		prepareWriteAccess();
		const uint8_t* src = _data.get();
		uint8_t* dst = _data.get();
		size_t stride = this->stride();
		for(size_t i = 0; i < s; i++, src += stride) {
			if(!mask.test(i)) {
				std::memcpy(dst, src, stride);
				dst += stride;
			}
		}
		finishWriteAccess();
		resize((dst - _data.get()) / stride, true);
	}
}

/******************************************************************************
* Creates a copy of the array, not containing those elements for which
* the corresponding bits in the given bit array were set.
******************************************************************************/
OORef<PropertyObject> PropertyObject::filterCopy(const boost::dynamic_bitset<>& mask) const
{
	prepareReadAccess();
	OVITO_ASSERT(size() == mask.size());

	size_t s = size();
	size_t newSize = size() - mask.count();
	OORef<PropertyObject> copy = OORef<PropertyObject>::create(dataset(), Application::ExecutionContext::Scripting, newSize, dataType(), componentCount(), stride(), name(), false, type(), componentNames());

	// Optimize filter operation for the most common property types.
	if(dataType() == PropertyObject::Float && stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(cbuffer());
		auto dst = reinterpret_cast<FloatType*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<FloatType*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyObject::Int && stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(cbuffer());
		auto dst = reinterpret_cast<int*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<int*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyObject::Int64 && stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<qlonglong*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyObject::Float && stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(cbuffer());
		auto dst = reinterpret_cast<Point3*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<Point3*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyObject::Float && stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(cbuffer());
		auto dst = reinterpret_cast<Color*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<Color*>(copy->buffer()) + newSize);
	}
	else if(dataType() == PropertyObject::Int && stride() == sizeof(Point3I)) {
		// Triple int.
		auto src = reinterpret_cast<const Point3I*>(cbuffer());
		auto dst = reinterpret_cast<Point3I*>(copy->buffer());
		for(size_t i = 0; i < s; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
		OVITO_ASSERT(dst == reinterpret_cast<Point3I*>(copy->buffer()) + newSize);
	}
	else {
		// Generic case:
		const uint8_t* src = _data.get();
		uint8_t* dst = copy->_data.get();
		size_t stride = this->stride();
		for(size_t i = 0; i < s; i++, src += stride) {
			if(!mask.test(i)) {
				std::memcpy(dst, src, stride);
				dst += stride;
			}
		}
		OVITO_ASSERT(dst == copy->_data.get() + newSize * stride);
	}
	finishReadAccess();
	return copy;
}

/******************************************************************************
* Copies the contents from the given source into this property storage using
* a mapping of indices.
******************************************************************************/
void PropertyObject::mappedCopyFrom(const PropertyObject& source, const std::vector<size_t>& mapping)
{
	OVITO_ASSERT(source.size() == mapping.size());
	OVITO_ASSERT(stride() == source.stride());
	OVITO_ASSERT(&source != this);
	prepareWriteAccess();
	source.prepareReadAccess();

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(source.cbuffer());
		auto dst = reinterpret_cast<FloatType*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(source.cbuffer());
		auto dst = reinterpret_cast<int*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(source.cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(source.cbuffer());
		auto dst = reinterpret_cast<Point3*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(source.cbuffer());
		auto dst = reinterpret_cast<Color*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(source.cbuffer());
		auto dst = reinterpret_cast<Point3I*>(buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < this->size());
			dst[idx] = *src++;
		}
	}
	else {
		// General case:
		const uint8_t* src = source.cbuffer();
		uint8_t* dst = buffer();
		size_t stride = this->stride();
		for(size_t i = 0; i < source.size(); i++, src += stride) {
			OVITO_ASSERT(mapping[i] < this->size());
			std::memcpy(dst + stride * mapping[i], src, stride);
		}
	}

	source.finishReadAccess();
	finishWriteAccess();
}

/******************************************************************************
* Copies the elements from this storage array into the given destination array 
* using an index mapping.
******************************************************************************/
void PropertyObject::mappedCopyTo(PropertyObject& destination, const std::vector<size_t>& mapping) const
{
	OVITO_ASSERT(destination.size() == mapping.size());
	OVITO_ASSERT(this->stride() == destination.stride());
	OVITO_ASSERT(&destination != this);
	prepareReadAccess();
	destination.prepareWriteAccess();

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		auto src = reinterpret_cast<const FloatType*>(cbuffer());
		auto dst = reinterpret_cast<FloatType*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		auto src = reinterpret_cast<const int*>(cbuffer());
		auto dst = reinterpret_cast<int*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(qlonglong)) {
		// Single 64-bit integer
		auto src = reinterpret_cast<const qlonglong*>(cbuffer());
		auto dst = reinterpret_cast<qlonglong*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		auto src = reinterpret_cast<const Point3*>(cbuffer());
		auto dst = reinterpret_cast<Point3*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		auto src = reinterpret_cast<const Color*>(cbuffer());
		auto dst = reinterpret_cast<Color*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else if(stride() == sizeof(Point3I)) {
		// Triple int
		auto src = reinterpret_cast<const Point3I*>(cbuffer());
		auto dst = reinterpret_cast<Point3I*>(destination.buffer());
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			*dst++ = src[idx];
		}
	}
	else {
		// General case:
		const uint8_t* src = cbuffer();
		uint8_t* dst = destination.buffer();
		size_t stride = this->stride();
		for(size_t idx : mapping) {
			OVITO_ASSERT(idx < size());
			std::memcpy(dst, src + stride * idx, stride);
			dst += stride;
		}
	}
	destination.finishWriteAccess();
	finishReadAccess();
}

/******************************************************************************
* Reorders the existing elements in this storage array using an index map.
******************************************************************************/
void PropertyObject::reorderElements(const std::vector<size_t>& mapping)
{
	PropertyPtr copy = CloneHelper().cloneObject(this, false);
	copy->mappedCopyTo(*this, mapping);
}

/******************************************************************************
* Copies the data elements from the given source array into this array. 
* Array size, component count and data type of source and destination must match exactly.
******************************************************************************/
void PropertyObject::copyFrom(const PropertyObject& source)
{
	OVITO_ASSERT(this->dataType() == source.dataType());
	OVITO_ASSERT(this->stride() == source.stride());
	OVITO_ASSERT(this->size() == source.size());
	if(&source != this) {
		prepareWriteAccess();
		source.prepareReadAccess();
		std::memcpy(buffer(), source.cbuffer(), this->stride() * this->size());
		source.finishReadAccess();
		finishWriteAccess();
	}
}

/******************************************************************************
* Copies a range of data elements from the given source array into this array. 
* Component count and data type of source and destination must be compatible.
******************************************************************************/
void PropertyObject::copyRangeFrom(const PropertyObject& source, size_t sourceIndex, size_t destIndex, size_t count)
{
	OVITO_ASSERT(this->dataType() == source.dataType());
	OVITO_ASSERT(this->stride() == source.stride());
	OVITO_ASSERT(sourceIndex + count <= source.size());
	OVITO_ASSERT(destIndex + count <= this->size());
	prepareWriteAccess();
	source.prepareReadAccess();
	std::memcpy(buffer() + destIndex * this->stride(), source.cbuffer() + sourceIndex * source.stride(), this->stride() * count);
	source.finishReadAccess();
	finishWriteAccess();
}

/******************************************************************************
* Checks if this property storage and its contents exactly match those of 
* another property storage.
******************************************************************************/
bool PropertyObject::equals(const PropertyObject& other) const
{
	prepareReadAccess();
	other.prepareReadAccess();

	bool result = [&]() {
		if(this->type() != other.type()) return false;
		if(this->type() == GenericUserProperty && this->name() != other.name()) return false;
		if(this->dataType() != other.dataType()) return false;
		if(this->size() != other.size()) return false;
		if(this->componentCount() != other.componentCount()) return false;
		OVITO_ASSERT(this->stride() == other.stride());
		return std::equal(this->cbuffer(), this->cbuffer() + this->size() * this->stride(), other.cbuffer());
	}();

	other.finishReadAccess();
	finishReadAccess();

	return result;
}

/******************************************************************************
* Puts the property array into a writable state.
* In the writable state, the Python binding layer will allow write access
* to the property's internal data.
******************************************************************************/
void PropertyObject::makeWritableFromPython()
{
	OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

	if(!isSafeToModify())
		throwException(tr("Modifying the data values stored in this property is not allowed, because the Property object currently is shared by more than one PropertyContainer or DataCollection. "
						"Please explicitly request a mutable version of the property using the '_' notation or by calling the DataObject.make_mutable() method on its parent container. "
						"See the documentation of this method for further information on OVITO's data model and the shared-ownership system."));
	_isWritableFromPython++;
}

/******************************************************************************
* Puts the property array back into the default read-only state.
* In the read-only state, the Python binding layer will not permit write 
* access to the property's internal data.
******************************************************************************/
void PropertyObject::makeReadOnlyFromPython()
{
	OVITO_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

	OVITO_ASSERT(_isWritableFromPython > 0);
	_isWritableFromPython--;
}

/******************************************************************************
* Helper method that remaps the existing type IDs to a contiguous range starting at the given
* base ID. This method is mainly used for file output, because some file formats
* work with numeric particle types only, which must form a contiguous range.
* The method returns the mapping of output type IDs to original type IDs
* and a copy of the property array in which the original type ID values have
* been remapped to the output IDs.
******************************************************************************/
std::tuple<std::map<int,int>, ConstPropertyPtr> PropertyObject::generateContiguousTypeIdMapping(int baseId) const
{
	OVITO_ASSERT(dataType() == PropertyObject::Int && componentCount() == 1);

	// Generate sorted list of existing type IDs.
	std::set<int> typeIds;
	for(const ElementType* t : elementTypes())
		typeIds.insert(t->numericId());

	// Add ID values that occur in the property array but which have not been defined as a type.
	for(int t : ConstPropertyAccess<int>(this))
		typeIds.insert(t);

	// Build the mappings between old and new IDs.
	std::map<int,int> oldToNewMap;
	std::map<int,int> newToOldMap;
	bool remappingRequired = false;
	for(int id : typeIds) {
		if(id != baseId) remappingRequired = true;
		oldToNewMap.emplace(id, baseId);
		newToOldMap.emplace(baseId++, id);
	}

	// Create a copy of the per-element type array in which old IDs have been replaced with new ones.
	ConstPropertyPtr remappedArray;
	if(remappingRequired) {
		// Make a copy of this property, which can be modified.
		PropertyAccessAndRef<int> array(CloneHelper().cloneObject(this, false));
		for(int& id : array)
			id = oldToNewMap[id];
		remappedArray = array.take();
	}
	else {
		// No data copied needed if ordering hasn't changed.
		remappedArray = this;
	}

	return std::make_tuple(std::move(newToOldMap), std::move(remappedArray));
}

/******************************************************************************
* Changes the data type of the property in place and convert the values stored 
* in the property.
******************************************************************************/
void PropertyObject::convertDataType(int newDataType)
{
	OVITO_ASSERT(newDataType == Int || newDataType == Int64 || newDataType == Float);

	if(dataType() == newDataType)
		return;

	size_t newDataTypeSize = QMetaType::sizeOf(newDataType);
	size_t newStride = _componentCount * newDataTypeSize;
	std::unique_ptr<uint8_t[]> newData = std::make_unique<uint8_t[]>(_numElements * newStride);

	// Copy values from old buffer to new buffer and perform data type convertion.
	ConstPropertyAccess<void, true> oldData(this);
	switch(newDataType) {
	case Int:
		{
			int* dest = reinterpret_cast<int*>(newData.get());
			for(size_t i = 0; i < _numElements; i++)
				for(size_t j = 0; j < _componentCount; j++)
					*dest++ = oldData.get<int>(i, j);
		}
		break;
	case Int64:
		{
			qlonglong* dest = reinterpret_cast<qlonglong*>(newData.get());
			for(size_t i = 0; i < _numElements; i++)
				for(size_t j = 0; j < _componentCount; j++)
					*dest++ = oldData.get<qlonglong>(i, j);
		}
		break;
	case Float:
		{
			FloatType* dest = reinterpret_cast<FloatType*>(newData.get());
			for(size_t i = 0; i < _numElements; i++)
				for(size_t j = 0; j < _componentCount; j++)
					*dest++ = oldData.get<FloatType>(i, j);
		}
		break;
	default:
		OVITO_ASSERT(false); // Unsupported data type
	}
	oldData.reset();

	prepareWriteAccess();
	_dataType = newDataType;
	_dataTypeSize = newDataTypeSize;
	_stride = newStride;
	_capacity = _numElements;
	_data = std::move(newData);
	finishWriteAccess();
}

/******************************************************************************
* Sorts the types w.r.t. their name.
* This method is used by file parsers that create element types on the
* go while the read the data. In such a case, the ordering of types
* depends on the storage order of data elements in the file, which is not desirable.
******************************************************************************/
void PropertyObject::sortElementTypesByName()
{
	OVITO_ASSERT(dataType() == StandardDataType::Int);

	// Check if type IDs form a consecutive sequence starting at 1.
	// If not, we leave the type order as it is.
	int id = 1;
	for(const ElementType* type : elementTypes()) {
		if(type->numericId() != id++)
			return;
	}

	// Check if types are already in the correct order.
	if(std::is_sorted(elementTypes().begin(), elementTypes().end(), 
			[](const ElementType* a, const ElementType* b) { return a->name().compare(b->name(), Qt::CaseInsensitive) < 0; }))
		return;

	// Reorder types by name.
	DataRefVector<ElementType> types = elementTypes();
	std::sort(types.begin(), types.end(), 
		[](const ElementType* a, const ElementType* b) { return a->name().compare(b->name(), Qt::CaseInsensitive) < 0; });
	setElementTypes(std::move(types));

#if 0
	// NOTE: No longer reassigning numeric IDs to the types here, because the new requirement is
	// that the numeric ID of an existing ElementType never changes once the type has been created.
	// Otherwise, the editable proxy objects would become out of sync.

	// Build map of IDs.
	std::vector<int> mapping(elementTypes().size() + 1);
	for(int index = 0; index < elementTypes().size(); index++) {
		int id = elementTypes()[index]->numericId();
		mapping[id] = index + 1;
		if(id != index + 1)
			makeMutable(elementTypes()[index])->setNumericId(index + 1);
	}

	// Remap type IDs.
	for(int& t : PropertyAccess<int>(this)) {
		OVITO_ASSERT(t >= 1 && t < mapping.size());
		t = mapping[t];
	}
#endif
}

/******************************************************************************
* Sorts the element types with respect to the numeric identifier.
******************************************************************************/
void PropertyObject::sortElementTypesById()
{
	DataRefVector<ElementType> types = elementTypes();
	std::sort(types.begin(), types.end(), 
		[](const auto& a, const auto& b) { return a->numericId() < b->numericId(); });
	setElementTypes(std::move(types));
}

/******************************************************************************
* Creates an editable proxy object for this DataObject and synchronizes its parameters.
******************************************************************************/
void PropertyObject::updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const
{
	DataObject::updateEditableProxies(state, dataPath);

	// Note: 'this' may no longer exist at this point, because the base method implementation may
	// have already replaced it with a mutable copy.
	const PropertyObject* self = static_object_cast<PropertyObject>(dataPath.back());

	if(PropertyObject* proxy = static_object_cast<PropertyObject>(self->editableProxy())) {
		// Synchronize the actual data object with the editable proxy object.
		OVITO_ASSERT(proxy->type() == self->type());
		OVITO_ASSERT(proxy->dataType() == self->dataType());
		OVITO_ASSERT(proxy->title() == self->title());

		// Add the proxies of newly created element types to the proxy property object.
		for(const ElementType* type : self->elementTypes()) {
			ElementType* proxyType = static_object_cast<ElementType>(type->editableProxy());
			OVITO_ASSERT(proxyType != nullptr);
			if(!proxy->elementTypes().contains(proxyType))
				proxy->addElementType(proxyType);
		}

		// Add element types that are non-existing in the actual property object.
		// Note: Currently this should never happen, because file parser never
		// remove element types.
		for(const ElementType* proxyType : proxy->elementTypes()) {
			OVITO_ASSERT(std::any_of(self->elementTypes().begin(), self->elementTypes().end(), [proxyType](const ElementType* type) { return type->editableProxy() == proxyType; }));
		}
	}
	else if(!self->elementTypes().empty()) {
		// Create and initialize a new proxy property object. 
		// Note: We avoid copying the property data here by constructing the proxy PropertyObject from scratch instead of cloning the original data object.
		OORef<PropertyObject> newProxy = OORef<PropertyObject>::create(self->dataset(), Application::ExecutionContext::Scripting, 0, self->dataType(), self->componentCount(), self->stride(), self->name(), false, self->type(), self->componentNames());
		newProxy->setTitle(self->title());

		// Adopt the proxy object for the element types, which have already been created by
		// the recursive method.
		for(const ElementType* type : self->elementTypes()) {
			OVITO_ASSERT(type->editableProxy() != nullptr);
			newProxy->addElementType(static_object_cast<ElementType>(type->editableProxy()));
		}

		// Make this data object mutable and attach the proxy object to it.
		state.makeMutableInplace(dataPath)->setEditableProxy(std::move(newProxy));
	}
}

}	// End of namespace
}	// End of namespace
