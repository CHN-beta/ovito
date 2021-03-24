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

        // Look up an existing Vulkan context from one of the interactive viewport windows. 
        // All viewport windows will share the same logical Vulkan device.
        std::shared_ptr<VulkanContext> context;
        for(Viewport* vp : container.currentSet()->viewportConfig()->viewports()) {
            if(ViewportWindowInterface* window = vp->window()) {
                if(VulkanSceneRenderer* renderer = dynamic_object_cast<VulkanSceneRenderer>(window->sceneRenderer())) {
                    context = renderer->context();
                    break;
                }
            }
        }

        // Create an adhoc instance of VulkanContext if needed.
        if(!context)
            context = std::make_shared<VulkanContext>();

        stream << "Number of physical devices: " << context->availablePhysicalDevices().count() << "\n";
        uint32_t deviceIndex = 0;
        for(const VkPhysicalDeviceProperties& props : context->availablePhysicalDevices()) {
            stream << tr("[%8] %1 - Version %2.%3.%4 - API Version %5.%6.%7\n")
                                .arg(props.deviceName)
                                .arg(VK_VERSION_MAJOR(props.driverVersion)).arg(VK_VERSION_MINOR(props.driverVersion))
                                .arg(VK_VERSION_PATCH(props.driverVersion))
                                .arg(VK_VERSION_MAJOR(props.apiVersion)).arg(VK_VERSION_MINOR(props.apiVersion))
                                .arg(VK_VERSION_PATCH(props.apiVersion))
                                .arg(deviceIndex++);
        }
        if(context->logicalDevice()) {
            stream << "Active physical device index: [" << context->physicalDeviceIndex() << "]\n"; 
            stream << "Unified memory architecture: " << context->isUMA() << "\n";
            stream << "features.wideLines: " << context->features().wideLines << "\n";
            stream << "limits.maxUniformBufferRange: " << context->physicalDeviceProperties()->limits.maxUniformBufferRange << "\n";
            stream << "limits.maxStorageBufferRange: " << context->physicalDeviceProperties()->limits.maxStorageBufferRange << "\n";
            stream << "limits.maxPushConstantsSize: " << context->physicalDeviceProperties()->limits.maxPushConstantsSize << "\n";
            stream << "limits.lineWidthRange: " << context->physicalDeviceProperties()->limits.lineWidthRange[0] << " - " << context->physicalDeviceProperties()->limits.lineWidthRange[1] << "\n";
            stream << "limits.lineWidthGranularity: " << context->physicalDeviceProperties()->limits.lineWidthGranularity << "\n";
        }
        else stream << "No active physical device\n"; 
	}
}

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanSceneRenderer::VulkanSceneRenderer(DataSet* dataset, std::shared_ptr<VulkanContext> vulkanContext, int concurrentFrameCount) 
    : SceneRenderer(dataset), 
    _context(std::move(vulkanContext)),
    _concurrentFrameCount(concurrentFrameCount)
{
	OVITO_ASSERT(_context);
    OVITO_ASSERT(_concurrentFrameCount >= 1);

    // Release our own Vulkan resources before the logical device gets destroyed.
    connect(context().get(), &VulkanContext::releaseResourcesRequested, this, &VulkanSceneRenderer::releaseVulkanDeviceResources);
}

/******************************************************************************
* Destructor.
******************************************************************************/
VulkanSceneRenderer::~VulkanSceneRenderer()
{
    // Verify that all Vulkan resources have already been released thanks to a call to aboutToBeDeleted().
    OVITO_ASSERT(_resourcesInitialized == false);
}

