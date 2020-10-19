////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

namespace Ovito {

/**
 * \brief A weak reference (a.k.a. guarded pointer) refering to a particular revision of a DataObject.
 *
 * Data objects can be modified and typically undergo changes. To make it possible for observers to detect such changes,
 * OVITO uses the system of 'object revision numbers'.
 *
 * Each object possesses an internal revision counter that is automatically incremented every time
 * the object is being modified in some way. This allows detecting changes made to an object without
 * explicitly comparing the stored data. In particular, this approach avoids saving a copy of the old data
 * to detect any changes to the object's internal state.
 *
 * The VersionedDataObjectRef class stores an ordinary guarded pointer (QPointer) to a DataObject instance and,
 * in addition, a revision number, which refers to a particular version (or state in time) of that object.
 *
 * Two VersionedDataObjectRef instances compare equal only when both the object pointers as well as the
 * object revision numbers match exactly.
 */
class VersionedDataObjectRef
{
public:

	/// Default constructor.
	VersionedDataObjectRef() noexcept : _revision(std::numeric_limits<int>::max()) {}

	/// Initialization constructor.
	VersionedDataObjectRef(const DataObject* p) : _ref(const_cast<DataObject*>(p)), _revision(p ? p->revisionNumber() : std::numeric_limits<int>::max()) {}

	/// Initialization constructor with explicit revision number.
	VersionedDataObjectRef(const DataObject* p, int revision) : _ref(const_cast<DataObject*>(p)), _revision(revision) {}

	VersionedDataObjectRef& operator=(const DataObject* rhs) {
		_ref = const_cast<DataObject*>(rhs);
		_revision = rhs ? rhs->revisionNumber() : std::numeric_limits<int>::max();
		return *this;
	}

	void reset() noexcept {
		_ref.clear();
		_revision = std::numeric_limits<int>::max();
	}

	void reset(const DataObject* rhs) {
		_ref = const_cast<DataObject*>(rhs);
		_revision = rhs ? rhs->revisionNumber() : std::numeric_limits<int>::max();
	}

	inline const DataObject* get() const noexcept {
		return _ref.data();
	}

	inline operator const DataObject*() const noexcept {
		return _ref.data();
	}

	inline const DataObject& operator*() const {
		return *_ref;
	}

	inline const DataObject* operator->() const {
		return _ref.data();
	}

	inline void swap(VersionedDataObjectRef& rhs) noexcept {
		std::swap(_ref, rhs._ref);
		std::swap(_revision, rhs._revision);
	}

	inline int revisionNumber() const noexcept { return _revision; }

	inline void updateRevisionNumber() noexcept {
		if(_ref) _revision = _ref->revisionNumber();
	}

private:

	// The internal guarded pointer.
	QPointer<DataObject> _ref;

	// The referenced revision of the object.
	int _revision;
};

inline bool operator==(const VersionedDataObjectRef& a, const VersionedDataObjectRef& b) noexcept {
	return a.get() == b.get() && a.revisionNumber() == b.revisionNumber();
}

inline bool operator!=(const VersionedDataObjectRef& a, const VersionedDataObjectRef& b) noexcept {
	return a.get() != b.get() || a.revisionNumber() != b.revisionNumber();
}

inline bool operator==(const VersionedDataObjectRef& a, const DataObject* b) noexcept {
	return a.get() == b && (b == nullptr || a.revisionNumber() == b->revisionNumber());
}

inline bool operator!=(const VersionedDataObjectRef& a, const DataObject* b) noexcept {
	return a.get() != b || (b != nullptr && a.revisionNumber() != b->revisionNumber());
}

inline bool operator==(const DataObject* a, const VersionedDataObjectRef& b) noexcept {
	return a == b.get() && (a == nullptr || a->revisionNumber() == b.revisionNumber());
}

inline bool operator!=(const DataObject* a, const VersionedDataObjectRef& b) noexcept {
	return a != b.get() || (a != nullptr && a->revisionNumber() != b.revisionNumber());
}

inline bool operator==(const VersionedDataObjectRef& p, std::nullptr_t) noexcept {
	return p.get() == nullptr;
}

inline bool operator==(std::nullptr_t, const VersionedDataObjectRef& p) noexcept {
	return p.get() == nullptr;
}

inline bool operator!=(const VersionedDataObjectRef& p, std::nullptr_t) noexcept {
	return p.get() != nullptr;
}

inline bool operator!=(std::nullptr_t, const VersionedDataObjectRef& p) noexcept {
	return p.get() != nullptr;
}

inline void swap(VersionedDataObjectRef& lhs, VersionedDataObjectRef& rhs) noexcept {
	lhs.swap(rhs);
}

inline const DataObject* get_pointer(const VersionedDataObjectRef& p) noexcept {
	return p.get();
}

inline QDebug operator<<(QDebug debug, const VersionedDataObjectRef& p) {
	return debug << p.get() << "(rev" << p.revisionNumber() << ")";
}

}	// End of namespace


