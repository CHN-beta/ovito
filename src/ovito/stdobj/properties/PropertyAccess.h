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

#pragma once


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include "PropertyObject.h"

namespace Ovito { namespace StdObj {

/**
 * \brief Helper class that provides read access to the data elements of a PropertyObject.
 * 
 * The TableMode template parameter should be set to true if access to the individual components
 * of a vector property array is desired or if the number of vector components of the property is unknown at compile time. 
 * If TableMode is set to false, the data elements can only be access as a whole and the number of components must
 * be a compile-time constant.
 */
template<typename T, bool TableMode = false>
class ConstPropertyAccess : public std::conditional_t<TableMode, Ovito::detail::ReadOnlyDataBufferAccessBaseTable<T, const PropertyObject*>, Ovito::detail::ReadOnlyDataBufferAccessBase<T, const PropertyObject*>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::detail::ReadOnlyDataBufferAccessBaseTable<T, const PropertyObject*>, Ovito::detail::ReadOnlyDataBufferAccessBase<T, const PropertyObject*>>;

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
class ConstPropertyAccessAndRef : public std::conditional_t<TableMode, Ovito::detail::ReadOnlyDataBufferAccessBaseTable<T, ConstPropertyPtr>, Ovito::detail::ReadOnlyDataBufferAccessBase<T, ConstPropertyPtr>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::detail::ReadOnlyDataBufferAccessBaseTable<T, ConstPropertyPtr>, Ovito::detail::ReadOnlyDataBufferAccessBase<T, ConstPropertyPtr>>;

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
class PropertyAccess : public std::conditional_t<TableMode, Ovito::detail::ReadWriteDataBufferAccessBaseTable<T, PropertyObject*>, Ovito::detail::ReadWriteDataBufferAccessBase<T, PropertyObject*>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::detail::ReadWriteDataBufferAccessBaseTable<T, PropertyObject*>, Ovito::detail::ReadWriteDataBufferAccessBase<T, PropertyObject*>>;

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
};

/**
 * \brief Helper class that provides read/write access to the data elements in a PropertyObject object
 *        and which keeps a strong reference to the PropertyObject.
 */
template<typename T, bool TableMode = false>
class PropertyAccessAndRef : public std::conditional_t<TableMode, Ovito::detail::ReadWriteDataBufferAccessBaseTable<T, PropertyPtr>, Ovito::detail::ReadWriteDataBufferAccessBase<T, PropertyPtr>>
{
	using ParentType = std::conditional_t<TableMode, Ovito::detail::ReadWriteDataBufferAccessBaseTable<T, PropertyPtr>, Ovito::detail::ReadWriteDataBufferAccessBase<T, PropertyPtr>>;

public:

	/// Constructs an accessor object not associated yet with any PropertyObject.
	PropertyAccessAndRef() = default;

	/// Constructs a read/write accessor for the data in a PropertyObject.
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
};

}	// End of namespace
}	// End of namespace