/******************************************************************************
* This method is called after the reference counter of this object has reached zero
* and before the object is being finally deleted. 
******************************************************************************/
void VulkanSceneRenderer::aboutToBeDeleted()
{
    // Release any Vulkan resources managed by the renderer.
	releaseVulkanDeviceResources();

    SceneRenderer::aboutToBeDeleted();
}

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with
* the given other renderer.
******************************************************************************/
bool VulkanSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	// Two Vulkan renderers are compatible when they use the same logical Vulkan device.
	if(VulkanSceneRenderer* otherVulkanRenderer = dynamic_object_cast<VulkanSceneRenderer>(otherRenderer))
		return context() == otherVulkanRenderer->context();
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
        _particlePrimitivePipelines.init(this);
        _imagePrimitivePipelines.init(this);
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
	OVITO_ASSERT(QThread::currentThread() == context()->thread());

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

		// Call virtual method to render additional content that is only visible in the interactive viewports.
        if(viewport() && isInteractive()) {
    		renderInteractiveContent();
        }
    }

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
void VulkanSceneRenderer::releaseVulkanDeviceResources()
{
	// This method may only be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == context()->thread());

    if(!_resourcesInitialized)
        return;

	OVITO_ASSERT(deviceFunctions());

    // Destroy the resources of the rendering primitives.
    _linePrimitivePipelines.release(this);
    _particlePrimitivePipelines.release(this);
    _imagePrimitivePipelines.release(this);
    _resourcesInitialized = false;
}

/******************************************************************************
* Creates a new line rendering primitive.
******************************************************************************/
std::shared_ptr<LinePrimitive> VulkanSceneRenderer::createLinePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<VulkanLinePrimitive>();
}

/******************************************************************************
* Creates a new particle rendering primitive.
******************************************************************************/
std::shared_ptr<ParticlePrimitive> VulkanSceneRenderer::createParticlePrimitive(ParticlePrimitive::ShadingMode shadingMode, ParticlePrimitive::RenderingQuality renderingQuality, ParticlePrimitive::ParticleShape shape)
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<VulkanParticlePrimitive>(shadingMode, renderingQuality, shape);
}

/******************************************************************************
* Creates a new image rendering primitive.
******************************************************************************/
std::shared_ptr<ImagePrimitive> VulkanSceneRenderer::createImagePrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<VulkanImagePrimitive>();
}

/******************************************************************************
* Creates a new text rendering primitive.
******************************************************************************/
std::shared_ptr<TextPrimitive> VulkanSceneRenderer::createTextPrimitive()
{
	OVITO_ASSERT(!isBoundingBoxPass());
	return std::make_shared<VulkanTextPrimitive>();
}

/******************************************************************************
* Renders a line primitive.
******************************************************************************/
void VulkanSceneRenderer::renderLines(const std::shared_ptr<LinePrimitive>& primitive)
{
    std::shared_ptr<VulkanLinePrimitive> vulkanPrimitive = dynamic_pointer_cast<VulkanLinePrimitive>(primitive);
    OVITO_ASSERT(vulkanPrimitive);
	vulkanPrimitive->render(this, _linePrimitivePipelines);
}

/******************************************************************************
* Renders a particle primitive.
******************************************************************************/
void VulkanSceneRenderer::renderParticles(const std::shared_ptr<ParticlePrimitive>& primitive)
{
    std::shared_ptr<VulkanParticlePrimitive> vulkanPrimitive = dynamic_pointer_cast<VulkanParticlePrimitive>(primitive);
    OVITO_ASSERT(vulkanPrimitive);
	vulkanPrimitive->render(this, _particlePrimitivePipelines);
}

/******************************************************************************
* Renders an image primitive.
******************************************************************************/
void VulkanSceneRenderer::renderImage(const std::shared_ptr<ImagePrimitive>& primitive)
{
    std::shared_ptr<VulkanImagePrimitive> vulkanPrimitive = dynamic_pointer_cast<VulkanImagePrimitive>(primitive);
    OVITO_ASSERT(vulkanPrimitive);
	vulkanPrimitive->render(this, _imagePrimitivePipelines);
}

/******************************************************************************
* Renders a text primitive.
******************************************************************************/
void VulkanSceneRenderer::renderText(const std::shared_ptr<TextPrimitive>& primitive)
{
    std::shared_ptr<VulkanTextPrimitive> vulkanPrimitive = dynamic_pointer_cast<VulkanTextPrimitive>(primitive);
    OVITO_ASSERT(vulkanPrimitive);
	vulkanPrimitive->render(this);
}

}	// End of namespace
