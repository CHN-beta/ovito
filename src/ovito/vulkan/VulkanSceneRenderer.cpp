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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "VulkanSceneRenderer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(VulkanSceneRenderer);

/******************************************************************************
* Is called by OVITO to query the class for any information that should be 
* included in the application's system report.
******************************************************************************/
void VulkanSceneRenderer::OOMetaClass::querySystemInformation(QTextStream& stream) const
{
	if(this == &VulkanSceneRenderer::OOClass()) {
		stream << "======== Vulkan info =======" << "\n";
	}
}

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with
* the given other renderer.
******************************************************************************/
bool VulkanSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	// Two Vulkan renderers are always compatible.
	if(VulkanSceneRenderer* otherGLRenderer = dynamic_object_cast<VulkanSceneRenderer>(otherRenderer)) {
		return true;
	}
	return false;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void VulkanSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void VulkanSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
	SceneRenderer::endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool VulkanSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	return !operation.isCanceled();
}

/******************************************************************************
* Renders a 2d polyline in the viewport.
******************************************************************************/
void VulkanSceneRenderer::render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed)
{
	if(isBoundingBoxPass())
		return;
}

/******************************************************************************
* Sets the frame buffer background color.
******************************************************************************/
void VulkanSceneRenderer::setClearColor(const ColorA& color)
{
}

/******************************************************************************
* Sets the rendering region in the frame buffer.
******************************************************************************/
void VulkanSceneRenderer::setRenderingViewport(int x, int y, int width, int height)
{
}

/******************************************************************************
* Clears the frame buffer contents.
******************************************************************************/
void VulkanSceneRenderer::clearFrameBuffer(bool clearDepthBuffer, bool clearStencilBuffer)
{
}

/******************************************************************************
* Temporarily enables/disables the depth test while rendering.
******************************************************************************/
void VulkanSceneRenderer::setDepthTestEnabled(bool enabled)
{
}

/******************************************************************************
* Activates the special highlight rendering mode.
******************************************************************************/
void VulkanSceneRenderer::setHighlightMode(int pass)
{
}

}	// End of namespace
