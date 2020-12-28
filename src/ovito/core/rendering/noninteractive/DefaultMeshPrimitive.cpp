////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include "DefaultMeshPrimitive.h"
#include "NonInteractiveSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Returns true if the geometry buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool DefaultMeshPrimitive::isValid(SceneRenderer* renderer)
{
	// This buffer type works only in conjunction with a non-interactive renderer.
	return (qobject_cast<NonInteractiveSceneRenderer*>(renderer) != nullptr);
}

/******************************************************************************
* Activates rendering of multiple instances of the mesh.
******************************************************************************/
void DefaultMeshPrimitive::setInstancedRendering(ConstDataBufferPtr perInstanceTMs, ConstDataBufferPtr perInstanceColors)
{
	OVITO_ASSERT(perInstanceTMs);
	OVITO_ASSERT(!perInstanceColors || perInstanceTMs->size() == perInstanceColors->size());
	OVITO_ASSERT(!perInstanceColors || perInstanceColors->stride() == sizeof(ColorA));
	OVITO_ASSERT(perInstanceTMs->stride() == sizeof(AffineTransformation));

	// Store the data arrays.
	_perInstanceTMs = std::move(perInstanceTMs);
	_perInstanceColors = std::move(perInstanceColors);
	OVITO_ASSERT(useInstancedRendering());
}

/******************************************************************************
* Renders the geometry.
******************************************************************************/
void DefaultMeshPrimitive::render(SceneRenderer* renderer)
{
	NonInteractiveSceneRenderer* niRenderer = dynamic_object_cast<NonInteractiveSceneRenderer>(renderer);
	if(_mesh.faceCount() <= 0 || !niRenderer || renderer->isPicking())
		return;

	niRenderer->renderMesh(*this);
}

}	// End of namespace
