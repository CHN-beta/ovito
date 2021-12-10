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

#include <ovito/core/Core.h>
#include "SplineInterpolationControllers.h"

namespace Ovito {

// This had to be removed for now, because it leads to a segmentation fault on Linux/Unix platforms.
// Apparently, global static initializers are run before C++17 inline static initializers of class templates.
// We leave these parameter fields without labels for now (they are not used in the GUI anyway) until
// a better solution for static initialization order problem is found.
#if 0 
SET_PROPERTY_FIELD_LABEL(FloatSplineAnimationKey, inTangent, "In Tangent");
SET_PROPERTY_FIELD_LABEL(FloatSplineAnimationKey, outTangent, "Out Tangent");

SET_PROPERTY_FIELD_LABEL(PositionSplineAnimationKey, inTangent, "In Tangent");
SET_PROPERTY_FIELD_LABEL(PositionSplineAnimationKey, outTangent, "Out Tangent");
#endif

}	// End of namespace
