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
#include <ovito/core/oo/PropertyField.h>
#include <ovito/core/dataset/data/DataOORef.h>

namespace Ovito {

/**
 * \brief Manages a copy-on-write reference to a DataObject class held by a RefMaker derived class.
 */
class OVITO_CORE_EXPORT SingleDataRefFieldBase : public PropertyFieldBase
{
public:

	/// The fancy pointer type.
	using pointer = DataOORef<const DataObject>;

#ifdef OVITO_DEBUG
	/// Destructor.
	~SingleDataRefFieldBase() {
		if(_pointer)
			qDebug() << "Reference field value:" << get();
		OVITO_ASSERT_MSG(!_pointer, "~ReferenceField()", "Owner object of reference field has not been deleted correctly. The reference field was not empty when the class destructor was called.");
	}
#endif

	/// Returns the DataObject pointer.
	inline std::pointer_traits<pointer>::element_type* get() const noexcept {
		return _pointer.get();
	}

protected:

	/// Replaces the current reference target with a new target. Handles undo recording.
	void setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, pointer newTarget);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, pointer& inactiveTarget);

	/// The actual pointer to the reference target.
	pointer _pointer{};

	friend class RefMaker;
	friend class RefTarget;
};

/**
 * \brief Class template specialization for DataOORef<> based object reference fields.
 */
template<typename T>
class ReferenceField<DataOORef<T>> : public SingleDataRefFieldBase
{
public:

	static_assert(std::is_const_v<T>, "A reference to a data object must always be const.");

	/// The fancy pointer type.
	using pointer = DataOORef<T>;
	/// The type of object referenced by this field.
	using target_object_type = std::remove_const_t<T>;

	/// Read access to the RefTarget derived pointer.
	operator T*() const noexcept { return get(); }

	/// Returns true if the reference is non-null.
	operator bool() const noexcept { return (bool)SingleDataRefFieldBase::_pointer; }

	/// Write access to the RefTarget pointer. Changes the value of the reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, pointer newPointer) {
		SingleDataRefFieldBase::setInternal(owner, descriptor, std::move(newPointer));
	}

	/// Returns the target currently being referenced by the reference field.
	inline T* get() const noexcept { return static_object_cast<target_object_type>(SingleDataRefFieldBase::get()); }

	/// Arrow operator.
	T* operator->() const noexcept {
		OVITO_ASSERT_MSG(SingleDataRefFieldBase::get(), "ReferenceField operator->", "Tried to dereference a null reference.");
		return get();
	}

	/// Dereference operator.
	T& operator*() const noexcept {
		OVITO_ASSERT_MSG(SingleDataRefFieldBase::get(), "ReferenceField operator*", "Tried to dereference a null reference.");
		return *get();
	}
};

}	// End of namespace
