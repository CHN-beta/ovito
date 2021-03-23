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
#include "VulkanParticlePrimitive.h"
#include "VulkanSceneRenderer.h"

namespace Ovito {

/******************************************************************************
* Creates the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanParticlePrimitive::Pipelines::init(VulkanSceneRenderer* renderer)
{
}

/******************************************************************************
* Destroys the Vulkan pipelines for this rendering primitive.
******************************************************************************/
void VulkanParticlePrimitive::Pipelines::release(VulkanSceneRenderer* renderer)
{
}

/******************************************************************************
* Renders the particles.
******************************************************************************/
void VulkanParticlePrimitive::render(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
    // Make sure there is something to be rendered. Otherwise, step out early.
	if(!positions() || positions()->size() == 0)
		return;
}

/******************************************************************************
* Renders the particles using box-shaped geometry.
******************************************************************************/
void VulkanParticlePrimitive::renderBoxGeometries(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
}

/******************************************************************************
* Renders the particles using imposter quads.
******************************************************************************/
void VulkanParticlePrimitive::renderImposterGeometries(VulkanSceneRenderer* renderer, const Pipelines& pipelines)
{
}

}	// End of namespace
