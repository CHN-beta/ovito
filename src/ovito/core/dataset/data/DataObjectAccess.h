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
#include <ovito/core/oo/OORef.h>
#include <ovito/core/oo/CloneHelper.h>

namespace Ovito {

/**
 * \brief Utility class that manages read/write access to a DataObject. 
 * 
 * Initially, after the DataObject is first loaded into the accessor, it only provides read ("const") access to the object.
 * If needed, a call to makeMutable() can be used at any time to request write access to the data object.
 * The accessor class automatically takes care of cloning the original data object if necessary to make it safe to modify.
 */
template<typename DataObjectClass>
class DataObjectAccess
{
public:

    /// Constructor taking an externally owned data object.
    DataObjectAccess(const DataObjectClass* object) noexcept : 
        _constObject(object), 
        _mutableObject(object && object->isSafeToModify() ? object : nullptr) {}

	/// Releases the data object from the accessor and returns it to the caller.
	OORef<const DataObjectClass> take() noexcept {
        _mutableObject.reset();
        return std::move(_constObject);
    }

    /// Releases the current data object from this accessor and loads a new one.
    void reset(const DataObjectClass* object = nullptr) noexcept {
        _constObject = object;
        if(object && object->isSafeToModify())
            _mutableObject = object;
        else
            _mutableObject.reset();
    }

    /// Returns a mutable version of the referenced data object that is safe to modify.
    /// Makes a shallow copy of the data object if necessary.
    inline DataObjectClass* makeMutable() {
        OVITO_ASSERT(_constObject);
        if(!_mutableObject) {
            if(_constObject->isSafeToModify())
                _mutableObject = const_pointer_cast<DataObjectClass>(_constObject);
            else
                _constObject = _mutableObject = CloneHelper().cloneObject(_constObject, false);
            OVITO_ASSERT(_mutableObject->isSafeToModify());
        }
        return _mutableObject;
    }

    /// Returns a reference to the immutable data object. 
    inline const DataObjectClass& operator*() const noexcept {
        OVITO_ASSERT(_constObject);
    	return *_constObject;
    }

    /// Returns a pointer to the immutable data object.
    inline const DataObjectClass* operator->() const noexcept {
    	OVITO_ASSERT(_constObject);
    	return _constObject;
    }

    /// Returns a pointer to the immutable data object.
    inline operator const DataObjectClass*() const noexcept {
    	return _constObject;
    }

    /// Swaps to two instance of this class.
    inline void swap(DataObjectAccess& rhs) noexcept {
    	_constObject.swap(rhs._constObject);
    	_mutableObject.swap(rhs._mutableObject);
    }

private:

    /// Pointer to the read-only data object, which keeps the object alive.
    /// This pointer is always up to date.
    OORef<const DataObjectClass> _constObject;

    /// Pointer to the data object after it has been made mutable.
    /// If the data object is still read-only, because it is shared by multiple owners, then
    /// this pointer is null. Otherwise it points to the same object as the read-only pointer.
    OORef<DataObjectClass> _mutableObject;
};

}	// End of namespace
