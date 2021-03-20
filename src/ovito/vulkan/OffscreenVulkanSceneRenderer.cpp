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
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include "OffscreenVulkanSceneRenderer.h"

#include <QVulkanDeviceFunctions>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(OffscreenVulkanSceneRenderer);

/******************************************************************************
* Helper function that looks for an existing logical Vulkan device in the current
* scene which can use for offscreen rendering.
******************************************************************************/
static std::shared_ptr<VulkanDevice> selectVulkanDevice(DataSet* dataset)
{
	// Use the Vulkan device used for the interactive viewport windows
	// also for offscreen rendering if available. 
	for(Viewport* vp : dataset->viewportConfig()->viewports()) {
		if(ViewportWindowInterface* window = vp->window()) {
			if(VulkanSceneRenderer* renderer = dynamic_object_cast<VulkanSceneRenderer>(window->sceneRenderer())) {
				OVITO_ASSERT(renderer->device());
				return renderer->device();
			}
		}
	}

	// Otherwise, create an adhoc Vulkan device just for offscreen rendering.
	return std::make_shared<VulkanDevice>();
}

/******************************************************************************
* Constructor.
******************************************************************************/
OffscreenVulkanSceneRenderer::OffscreenVulkanSceneRenderer(DataSet* dataset, std::shared_ptr<VulkanDevice> vulkanDevice, bool grabDepthBuffer) 
	: VulkanSceneRenderer(dataset, vulkanDevice ? std::move(vulkanDevice) : selectVulkanDevice(dataset)), _grabDepthBuffer(grabDepthBuffer) 
{
}

