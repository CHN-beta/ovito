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

#include <QVulkanDeviceFunctions>

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
* Returns a reference to the global Vulkan instance.
******************************************************************************/
QVulkanInstance& VulkanSceneRenderer::vkInstance()
{
	static QVulkanInstance inst;
	if(!inst.isValid()) {
#ifdef OVITO_DEBUG
		inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
#endif
		if(!inst.create())
			throw Exception(tr("Failed to create Vulkan instance: %1").arg(inst.errorCode()));
	}
	return inst;
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

/******************************************************************************
* Loads a SPIR-V shader from a file.
******************************************************************************/
VkShaderModule VulkanSceneRenderer::createShader(VkDevice device, const QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
		throw Exception(tr("File to load Vulkan shader file '%1': %2").arg(filename).arg(file.errorString()));
//        return VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
    VkShaderModule shaderModule;
    VkResult err = vkInstance().deviceFunctions(device)->vkCreateShaderModule(device, &shaderInfo, nullptr, &shaderModule);
    if(err != VK_SUCCESS) {
		throw Exception(tr("File to create Vulkan shader module '%1'. Error code: %2").arg(filename).arg(err));
//        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

}	// End of namespace
