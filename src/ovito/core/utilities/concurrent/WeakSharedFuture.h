////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
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
#include "SharedFuture.h"

namespace Ovito {

/******************************************************************************
* A weak reference to a SharedFuture
******************************************************************************/
template<typename... R>
class WeakSharedFuture : private std::weak_ptr<Task>
{
public:

#ifndef Q_CC_MSVC
	constexpr WeakSharedFuture() noexcept = default;
#else
	constexpr WeakSharedFuture() noexcept : std::weak_ptr<Task>() {}
#endif

	WeakSharedFuture(const SharedFuture<R...>& future) noexcept : std::weak_ptr<Task>(future.task()) {}

	WeakSharedFuture& operator=(const Future<R...>& f) noexcept {
		std::weak_ptr<Task>::operator=(f.task());
		return *this;
	}

	WeakSharedFuture& operator=(const SharedFuture<R...>& f) noexcept {
		std::weak_ptr<Task>::operator=(f.task());
		return *this;
	}

	void reset() noexcept {
		std::weak_ptr<Task>::reset();
	}

	SharedFuture<R...> lock() const noexcept {
		return SharedFuture<R...>(std::weak_ptr<Task>::lock());
	}

	bool expired() const noexcept {
		return std::weak_ptr<Task>::expired();
	}
};

}	// End of namespace