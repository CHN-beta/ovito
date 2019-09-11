///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <ovito/core/Core.h>
#include "DefaultMarkerPrimitive.h"
#include "NonInteractiveSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool DefaultMarkerPrimitive::isValid(SceneRenderer* renderer)
{
	// This buffer type works only in conjunction with a non-interactive renderer.
	return (qobject_cast<NonInteractiveSceneRenderer*>(renderer) != nullptr);
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void DefaultMarkerPrimitive::render(SceneRenderer* renderer)
{
	// Markers are only rendered by the interactive viewport renderer.
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
