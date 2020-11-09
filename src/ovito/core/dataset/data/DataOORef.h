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
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/oo/OORef.h>

namespace Ovito {

/**
 * \brief Strong object smart-pointer to a DataObject, which ensures that the DataObject is not being modified
 *        while being referenced by multiple pointers.
 */
template<typename DataObjectClass>
class DataOORef
{
private:

    /// The internal smart-pointer to the DataObject, which keeps the object instance alive.
    OORef<DataObjectClass> _ref;

	template<class U> friend class DataOORef;

public:

    using element_type = DataObjectClass;

    /// Default constructor.
#ifndef MSVC
    DataOORef() noexcept = default;
#else
    DataOORef() noexcept {}
#endif

    /// Null pointer constructor.
    DataOORef(std::nullptr_t) noexcept {}

    /// Initialization constructor.
    DataOORef(const DataObjectClass* p) noexcept : _ref(p) {
        if(_ref) _ref->incrementDataReferenceCount();
    }

    /// Copy constructor.
    DataOORef(const DataOORef& rhs) noexcept : _ref(rhs._ref) {
        if(_ref) _ref->incrementDataReferenceCount();
    }

    /// Conversion constructor.
    template<class U>
    DataOORef(const DataOORef<U>& rhs) noexcept : _ref(rhs._ref) {
        if(_ref) _ref->incrementDataReferenceCount();
    }

    /// Move constructor.
    DataOORef(DataOORef&& rhs) noexcept : _ref(std::move(rhs._ref)) {
        OVITO_ASSERT(!rhs._ref);
    }

    /// Move and conversion constructor from a OORef.
    template<class U>
    DataOORef(OORef<U>&& rhs) noexcept : _ref(std::move(rhs)) {
        if(_ref) _ref->incrementDataReferenceCount();
    }

    /// Move and conversion constructor.
    template<class U>
    DataOORef(DataOORef<U>&& rhs) noexcept : _ref(std::move(rhs._ref)) {
        OVITO_ASSERT(!rhs._ref);
    }

    /// Destructor.
    ~DataOORef() {
        if(_ref) _ref->decrementDataReferenceCount();
    }

    /// Copy assignment operator.
    DataOORef& operator=(DataObjectClass* rhs) {
    	DataOORef(rhs).swap(*this);
    	return *this;
    }

    /// Copy assignment operator.
    DataOORef& operator=(const DataOORef& rhs) {
    	DataOORef(rhs).swap(*this);
    	return *this;
    }

    /// Copy assignment and conversion operator.
    template<class U>
    DataOORef& operator=(const DataOORef<U>& rhs) {
    	DataOORef(rhs).swap(*this);
    	return *this;
    }

    /// Move assignment operator.
    DataOORef& operator=(DataOORef&& rhs) noexcept {
    	DataOORef(std::move(rhs)).swap(*this);
    	return *this;
    }

    /// Move assignment and conversion operator.
    template<class U>
    DataOORef& operator=(DataOORef<U>&& rhs) noexcept {
    	DataOORef(std::move(rhs)).swap(*this);
    	return *this;
    }

    /// Move assignment operator with OORef.
    template<class U>
    DataOORef& operator=(OORef<U>&& rhs) noexcept {
    	DataOORef(std::move(rhs)).swap(*this);
    	return *this;
    }

    void reset() {
    	DataOORef().swap(*this);
    }

    void reset(DataObjectClass* rhs) {
    	DataOORef(rhs).swap(*this);
    }

    inline DataObjectClass* get() const noexcept {
    	return _ref.get();
    }

    inline operator DataObjectClass*() const noexcept {
    	return _ref.get();
    }

    inline DataObjectClass& operator*() const noexcept {
    	return *_ref;
    }

    inline DataObjectClass* operator->() const noexcept {
    	OVITO_ASSERT(_ref);
    	return _ref.get();
    }

    inline void swap(DataOORef& rhs) noexcept {
    	_ref.swap(rhs._ref);
    }

    /// Factory method instantiating a new data object and returning a smart-pointer to it.
    template<typename... Args>
	static DataOORef create(DataSet* dataset, Application::ExecutionContext executionContext, Args&&... args) {
		return DataOORef(OORef<DataObjectClass>::create(dataset, executionContext, std::forward<Args>(args)...));
	}

#if 0
    /// Turns a read-only reference to a data object into a read-write reference.
    /// If the data object is referenced by some other third-party, make a copy of the 
    /// object, which can be safely modified.
    DataOORef<std::remove_const_t<DataObjectClass>> makeMutable() && {
        if(!_ref || _ref->isSafeToModify())
            return std::move(_ref);
        return CloneHelper().cloneObject(_ref, false);
    }
#endif

    /// Returns a copy of the data object, which can be safely modified.
    DataOORef<std::remove_const_t<DataObjectClass>> makeCopy() const {
        return CloneHelper().cloneObject(_ref, false);
    }

    /// Makes a shallow copy of a data object.
    static DataOORef<std::remove_const_t<DataObjectClass>> makeCopy(const DataObjectClass* obj) {
        return CloneHelper().cloneObject(obj, false);
    }

    /// Makes a deep copy of a data object and its children.
    static DataOORef<std::remove_const_t<DataObjectClass>> makeDeepCopy(const DataObjectClass* obj) {
        return CloneHelper().cloneObject(obj, true);
    }

    template<class T, class U> friend DataOORef<T> static_pointer_cast(DataOORef<U>&& p) noexcept;
    template<class T, class U> friend DataOORef<T> dynamic_pointer_cast(DataOORef<U>&& p) noexcept;
};

template<class T> T* get_pointer(const DataOORef<T>& p) noexcept
{
    return p.get();
}

template<class T, class U> DataOORef<T> static_pointer_cast(const DataOORef<U>& p) noexcept
{
    return static_cast<T*>(p.get());
}

template<class T, class U> DataOORef<T> static_pointer_cast(DataOORef<U>&& p) noexcept
{
    return static_pointer_cast<T>(std::move(p._ref));
}

template<class T, class U> DataOORef<T> const_pointer_cast(const DataOORef<U>& p) noexcept
{
    return const_cast<T*>(p.get());
}

template<class T, class U> DataOORef<T> dynamic_pointer_cast(const DataOORef<U>& p) noexcept
{
    return qobject_cast<T*>(p.get());
}

template<class T, class U> DataOORef<T> dynamic_pointer_cast(DataOORef<U>&& p) noexcept
{
    return dynamic_pointer_cast<T>(std::move(p._ref));
}

template<class T> QDebug operator<<(QDebug debug, const DataOORef<T>& p)
{
	return debug << p.get();
}

}	// End of namespace
