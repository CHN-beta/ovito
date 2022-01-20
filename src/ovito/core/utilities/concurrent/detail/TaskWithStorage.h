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
#include <ovito/core/utilities/concurrent/Task.h>

namespace Ovito::detail {

/**
 * \brief Composite class template that packages a Task together with the storage for the task's results tuple.
 *
 * \tparam Tuple The std::tuple<...> type of the results storage.
 *
 * The task gets automatically configured to use the internal results storage provided by this class.
 */
template<class Tuple, class TaskBase>
class TaskWithStorage : public TaskBase, private Tuple
{
public:
    /// \brief Constructor assigning the task's results storage and forwarding any extra arguments to the task class constructor.
    /// \param initialResult The value to assign to the results storage tuple.
    template<typename InitialValue>
    explicit TaskWithStorage(Task::State initialState, InitialValue&& initialResult) : TaskBase(initialState, static_cast<Tuple*>(this)), Tuple(std::forward<InitialValue>(initialResult)) {
#ifdef OVITO_DEBUG
        // This is used in debug builds to detect programming errors and explicitly keep track of whether a result has been assigned to the task.
        this->_hasResultsStored = true;
#endif
    }

    /// \brief Constructor which leaves results storage uninitialized.
    explicit TaskWithStorage(Task::State initialState = Task::NoState) noexcept : TaskBase(initialState, std::tuple_size_v<Tuple> != 0 ? static_cast<Tuple*>(this) : nullptr) {}

protected:

    /// Provides read/write access to the internal results storage.
    Tuple& resultsTupleStorage() { 
        return static_cast<Tuple&>(*this); 
    }

    /// Provides read/write access to the internal results storage.
    decltype(auto) resultsStorage() { 
        return std::get<0>(resultsTupleStorage());
    }
};

}	// End of namespace
