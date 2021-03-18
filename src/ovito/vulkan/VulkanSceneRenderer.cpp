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
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "VulkanSceneRenderer.h"
#include "VulkanLinePrimitive.h"

#include <QVulkanDeviceFunctions>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(VulkanSceneRenderer);

/******************************************************************************
* Is called by OVITO to query the class for any information that should be 
* included in the application's system report.
******************************************************************************/
void VulkanSceneRenderer::OOMetaClass::querySystemInformation(QTextStream& stream, DataSetContainer& container) const
{
	if(this == &VulkanSceneRenderer::OOClass()) {
		stream << "======== Vulkan info =======" << "\n";

        // Look up an existing Vulkan device from one of the interactive viewport windows.
        std::shared_ptr<VulkanDevice> device;
        for(Viewport* vp : container.currentSet()->viewportConfig()->viewports()) {
            if(ViewportWindowInterface* window = vp->window()) {
                if(VulkanSceneRenderer* renderer = dynamic_object_cast<VulkanSceneRenderer>(window->sceneRenderer())) {
                    device = renderer->device();
                    break;
                }
            }
        }

        // Create an adhoc instance of VulkanDevice class if needed.
        if(!device)
            device = std::make_shared<VulkanDevice>();

        stream << "Number of physical devices: " << device->availablePhysicalDevices().count() << "\n";
        uint32_t deviceIndex = 0;
        for(const VkPhysicalDeviceProperties& props : device->availablePhysicalDevices()) {
            stream << tr("[%8] %1 - Version %2.%3.%4 - API Version %5.%6.%7\n")
                                .arg(props.deviceName)
                                .arg(VK_VERSION_MAJOR(props.driverVersion)).arg(VK_VERSION_MINOR(props.driverVersion))
                                .arg(VK_VERSION_PATCH(props.driverVersion))
                                .arg(VK_VERSION_MAJOR(props.apiVersion)).arg(VK_VERSION_MINOR(props.apiVersion))
                                .arg(VK_VERSION_PATCH(props.apiVersion))
                                .arg(deviceIndex++);
        }
        if(device->logicalDevice())
            stream << "Active physical device index: [" << device->physicalDeviceIndex() << "]\n"; 
        else
            stream << "No active physical device\n"; 
	}
}

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanSceneRenderer::VulkanSceneRenderer(DataSet* dataset, std::shared_ptr<VulkanDevice> vulkanDevice, int concurrentFrameCount) 
    : SceneRenderer(dataset), 
    _device(std::move(vulkanDevice)),
    _concurrentFrameCount(concurrentFrameCount)
{
	OVITO_ASSERT(_device);
    OVITO_ASSERT(_concurrentFrameCount >= 1);

    // Release our own Vulkan resources right before the logical device is destroyed.
    connect(_device.get(), &VulkanDevice::releaseResourcesRequested, this, &VulkanSceneRenderer::releaseResources);
}

/******************************************************************************
* Destructor.
******************************************************************************/
VulkanSceneRenderer::~VulkanSceneRenderer()
{
	releaseResources();
}

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with
* the given other renderer.
******************************************************************************/
bool VulkanSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	// Two Vulkan renderers are compatible when they use the same logical Vulkan device.
	if(VulkanSceneRenderer* otherVulkanRenderer = dynamic_object_cast<VulkanSceneRenderer>(otherRenderer))
		return device() == otherVulkanRenderer->device();
	return false;
}

/******************************************************************************
* Creates the Vulkan resources needed by this renderer.
******************************************************************************/
void VulkanSceneRenderer::initResources()
{
    // Create the resources of the rendering primitives.
    if(!_resourcesInitialized) {
        _linePrimitivePipelines.init(this);
        _resourcesInitialized = true;
    }
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void VulkanSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);

	// This method may only be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

    // Make sure our Vulkan objects have been created.
    initResources();

    // Specify dynamic Vulkan viewport area.
    VkViewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = frameBufferSize().width();
    viewport.height = frameBufferSize().height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    deviceFunctions()->vkCmdSetViewport(currentCommandBuffer(), 0, 1, &viewport);

    // Specify dynamic Vulkan scissor rectangle.
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    deviceFunctions()->vkCmdSetScissor(currentCommandBuffer(), 0, 1, &scissor);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool VulkanSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	// Render the 3D scene objects.
	if(renderScene(operation.subOperation())) {

		// Call subclass to render additional content that is only visible in the interactive viewports.
        if(viewport()) {
    		renderInteractiveContent();
        }
    }

	return !operation.isCanceled();
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void VulkanSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
	SceneRenderer::endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void VulkanSceneRenderer::endRender()
{
	// This method must be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

    // Let the base class release its resources.
	SceneRenderer::endRender();

    // Unless this is a rendered for the interactive viewports, release the resources after rendering is done.
    if(!isInteractive()) {
        // Wait for device to finish all work.
        deviceFunctions()->vkDeviceWaitIdle(logicalDevice());

        // Release the Vulkan resources managed by this class.
        releaseResources();
    }

	// Note: To speed up subsequent rendering passes, the logical Vulkan device is kept alive until the VulkanDevice instance 
	// gets automatically destroyed together with this VulkanSceneRenderer.
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
* Releases all Vulkan resources held by the renderer class.
******************************************************************************/
void VulkanSceneRenderer::releaseResources()
{
	// This method may only be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

	if(!deviceFunctions())
		return;
    if(!_resourcesInitialized)
        return;

    // Destroy the resources of the rendering primitives.
    _resourcesInitialized = false;
    _linePrimitivePipelines.release(this);
}

/******************************************************************************
* Requests a new line geometry buffer from the renderer.
******************************************************************************/
std::shared_ptr<LinePrimitive> VulkanSceneRenderer::createLinePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<VulkanLinePrimitive>(this);
}

/******************************************************************************
* Renders the line geometry stored in the given buffer.
******************************************************************************/
void VulkanSceneRenderer::renderLines(const std::shared_ptr<LinePrimitive>& primitive)
{
    std::shared_ptr<VulkanLinePrimitive> vulkanPrimitive = dynamic_pointer_cast<VulkanLinePrimitive>(primitive);
    OVITO_ASSERT(vulkanPrimitive);
	vulkanPrimitive->render(this, _linePrimitivePipelines);
}

}	// End of namespace
