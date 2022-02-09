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

namespace Ovito {

class OVITO_CORE_EXPORT ExecutionContext
{
public:

    /// The different types of contexts in which the program's actions may be performed.
    enum Type {
        Scripting,		///< Actions are currently performed by a script.
        Interactive		///< Actions are currently performed by the user.
    };

    /// Returns the type of context the current thread performs its actions in.
    static Type current() noexcept { return _current; }

    /// Returns true if the current operation is performed by the user.
    static bool isInteractive() noexcept { return current() == Interactive; }

    /// Sets the type of context the current thread performs its actions in.
    static void setCurrent(Type type) noexcept { _current = type; }

    class OVITO_CORE_EXPORT Scope
    {
    public:
        explicit Scope(Type type) noexcept : _previous(ExecutionContext::current()) { ExecutionContext::setCurrent(type); }
        ~Scope() { ExecutionContext::setCurrent(_previous); }
    private:
        Type _previous;
    };

private:

    /// The active execution context in the current thread.
    static thread_local Type _current;
};

}	// End of namespace
