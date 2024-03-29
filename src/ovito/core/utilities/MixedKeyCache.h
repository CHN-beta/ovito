////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
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

/**
 * \file
 * \brief Contains the definition of the Ovito::MixedKeyCache class.
 */

#pragma once


#include <ovito/core/Core.h>
#include <typeinfo>
#include "MoveOnlyAny.h"

namespace Ovito {

/**
 * \brief A cache data structure that can handle arbitrary keys and data values.
 */
class MixedKeyCache
{
public:

	/// Returns a reference to the value for the given key.
	/// Creates a new cache entry with a default-initialized value if the key doesn't exist.
	template<typename Value, typename Key>
	Value& get(Key&& key) {
		// Check if the key exists in the cache.
		for(auto& entry : _entries) {
			if(std::get<0>(entry).type() == typeid(Key) && key == any_cast<const Key&>(std::get<0>(entry))) {
				// Mark this cache entry as recently accessed.
				std::get<2>(entry) = true;
				// Read out the value of the cache entry.
				return any_cast<Value&>(std::get<1>(entry));
			}
		}
		// Create a new entry if key doesn't exist yet.
		_entries.emplace_back(std::forward<Key>(key), Value{}, true);
		return any_cast<Value&>(std::get<1>(_entries.back()));
	}

	/// This removes entries from the cache that have not been accessed since the last call to discardUnusedObjects().
	void discardUnusedObjects() {
		auto entry = _entries.begin();
		auto end_entry = _entries.end();
		while(entry != end_entry) {
			if(!std::get<2>(*entry)) {
				// Discard entry.
				--end_entry;
				*entry = std::move(*end_entry);
			}
			else {
				// Reset usage marker for the entry.
				std::get<2>(*entry) = false;
				++entry;
			}
		}
		_entries.erase(end_entry, _entries.end());
	}

private:

	/// Stores all key-value pairs of the cache. 
	/// Note we are using std::deque instead of std::vector here, because we require stability of pointers.
	/// get() returns references to elements in the cache, which must remain valid even when new objects are added
	/// to the cache.
	std::deque<std::tuple<any_moveonly, any_moveonly, bool>> _entries;
};

}	// End of namespace
