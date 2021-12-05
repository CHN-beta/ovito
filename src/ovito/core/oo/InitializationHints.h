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

#pragma once


#include <ovito/core/Core.h>

namespace Ovito {

/// Bit flags that can be passed to the OORef<>::create() factory method when instantiating an object class.
enum ObjectInitializationHint {
    LoadFactoryDefaults = 0,		//< Perform standard object initialization using hard-coded defaults.
    LoadUserDefaults    = (1<<0),	//< Load user-defined standard values from the application settings store.
    WithoutVisElement   = (1<<1),	//< Do not attach a standard visual element when creating a new data object.
};
Q_DECLARE_FLAGS(ObjectInitializationHints, ObjectInitializationHint);
Q_DECLARE_OPERATORS_FOR_FLAGS(ObjectInitializationHints);

}	// End of namespace