/******************************************************************************
* Prepares the renderer for rendering and sets the data set that is being rendered.
******************************************************************************/
bool OffscreenVulkanSceneRenderer::startRender(DataSet* dataset, RenderSettings* settings, const QSize& frameBufferSize)
{
	// This method may only be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

	if(!VulkanSceneRenderer::startRender(dataset, settings, frameBufferSize))
		return false;

	OVITO_ASSERT(_colorImage == VK_NULL_HANDLE);
	OVITO_ASSERT(_colorMem == VK_NULL_HANDLE);
	OVITO_ASSERT(_colorView == VK_NULL_HANDLE);
	OVITO_ASSERT(_dsImage == VK_NULL_HANDLE);
	OVITO_ASSERT(_dsMem == VK_NULL_HANDLE);
	OVITO_ASSERT(_dsView == VK_NULL_HANDLE);

	// Initialize the logical Vulkan device.
	if(!device()->create(nullptr))
        throwException(tr("The Vulkan rendering device could not be initialized."));

	// Determine internal framebuffer size when using supersampling.
	_outputSize = frameBufferSize;
	setFrameBufferSize(QSize(_outputSize.width() * antialiasingLevel(), _outputSize.height() * antialiasingLevel()));

	// Create Vulkan color buffer image.
	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	if(!device()->createVulkanImage(this->frameBufferSize(), colorFormat, VK_SAMPLE_COUNT_1_BIT, usage, aspectFlags, &_colorImage, &_colorMem, &_colorView, 1))
		throwException(tr("Could not create Vulkan offscreen image buffer."));

	// Create Vulkan depth-stencil buffer image.
	VkFormat dsFormat = _grabDepthBuffer ? VK_FORMAT_D24_UNORM_S8_UINT : device()->depthStencilFormat();
	usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if(_grabDepthBuffer) {
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if(dsFormat == VK_FORMAT_D24_UNORM_S8_UINT || dsFormat == VK_FORMAT_X8_D24_UNORM_PACK32) _depthBufferBits = 24;
		else if(dsFormat == VK_FORMAT_D16_UNORM || dsFormat == VK_FORMAT_D16_UNORM_S8_UINT) _depthBufferBits = 16;
		else if(dsFormat == VK_FORMAT_D32_SFLOAT || dsFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) _depthBufferBits = 0;
		else {
			dsFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			_depthBufferBits = 24;
		}
	}
	aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	if(!device()->createVulkanImage(this->frameBufferSize(), dsFormat, VK_SAMPLE_COUNT_1_BIT, usage, aspectFlags, &_dsImage, &_dsMem, &_dsView, 1))
		throwException(tr("Could not create Vulkan offscreen depth-buffer image."));

	// Create renderpass.
	std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
	// Color attachment
	attchmentDescriptions[0].format = colorFormat;
	attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	// Depth-stencil attachment
	attchmentDescriptions[1].format = dsFormat;
	attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[1].storeOp = _grabDepthBuffer ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[1].finalLayout = _grabDepthBuffer ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
	renderPassInfo.pAttachments = attchmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();
	VkResult err = deviceFunctions()->vkCreateRenderPass(logicalDevice(), &renderPassInfo, nullptr, &_renderPass);
    if(err != VK_SUCCESS) {
        qWarning("OffscreenVulkanSceneRenderer: Failed to create renderpass: %d", err);
		throwException(tr("Failed to create Vulkan renderpass for offscreen rendering."));
    }
    setDefaultRenderPass(_renderPass);

	// Create Vulkan framebuffer.
	VkImageView attachments[2];
	attachments[0] = _colorView;
	attachments[1] = _dsView;
	VkFramebufferCreateInfo framebufferCreateInfo;
	memset(&framebufferCreateInfo, 0, sizeof(framebufferCreateInfo));
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass = _renderPass;
	framebufferCreateInfo.attachmentCount = 2;
	framebufferCreateInfo.pAttachments = attachments;
	framebufferCreateInfo.width = this->frameBufferSize().width();
	framebufferCreateInfo.height = this->frameBufferSize().height();
	framebufferCreateInfo.layers = 1;
	err = deviceFunctions()->vkCreateFramebuffer(logicalDevice(), &framebufferCreateInfo, nullptr, &_framebuffer);
    if(err != VK_SUCCESS) {
        qWarning("OffscreenVulkanSceneRenderer: Failed to create framebuffer: %d", err);
		throwException(tr("Failed to create Vulkan framebuffer for offscreen rendering."));
    }

	// Create the linear tiled destination image to copy to and to read the memory from.
	VkImageCreateInfo imgCreateInfo;
    memset(&imgCreateInfo, 0, sizeof(imgCreateInfo));
    imgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imgCreateInfo.extent.width = this->frameBufferSize().width();
	imgCreateInfo.extent.height = this->frameBufferSize().height();
	imgCreateInfo.extent.depth = 1;
	imgCreateInfo.arrayLayers = 1;
	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	// Create the Vulkan image for reading back the framebuffer to host memory.
	err = deviceFunctions()->vkCreateImage(logicalDevice(), &imgCreateInfo, nullptr, &_frameGrabImage);
	if(err != VK_SUCCESS) {
        qWarning("OffscreenVulkanSceneRenderer: Failed to create image for readback: %d", err);
        throwException(tr("Failed to create Vulkan image for framebuffer readback."));
    }

	// Create memory to back up the image.
	VkMemoryRequirements memRequirements;
	deviceFunctions()->vkGetImageMemoryRequirements(logicalDevice(), _frameGrabImage, &memRequirements);
	VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memRequirements.size,
        device()->hostVisibleMemoryIndex()
    };
	err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memAllocInfo, nullptr, &_frameGrabImageMem);
	if(err != VK_SUCCESS) {
        qWarning("OffscreenVulkanSceneRenderer: Failed to allocate image memory for readback: %d", err);
        throwException(tr("Failed to allocate Vulkan image memory for framebuffer readback."));
    }
	deviceFunctions()->vkBindImageMemory(logicalDevice(), _frameGrabImage, _frameGrabImageMem, 0);
	if(err != VK_SUCCESS) {
        qWarning("OffscreenVulkanSceneRenderer: Failed to bind readback image memory: %d", err);
        throwException(tr("Failed to bind Vulkan image memory for framebuffer readback."));
    }

	// Create staging buffer for grabbing the depth buffer contents.
	if(_grabDepthBuffer) {
		VkDeviceSize bufferSize = this->frameBufferSize().width() * this->frameBufferSize().height() * (_depthBufferBits != 16 ? 4 : 2);
		VkBufferCreateInfo bufferInfo;
	    memset(&bufferInfo, 0, sizeof(bufferInfo));
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		err = deviceFunctions()->vkCreateBuffer(logicalDevice(), &bufferInfo, nullptr, &_depthGrabBuffer);
		if(err != VK_SUCCESS) {
			qWarning("OffscreenVulkanSceneRenderer: Failed to create staging buffer for reading back depth-buffer: %d", err);
			throwException(tr("Failed to create staging buffer for reading back depth-buffer."));
		}


		VkMemoryRequirements memRequirements;
		deviceFunctions()->vkGetBufferMemoryRequirements(logicalDevice(), _depthGrabBuffer, &memRequirements);

		VkMemoryAllocateInfo memAllocInfo = {
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			memRequirements.size,
			device()->hostVisibleMemoryIndex()
		};
		err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memAllocInfo, nullptr, &_depthGrabBufferMem);
		if(err != VK_SUCCESS) {
			qWarning("OffscreenVulkanSceneRenderer: Failed to allocate staging buffer memory for depth-buffer readback: %d", err);
			throwException(tr("Failed to allocate Vulkan staging buffer memory for depth-buffer readback."));
		}
		deviceFunctions()->vkBindBufferMemory(logicalDevice(), _depthGrabBuffer, _depthGrabBufferMem, 0);
		if(err != VK_SUCCESS) {
			qWarning("OffscreenVulkanSceneRenderer: Failed to bind depth-buffer readback memory: %d", err);
			throwException(tr("Failed to bind Vulkan buffer memory for depth-buffer readback."));
		}
	}
	else _depthBufferData.reset();

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void OffscreenVulkanSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	// This method must be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

	// Allocate a Vulkan command buffer.
    VkCommandBufferAllocateInfo cmdBufInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, device()->graphicsCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    VkResult err = deviceFunctions()->vkAllocateCommandBuffers(logicalDevice(), &cmdBufInfo, &_cmdBuf);
    if(err != VK_SUCCESS) {
		qWarning("OffscreenVulkanSceneRenderer: Failed to allocate frame command buffer: %d", err);
		throwException(tr("Failed to allocate Vulkan frame command buffer."));
    }

	// Pass command buffer to base class implementation.
    setCurrentCommandBuffer(_cmdBuf);

	// Tell the Vulkan resource manager that we are beginning a new frame.
	OVITO_ASSERT(currentResourceFrame() == 0);
	setCurrentResourceFrame(device()->acquireResourceFrame());

	// Begin recording to the Vulkan command buffer.
    VkCommandBufferBeginInfo cmdBufBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT , nullptr };
    err = deviceFunctions()->vkBeginCommandBuffer(_cmdBuf, &cmdBufBeginInfo);
    if(err != VK_SUCCESS) {
		qWarning("OffscreenVulkanSceneRenderer: Failed to begin frame command buffer: %d", err);
		throwException(tr("Failed to begin Vulkan frame command buffer."));
    }

	// Always render with a fully transparent background. 
	// Compositing with the viewport layer content will be performed in an OVITO FrameBuffer. 
    VkClearColorValue clearColor = {{ 0, 0, 0, 0 }};
    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil = clearDS;

	// Begin a render pass.
    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = _renderPass;
    rpBeginInfo.framebuffer = _framebuffer;
    rpBeginInfo.renderArea.extent.width = this->frameBufferSize().width();
    rpBeginInfo.renderArea.extent.height = this->frameBufferSize().height();
    rpBeginInfo.clearValueCount = 2;
    rpBeginInfo.pClearValues = clearValues;
    deviceFunctions()->vkCmdBeginRenderPass(currentCommandBuffer(), &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VulkanSceneRenderer::beginFrame(time, params, vp);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool OffscreenVulkanSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	// This method must be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

	// Let the base class do the main rendering work.
	if(!VulkanSceneRenderer::renderFrame(frameBuffer, stereoTask, std::move(operation)))
		return false;

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void OffscreenVulkanSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
	// This method must be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

    deviceFunctions()->vkCmdEndRenderPass(currentCommandBuffer());

	// Copy framebuffer image to host visible image.

	// Transition destination image to transfer destination layout.
	VkImageMemoryBarrier barrier;
    memset(&barrier, 0, sizeof(barrier));
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;
    barrier.image = _frameGrabImage;
    deviceFunctions()->vkCmdPipelineBarrier(currentCommandBuffer(),
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, 0, nullptr, 0, nullptr,
                                   1, &barrier);

	// Do the actual blit from the offscreen image to our host visible destination image.
    VkImageCopy copyInfo;
    memset(&copyInfo, 0, sizeof(copyInfo));
    copyInfo.srcSubresource.aspectMask = copyInfo.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyInfo.srcSubresource.layerCount = copyInfo.dstSubresource.layerCount = 1;
    copyInfo.extent.width = frameBufferSize().width();
    copyInfo.extent.height = frameBufferSize().height();
    copyInfo.extent.depth = 1;
    deviceFunctions()->vkCmdCopyImage(currentCommandBuffer(), _colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             _frameGrabImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on.
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier.image = _frameGrabImage;
    deviceFunctions()->vkCmdPipelineBarrier(currentCommandBuffer(),
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_HOST_BIT,
                                   0, 0, nullptr, 0, nullptr,
                                   1, &barrier);	

	if(_grabDepthBuffer) {
		VkBufferImageCopy region;
    	memset(&region, 0, sizeof(region));
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { (uint32_t)frameBufferSize().width(), (uint32_t)frameBufferSize().height(), 1 };
		deviceFunctions()->vkCmdCopyImageToBuffer(currentCommandBuffer(), _dsImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _depthGrabBuffer, 1, &region);
	}

    VkResult err = deviceFunctions()->vkEndCommandBuffer(currentCommandBuffer());
    if(err != VK_SUCCESS) {
		qWarning("OffscreenVulkanSceneRenderer: Failed to end frame command buffer: %d", err);
		throwException(tr("Failed to end Vulkan frame command buffer."));
    }

	// Unless rendering has been interrupted, submit draw calls and prepare for reading back the Vulkan framebuffer contents.
	if(renderingSuccessful) {

		// Submit draw calls
		VkCommandBuffer cmdBuf = currentCommandBuffer();
		VkSubmitInfo submitInfo;
		memset(&submitInfo, 0, sizeof(submitInfo));
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuf;
		VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.pWaitDstStageMask = &psf;

		VkFenceCreateInfo fenceInfo;
		memset(&fenceInfo, 0, sizeof(fenceInfo));
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence;
		deviceFunctions()->vkCreateFence(logicalDevice(), &fenceInfo, nullptr, &fence);

		// Submit command buffer to a queue and wait for fence until queue operations have been finished
		err = deviceFunctions()->vkQueueSubmit(device()->graphicsQueue(), 1, &submitInfo, fence);
		if(err != VK_SUCCESS) {
			qWarning("OffscreenVulkanSceneRenderer: Failed to submit commands to Vulkan queue: %d", err);
			throwException(tr("Failed to submit commands to Vulkan queue."));
		}

		// Block until the current frame is done.
		deviceFunctions()->vkWaitForFences(logicalDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
		// Release the fence object.
		deviceFunctions()->vkDestroyFence(logicalDevice(), fence, nullptr);

		// Tell the Vulkan resource manager that we are done rendering the frame.
		device()->releaseResourceFrame(currentResourceFrame());
		setCurrentResourceFrame(0);

		// Get layout of the image (including row pitch).
		VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout layout;
		deviceFunctions()->vkGetImageSubresourceLayout(logicalDevice(), _frameGrabImage, &subres, &layout);

		// Map image memory so we can start copying from it.
		uchar *p;
		err = deviceFunctions()->vkMapMemory(logicalDevice(), _frameGrabImageMem, layout.offset, layout.size, 0, reinterpret_cast<void**>(&p));
		if(err != VK_SUCCESS) {
			qWarning("OffscreenVulkanSceneRenderer: Failed to map readback image memory after transfer: %d", err);
			throwException(tr("Failed to map readback Vulkan image memory after transfer."));
		}

		// Copy pixel data over to a QImage.
		QImage frameGrabTargetImage(frameBufferSize(), QImage::Format_RGBA8888);
		for(int y = 0; y < frameGrabTargetImage.height(); ++y) {
			memcpy(frameGrabTargetImage.scanLine(y), p, frameGrabTargetImage.width() * 4);
			p += layout.rowPitch;
		}
		deviceFunctions()->vkUnmapMemory(logicalDevice(), _frameGrabImageMem);
		
		// Rescale supersampled image to final size.
		QImage scaledImage = frameGrabTargetImage.scaled(_outputSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

		// Transfer acquired image to the output frame buffer.
		if(!frameBuffer->image().isNull()) {
			// If the existing framebuffer is not empty, perform proper alpha blending.
			QPainter painter(&frameBuffer->image());
			painter.drawImage(painter.window(), scaledImage);
		}
		else {
			// If the existing framebuffer is empty, no need to perform blending.
			frameBuffer->image() = scaledImage;
		}
		frameBuffer->update();

		if(_grabDepthBuffer) {
			VkDeviceSize bufferSize = frameBufferSize().width() * frameBufferSize().height() * (_depthBufferBits != 16 ? 4 : 2);
			_depthBufferData = std::make_unique<quint8[]>(bufferSize);
			quint8* p;
            deviceFunctions()->vkMapMemory(logicalDevice(), _depthGrabBufferMem, 0, bufferSize, 0, reinterpret_cast<void**>(&p));
			memcpy(_depthBufferData.get(), p, (size_t)bufferSize);
            deviceFunctions()->vkUnmapMemory(logicalDevice(), _depthGrabBufferMem);
		}
	}

	// Release command buffer.
	if(_cmdBuf) {
		deviceFunctions()->vkFreeCommandBuffers(logicalDevice(), device()->graphicsCommandPool(), 1, &_cmdBuf);
		_cmdBuf = VK_NULL_HANDLE;
	}

	VulkanSceneRenderer::endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void OffscreenVulkanSceneRenderer::endRender()
{
	// This method must be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == device()->thread());

	// Let the base class release its Vulkan resources first.
	// This implicitly waits for the Vulkan device to finish all work.
	VulkanSceneRenderer::endRender();

	// Release Vulkan resources.
	if(_depthGrabBuffer != VK_NULL_HANDLE) {
	    deviceFunctions()->vkDestroyBuffer(logicalDevice(), _depthGrabBuffer, nullptr);
		_depthGrabBuffer = VK_NULL_HANDLE;
	}
	if(_depthGrabBufferMem != VK_NULL_HANDLE) {
	    deviceFunctions()->vkFreeMemory(logicalDevice(), _depthGrabBufferMem, nullptr);
		_depthGrabBufferMem = VK_NULL_HANDLE;
	}
	if(_frameGrabImage != VK_NULL_HANDLE) {
	    deviceFunctions()->vkDestroyImage(logicalDevice(), _frameGrabImage, nullptr);
		_frameGrabImage = VK_NULL_HANDLE;
	}
	if(_frameGrabImageMem != VK_NULL_HANDLE) {
	    deviceFunctions()->vkFreeMemory(logicalDevice(), _frameGrabImageMem, nullptr);
		_frameGrabImageMem = VK_NULL_HANDLE;
	}
	if(_framebuffer != VK_NULL_HANDLE) {
		deviceFunctions()->vkDestroyFramebuffer(logicalDevice(), _framebuffer, nullptr);
		_framebuffer = VK_NULL_HANDLE;
	}
    if(_renderPass != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyRenderPass(logicalDevice(), _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;
	    setDefaultRenderPass(VK_NULL_HANDLE);
    }
    if(_dsView != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyImageView(logicalDevice(), _dsView, nullptr);
        _dsView = VK_NULL_HANDLE;
    }
    if(_dsImage != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyImage(logicalDevice(), _dsImage, nullptr);
        _dsImage = VK_NULL_HANDLE;
    }
    if(_dsMem != VK_NULL_HANDLE) {
        deviceFunctions()->vkFreeMemory(logicalDevice(), _dsMem, nullptr);
        _dsMem = VK_NULL_HANDLE;
    }
    if(_colorView != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyImageView(logicalDevice(), _colorView, nullptr);
        _colorView = VK_NULL_HANDLE;
    }
    if(_colorImage != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyImage(logicalDevice(), _colorImage, nullptr);
        _colorImage = VK_NULL_HANDLE;
    }
    if(_colorMem != VK_NULL_HANDLE) {
        deviceFunctions()->vkFreeMemory(logicalDevice(), _colorMem, nullptr);
        _colorMem = VK_NULL_HANDLE;
    }

	// Note: To speed up subsequent rendering passes, the logical Vulkan device is kept alive until the VulkanDevice instance 
	// gets automatically destroyed together with this VulkanSceneRenderer.
}

}	// End of namespace
