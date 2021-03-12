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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/vulkan/VulkanSceneRenderer.h>
#include "VulkanViewportWindow.h"

#include <QVulkanFunctions>

namespace Ovito {

Q_LOGGING_CATEGORY(lcGuiVk, "qt.vulkan");

OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(VulkanViewportWindow);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanViewportWindow::VulkanViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parentWidget) : 
		WidgetViewportWindow(mainWindow, inputManager, vp)
{
    // Create the logical device wrapper.
    _device = std::make_shared<VulkanDevice>();

    // Make this a Vulkan compatible window.
    setSurfaceType(QSurface::VulkanSurface);
	setVulkanInstance(_device->vulkanInstance());

	// Embed the QWindow in a QWidget container.
	_widget = QWidget::createWindowContainer(this, parentWidget);

//	setMouseTracking(true);
//	setFocusPolicy(Qt::StrongFocus);

	// Create the viewport renderer.
	// It is shared by all viewports of a dataset.
	for(Viewport* vp : viewport()->dataset()->viewportConfig()->viewports()) {
		if(VulkanViewportWindow* window = dynamic_cast<VulkanViewportWindow*>(vp->window())) {
			_viewportRenderer = window->_viewportRenderer;
			if(_viewportRenderer) break;
		}
	}
	if(!_viewportRenderer) {
		_viewportRenderer = new VulkanSceneRenderer(viewport()->dataset());
		_viewportRenderer->setInteractive(true);
	}

	// Create the object picking renderer.
//	_pickingRenderer = new PickingOpenGLSceneRenderer(viewport()->dataset());
//	_pickingRenderer->setInteractive(true);
}

/******************************************************************************
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void VulkanViewportWindow::renderLater()
{
	_updateRequested = true;
    // Request a deferred refresh of the QWindow.
	QWindow::requestUpdate();
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void VulkanViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
		OVITO_ASSERT_MSG(!viewport()->isRendering(), "VulkanViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		OVITO_ASSERT_MSG(!viewport()->dataset()->viewportConfig()->isRendering(), "VulkanViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");

        // Note: All we can do is request a deferred window update. 
        // A QWindow has no way of forcing an immediate repaint.
		QWindow::requestUpdate();
	}
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult VulkanViewportWindow::pick(const QPointF& pos)
{
	ViewportPickResult result;
	return result;
}

Q_DECLARE_LOGGING_CATEGORY(lcGuiVk);

/******************************************************************************
* Is called by the window system whenever an area of the window is invalidated, 
* for example due to the exposure in the windowing system changing
******************************************************************************/
void VulkanViewportWindow::exposeEvent(QExposeEvent*)
{
    if(isExposed()) {
        ensureStarted();
    } 
    else {
//      if(!d->flags.testFlag(PersistentResources)) {
            releaseSwapChain();
            reset();
//      }
    }
}

/******************************************************************************
* Handles events sent to the window by the system.
******************************************************************************/
bool VulkanViewportWindow::event(QEvent* e)
{
    switch(e->type()) {
    case QEvent::UpdateRequest:
        beginFrame();
        break;
    // The swapchain must be destroyed before the surface as per spec. This is
    // not ideal for us because the surface is managed by the QPlatformWindow
    // which may be gone already when the unexpose comes, making the validation
    // layer scream. The solution is to listen to the PlatformSurface events.
    case QEvent::PlatformSurface:
        if(static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            releaseSwapChain();
            reset();
        }
        break;
    default:
        break;
    }
    return QWindow::event(e);
}


/******************************************************************************
* Keeps trying to initialize the Vulkan window surface.
******************************************************************************/
void VulkanViewportWindow::ensureStarted()
{
    if(_status == StatusFailRetry)
        _status = StatusUninitialized;
    if(_status == StatusUninitialized) {
        init();
        if(_status == StatusDeviceReady)
            recreateSwapChain();
    }
    if(_status == StatusReady)
        requestUpdate();
}

/******************************************************************************
* Sets the preferred \a formats of the swapchain.
* By default no application-preferred format is set. In this case the
* surface's preferred format will be used or, in absence of that,
* \c{VK_FORMAT_B8G8R8A8_UNORM}.
*
* The list in \a formats is ordered. If the first format is not supported,
* the second will be considered, and so on. When no formats in the list are
* supported, the behavior is the same as in the default case.
* To query the actual format after initialization, call colorFormat().
*
* This function must be called before the window is made visible.
* 
* Reimplementing preInitResources() allows dynamically examining the list of 
* supported formats, should that be desired. There the surface is retrievable via
* QVulkanInstace::surfaceForWindow(), while this function can still safely be
* called to affect the later stages of initialization.
******************************************************************************/
void VulkanViewportWindow::setPreferredColorFormats(const QVector<VkFormat>& formats)
{
    if(_status != StatusUninitialized) {
        qWarning("VulkanViewportWindow: Attempted to set preferred color format when already initialized");
        return;
    }
    _requestedColorFormats = formats;
}

static struct {
    VkSampleCountFlagBits mask;
    int count;
} qvk_sampleCounts[] = {
    // keep this sorted by 'count'
    { VK_SAMPLE_COUNT_1_BIT, 1 },
    { VK_SAMPLE_COUNT_2_BIT, 2 },
    { VK_SAMPLE_COUNT_4_BIT, 4 },
    { VK_SAMPLE_COUNT_8_BIT, 8 },
    { VK_SAMPLE_COUNT_16_BIT, 16 },
    { VK_SAMPLE_COUNT_32_BIT, 32 },
    { VK_SAMPLE_COUNT_64_BIT, 64 }
};

/******************************************************************************
* Initializes the Vulkan objects of the window after it has been exposed for
* first time.  
******************************************************************************/
void VulkanViewportWindow::init()
{
    OVITO_ASSERT(_status == StatusUninitialized);
    qCDebug(lcGuiVk, "QVulkanWindow init");

    _surface = QVulkanInstance::surfaceForWindow(this);
    if(_surface == VK_NULL_HANDLE) {
        qWarning("VulkanViewportWindow: Failed to retrieve Vulkan surface for window");
        _status = StatusFailRetry;
        return;
    }

    try {
        if(!_device->create(this)) {
            _status = StatusUninitialized;
            qCDebug(lcGuiVk, "Attempting to restart in 2 seconds");
            QTimer::singleShot(2000, this, &VulkanViewportWindow::ensureStarted);
            return;
        }
    }
    catch(const Exception& ex) {
        ex.reportError();
        _status = StatusFail;
        return;
    }

    _hostVisibleMemIndex = 0;
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    bool hostVisibleMemIndexSet = false;
    _device->vulkanFunctions()->vkGetPhysicalDeviceMemoryProperties(_device->physicalDevice(), &physDevMemProps);
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        const VkMemoryType* memType = physDevMemProps.memoryTypes;
        qCDebug(lcGuiVk, "memtype %d: flags=0x%x", i, memType[i].propertyFlags);
        // Find a host visible, host coherent memtype. If there is one that is
        // cached as well (in addition to being coherent), prefer that.
        const int hostVisibleAndCoherent = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if((memType[i].propertyFlags & hostVisibleAndCoherent) == hostVisibleAndCoherent) {
            if(!hostVisibleMemIndexSet || (memType[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
                hostVisibleMemIndexSet = true;
                _hostVisibleMemIndex = i;
            }
        }
    }
    qCDebug(lcGuiVk, "Picked memtype %d for host visible memory", _hostVisibleMemIndex);
    _deviceLocalMemIndex = 0;
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        const VkMemoryType* memType = physDevMemProps.memoryTypes;
        // Just pick the first device local memtype.
        if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            _deviceLocalMemIndex = i;
            break;
        }
    }
    qCDebug(lcGuiVk, "Picked memtype %d for device local memory", _deviceLocalMemIndex);
    if(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR || !vkGetPhysicalDeviceSurfaceFormatsKHR) {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vulkanInstance()->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vulkanInstance()->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
        if(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR || !vkGetPhysicalDeviceSurfaceFormatsKHR) {
            qWarning("VulkanViewportWindow: Physical device surface queries not available");
            _status = StatusFail;
            return;
        }
    }
    // Figure out the color format here. Must not wait until recreateSwapChain()
    // because the renderpass should be available already from initResources (so that apps do not have to defer pipeline creation to initSwapChainResources), 
    // but the renderpass needs the final color format.
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device->physicalDevice(), _surface, &formatCount, nullptr);
    QVector<VkSurfaceFormatKHR> formats(formatCount);
    if(formatCount)
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device->physicalDevice(), _surface, &formatCount, formats.data());
    _colorFormat = VK_FORMAT_B8G8R8A8_UNORM; // our documented default if all else fails
    _colorSpace = VkColorSpaceKHR(0); // this is in fact VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    // Pick the preferred format, if there is one.
    if(!formats.isEmpty() && formats[0].format != VK_FORMAT_UNDEFINED) {
        _colorFormat = formats[0].format;
        _colorSpace = formats[0].colorSpace;
    }
    // Try to honor the user request.
    if(!formats.isEmpty() && !_requestedColorFormats.isEmpty()) {
        for(VkFormat reqFmt : qAsConst(_requestedColorFormats)) {
            auto r = std::find_if(formats.cbegin(), formats.cend(), [reqFmt](const VkSurfaceFormatKHR &sfmt) { return sfmt.format == reqFmt; });
            if(r != formats.cend()) {
                _colorFormat = r->format;
                _colorSpace = r->colorSpace;
                break;
            }
        }
    }
    const VkFormat dsFormatCandidates[] = {
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT
    };
    const int dsFormatCandidateCount = sizeof(dsFormatCandidates) / sizeof(VkFormat);
    int dsFormatIdx = 0;
    while(dsFormatIdx < dsFormatCandidateCount) {
        _dsFormat = dsFormatCandidates[dsFormatIdx];
        VkFormatProperties fmtProp;
        _device->vulkanFunctions()->vkGetPhysicalDeviceFormatProperties(_device->physicalDevice(), _dsFormat, &fmtProp);
        if (fmtProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            break;
        ++dsFormatIdx;
    }
    if(dsFormatIdx == dsFormatCandidateCount)
        qWarning("VulkanViewportWindow: Failed to find an optimal depth-stencil format");
    qCDebug(lcGuiVk, "Color format: %d Depth-stencil format: %d", _colorFormat, _dsFormat);
    if(!createDefaultRenderPass())
        return;
    initResources();
    _status = StatusDeviceReady;
}

/******************************************************************************
* Creates the default Vulkan render pass.  
******************************************************************************/
bool VulkanViewportWindow::createDefaultRenderPass()
{
    VkAttachmentDescription attDesc[3];
    memset(attDesc, 0, sizeof(attDesc));
    const bool msaa = _sampleCount > VK_SAMPLE_COUNT_1_BIT;
    // This is either the non-msaa render target or the resolve target.
    attDesc[0].format = _colorFormat;
    attDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // ignored when msaa
    attDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attDesc[1].format = _dsFormat;
    attDesc[1].samples = _sampleCount;
    attDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if(msaa) {
        // msaa render target
        attDesc[2].format = _colorFormat;
        attDesc[2].samples = _sampleCount;
        attDesc[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference resolveRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference dsRef = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subPassDesc;
    memset(&subPassDesc, 0, sizeof(subPassDesc));
    subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDesc.colorAttachmentCount = 1;
    subPassDesc.pColorAttachments = &colorRef;
    subPassDesc.pDepthStencilAttachment = &dsRef;
    VkRenderPassCreateInfo rpInfo;
    memset(&rpInfo, 0, sizeof(rpInfo));
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 2;
    rpInfo.pAttachments = attDesc;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subPassDesc;
    if(msaa) {
        colorRef.attachment = 2;
        subPassDesc.pResolveAttachments = &resolveRef;
        rpInfo.attachmentCount = 3;
    }
    VkResult err = deviceFunctions()->vkCreateRenderPass(logicalDevice(), &rpInfo, nullptr, &_defaultRenderPass);
    if(err != VK_SUCCESS) {
        qWarning("VulkanViewportWindow: Failed to create renderpass: %d", err);
        return false;
    }
    return true;
}

/******************************************************************************
* Recreates the Vulkan swapchain.  
******************************************************************************/
void VulkanViewportWindow::recreateSwapChain()
{
    OVITO_ASSERT(_status >= StatusDeviceReady);
    _swapChainImageSize = size() * devicePixelRatio(); // note: may change below due to surfaceCaps
    if(_swapChainImageSize.isEmpty()) // handle null window size gracefully
        return;
    QVulkanInstance* inst = vulkanInstance();
    QVulkanFunctions* f = inst->functions();
    deviceFunctions()->vkDeviceWaitIdle(logicalDevice());
    if(!vkCreateSwapchainKHR) {
        vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(f->vkGetDeviceProcAddr(logicalDevice(), "vkCreateSwapchainKHR"));
        vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(f->vkGetDeviceProcAddr(logicalDevice(), "vkDestroySwapchainKHR"));
        vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(f->vkGetDeviceProcAddr(logicalDevice(), "vkGetSwapchainImagesKHR"));
        vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(f->vkGetDeviceProcAddr(logicalDevice(), "vkAcquireNextImageKHR"));
        vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(f->vkGetDeviceProcAddr(logicalDevice(), "vkQueuePresentKHR"));
    }
    VkPhysicalDevice physDev = _device->physicalDevice();
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, _surface, &surfaceCaps);
    uint32_t reqBufferCount = _swapChainBufferCount;
    if(surfaceCaps.maxImageCount)
        reqBufferCount = qBound(surfaceCaps.minImageCount, reqBufferCount, surfaceCaps.maxImageCount);
    VkExtent2D bufferSize = surfaceCaps.currentExtent;
    if(bufferSize.width == uint32_t(-1)) {
        Q_ASSERT(bufferSize.height == uint32_t(-1));
        bufferSize.width = _swapChainImageSize.width();
        bufferSize.height = _swapChainImageSize.height();
    } 
    else {
        _swapChainImageSize = QSize(bufferSize.width, bufferSize.height);
    }
    VkSurfaceTransformFlagBitsKHR preTransform =
        (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        : surfaceCaps.currentTransform;
    VkCompositeAlphaFlagBitsKHR compositeAlpha =
        (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if(requestedFormat().hasAlpha()) {
        if(surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        else if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkSwapchainKHR oldSwapChain = _swapChain;
    VkSwapchainCreateInfoKHR swapChainInfo;
    memset(&swapChainInfo, 0, sizeof(swapChainInfo));
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = _surface;
    swapChainInfo.minImageCount = reqBufferCount;
    swapChainInfo.imageFormat = _colorFormat;
    swapChainInfo.imageColorSpace = _colorSpace;
    swapChainInfo.imageExtent = bufferSize;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = usage;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = preTransform;
    swapChainInfo.compositeAlpha = compositeAlpha;
    swapChainInfo.presentMode = _presentMode;
    swapChainInfo.clipped = true;
    swapChainInfo.oldSwapchain = oldSwapChain;
    qCDebug(lcGuiVk, "Creating new swap chain of %d buffers, size %dx%d", reqBufferCount, bufferSize.width, bufferSize.height);
    VkSwapchainKHR newSwapChain;
    VkResult err = vkCreateSwapchainKHR(logicalDevice(), &swapChainInfo, nullptr, &newSwapChain);
    if(err != VK_SUCCESS) {
        qWarning("VulkanViewportWindow: Failed to create swap chain: %d", err);
        return;
    }
    if(oldSwapChain)
        releaseSwapChain();
    _swapChain = newSwapChain;
    uint32_t actualSwapChainBufferCount = 0;
    err = vkGetSwapchainImagesKHR(logicalDevice(), _swapChain, &actualSwapChainBufferCount, nullptr);
    if(err != VK_SUCCESS || actualSwapChainBufferCount < 2) {
        qWarning("VulkanViewportWindow: Failed to get swapchain images: %d (count=%d)", err, actualSwapChainBufferCount);
        return;
    }
    qCDebug(lcGuiVk, "Actual swap chain buffer count: %d", actualSwapChainBufferCount);
    if(actualSwapChainBufferCount > MAX_SWAPCHAIN_BUFFER_COUNT) {
        qWarning("VulkanViewportWindow: Too many swapchain buffers (%d)", actualSwapChainBufferCount);
        return;
    }
    _swapChainBufferCount = actualSwapChainBufferCount;
    VkImage swapChainImages[MAX_SWAPCHAIN_BUFFER_COUNT];
    err = vkGetSwapchainImagesKHR(logicalDevice(), _swapChain, &actualSwapChainBufferCount, swapChainImages);
    if(err != VK_SUCCESS) {
        qWarning("VulkanViewportWindow: Failed to get swapchain images: %d", err);
        return;
    }
    if(!createTransientImage(_dsFormat,
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                              VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                              &_dsImage,
                              &_dsMem,
                              &_dsView,
                              1))
    {
        return;
    }
    const bool msaa = _sampleCount > VK_SAMPLE_COUNT_1_BIT;
    VkImage msaaImages[MAX_SWAPCHAIN_BUFFER_COUNT];
    VkImageView msaaViews[MAX_SWAPCHAIN_BUFFER_COUNT];
    if(msaa) {
        if(!createTransientImage(_colorFormat,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                  msaaImages,
                                  &_msaaImageMem,
                                  msaaViews,
                                  _swapChainBufferCount))
        {
            return;
        }
    }
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    for(int i = 0; i < _swapChainBufferCount; ++i) {
        ImageResources &image(_imageRes[i]);
        image.image = swapChainImages[i];
        if(msaa) {
            image.msaaImage = msaaImages[i];
            image.msaaImageView = msaaViews[i];
        }
        VkImageViewCreateInfo imgViewInfo;
        memset(&imgViewInfo, 0, sizeof(imgViewInfo));
        imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewInfo.image = swapChainImages[i];
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewInfo.format = _colorFormat;
        imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgViewInfo.subresourceRange.levelCount = imgViewInfo.subresourceRange.layerCount = 1;
        err = deviceFunctions()->vkCreateImageView(logicalDevice(), &imgViewInfo, nullptr, &image.imageView);
        if(err != VK_SUCCESS) {
            qWarning("VulkanViewportWindow: Failed to create swapchain image view %d: %d", i, err);
            return;
        }
        err = deviceFunctions()->vkCreateFence(logicalDevice(), &fenceInfo, nullptr, &image.cmdFence);
        if(err != VK_SUCCESS) {
            qWarning("VulkanViewportWindow: Failed to create command buffer fence: %d", err);
            return;
        }
        image.cmdFenceWaitable = true; // fence was created in signaled state
        VkImageView views[3] = { image.imageView,
                                 _dsView,
                                 msaa ? image.msaaImageView : VK_NULL_HANDLE };
        VkFramebufferCreateInfo fbInfo;
        memset(&fbInfo, 0, sizeof(fbInfo));
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = _defaultRenderPass;
        fbInfo.attachmentCount = msaa ? 3 : 2;
        fbInfo.pAttachments = views;
        fbInfo.width = _swapChainImageSize.width();
        fbInfo.height = _swapChainImageSize.height();
        fbInfo.layers = 1;
        VkResult err = deviceFunctions()->vkCreateFramebuffer(logicalDevice(), &fbInfo, nullptr, &image.fb);
        if(err != VK_SUCCESS) {
            qWarning("VulkanViewportWindow: Failed to create framebuffer: %d", err);
            return;
        }
        if(_device->separatePresentQueue()) {
            // Pre-build the static image-acquire-on-present-queue command buffer.
            VkCommandBufferAllocateInfo cmdBufInfo = {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, _device->presentCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
            err = deviceFunctions()->vkAllocateCommandBuffers(logicalDevice(), &cmdBufInfo, &image.presTransCmdBuf);
            if(err != VK_SUCCESS) {
                qWarning("VulkanViewportWindow: Failed to allocate acquire-on-present-queue command buffer: %d", err);
                return;
            }
            VkCommandBufferBeginInfo cmdBufBeginInfo = {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
                VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr };
            err = deviceFunctions()->vkBeginCommandBuffer(image.presTransCmdBuf, &cmdBufBeginInfo);
            if(err != VK_SUCCESS) {
                qWarning("VulkanViewportWindow: Failed to begin acquire-on-present-queue command buffer: %d", err);
                return;
            }
            VkImageMemoryBarrier presTrans;
            memset(&presTrans, 0, sizeof(presTrans));
            presTrans.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            presTrans.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            presTrans.oldLayout = presTrans.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            presTrans.srcQueueFamilyIndex = _device->graphicsQueueFamilyIndex();
            presTrans.dstQueueFamilyIndex = _device->presentQueueFamilyIndex();
            presTrans.image = image.image;
            presTrans.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            presTrans.subresourceRange.levelCount = presTrans.subresourceRange.layerCount = 1;
            deviceFunctions()->vkCmdPipelineBarrier(image.presTransCmdBuf,
                                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                           0, 0, nullptr, 0, nullptr,
                                           1, &presTrans);
            err = deviceFunctions()->vkEndCommandBuffer(image.presTransCmdBuf);
            if(err != VK_SUCCESS) {
                qWarning("VulkanViewportWindow: Failed to end acquire-on-present-queue command buffer: %d", err);
                return;
            }
        }
    }
    _currentImage = 0;
    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    for(int i = 0; i < _frameLag; ++i) {
        FrameResources& frame = _frameRes[i];
        frame.imageAcquired = false;
        frame.imageSemWaitable = false;
        deviceFunctions()->vkCreateFence(logicalDevice(), &fenceInfo, nullptr, &frame.fence);
        frame.fenceWaitable = true; // fence was created in signaled state
        deviceFunctions()->vkCreateSemaphore(logicalDevice(), &semInfo, nullptr, &frame.imageSem);
        deviceFunctions()->vkCreateSemaphore(logicalDevice(), &semInfo, nullptr, &frame.drawSem);
        if(_device->separatePresentQueue())
            deviceFunctions()->vkCreateSemaphore(logicalDevice(), &semInfo, nullptr, &frame.presTransSem);
    }
    _currentFrame = 0;
    initSwapChainResources();
    _status = StatusReady;
}

/******************************************************************************
* Creates a Vulkan image.
******************************************************************************/
bool VulkanViewportWindow::createTransientImage(VkFormat format,
                                                VkImageUsageFlags usage,
                                                VkImageAspectFlags aspectMask,
                                                VkImage *images,
                                                VkDeviceMemory *mem,
                                                VkImageView *views,
                                                int count)
{
    VkMemoryRequirements memReq;
    VkResult err;
    for(int i = 0; i < count; ++i) {
        VkImageCreateInfo imgInfo;
        memset(&imgInfo, 0, sizeof(imgInfo));
        imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType = VK_IMAGE_TYPE_2D;
        imgInfo.format = format;
        imgInfo.extent.width = _swapChainImageSize.width();
        imgInfo.extent.height = _swapChainImageSize.height();
        imgInfo.extent.depth = 1;
        imgInfo.mipLevels = imgInfo.arrayLayers = 1;
        imgInfo.samples = _sampleCount;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        err = deviceFunctions()->vkCreateImage(logicalDevice(), &imgInfo, nullptr, images + i);
        if(err != VK_SUCCESS) {
            qWarning("VulkanViewportWindow: Failed to create image: %d", err);
            return false;
        }
        // Assume the reqs are the same since the images are same in every way.
        // Still, call GetImageMemReq for every image, in order to prevent the
        // validation layer from complaining.
        deviceFunctions()->vkGetImageMemoryRequirements(logicalDevice(), images[i], &memReq);
    }
    VkMemoryAllocateInfo memInfo;
    memset(&memInfo, 0, sizeof(memInfo));
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.allocationSize = aligned(memReq.size, memReq.alignment) * count;
    uint32_t startIndex = 0;
    do {
        memInfo.memoryTypeIndex = _device->chooseTransientImageMemType(images[0], startIndex);
        if(memInfo.memoryTypeIndex == uint32_t(-1)) {
            qWarning("VulkanViewportWindow: No suitable memory type found");
            return false;
        }
        startIndex = memInfo.memoryTypeIndex + 1;
        qCDebug(lcGuiVk, "Allocating %u bytes for transient image (memtype %u)",
                uint32_t(memInfo.allocationSize), memInfo.memoryTypeIndex);
        err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memInfo, nullptr, mem);
        if(err != VK_SUCCESS && err != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            qWarning("VulkanViewportWindow: Failed to allocate image memory: %d", err);
            return false;
        }
    } 
    while(err != VK_SUCCESS);
    VkDeviceSize ofs = 0;
    for(int i = 0; i < count; ++i) {
        err = deviceFunctions()->vkBindImageMemory(logicalDevice(), images[i], *mem, ofs);
        if(err != VK_SUCCESS) {
            qWarning("VulkanViewportWindow: Failed to bind image memory: %d", err);
            return false;
        }
        ofs += aligned(memReq.size, memReq.alignment);
        VkImageViewCreateInfo imgViewInfo;
        memset(&imgViewInfo, 0, sizeof(imgViewInfo));
        imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewInfo.image = images[i];
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewInfo.format = format;
        imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgViewInfo.subresourceRange.aspectMask = aspectMask;
        imgViewInfo.subresourceRange.levelCount = imgViewInfo.subresourceRange.layerCount = 1;
        err = deviceFunctions()->vkCreateImageView(logicalDevice(), &imgViewInfo, nullptr, views + i);
        if(err != VK_SUCCESS) {
            qWarning("VulkanViewportWindow: Failed to create image view: %d", err);
            return false;
        }
    }
    return true;
}

/******************************************************************************
* Releases the resources of the Vulkan swapchain.  
******************************************************************************/
void VulkanViewportWindow::releaseSwapChain()
{
    if(!logicalDevice() || !_swapChain) // do not rely on 'status', a half done init must be cleaned properly too
        return;
    qCDebug(lcGuiVk, "Releasing swapchain");
    deviceFunctions()->vkDeviceWaitIdle(logicalDevice());
    releaseSwapChainResources();
    deviceFunctions()->vkDeviceWaitIdle(logicalDevice());
    for(int i = 0; i < _frameLag; ++i) {
        FrameResources& frame = _frameRes[i];
        if(frame.fence) {
            if(frame.fenceWaitable)
                deviceFunctions()->vkWaitForFences(logicalDevice(), 1, &frame.fence, VK_TRUE, UINT64_MAX);
            deviceFunctions()->vkDestroyFence(logicalDevice(), frame.fence, nullptr);
            frame.fence = VK_NULL_HANDLE;
            frame.fenceWaitable = false;
        }
        if(frame.imageSem) {
            deviceFunctions()->vkDestroySemaphore(logicalDevice(), frame.imageSem, nullptr);
            frame.imageSem = VK_NULL_HANDLE;
        }
        if(frame.drawSem) {
            deviceFunctions()->vkDestroySemaphore(logicalDevice(), frame.drawSem, nullptr);
            frame.drawSem = VK_NULL_HANDLE;
        }
        if(frame.presTransSem) {
            deviceFunctions()->vkDestroySemaphore(logicalDevice(), frame.presTransSem, nullptr);
            frame.presTransSem = VK_NULL_HANDLE;
        }
    }
    for(int i = 0; i < _swapChainBufferCount; ++i) {
        ImageResources& image = _imageRes[i];
        if(image.cmdFence) {
            if(image.cmdFenceWaitable)
                deviceFunctions()->vkWaitForFences(logicalDevice(), 1, &image.cmdFence, VK_TRUE, UINT64_MAX);
            deviceFunctions()->vkDestroyFence(logicalDevice(), image.cmdFence, nullptr);
            image.cmdFence = VK_NULL_HANDLE;
            image.cmdFenceWaitable = false;
        }
        if(image.fb) {
            deviceFunctions()->vkDestroyFramebuffer(logicalDevice(), image.fb, nullptr);
            image.fb = VK_NULL_HANDLE;
        }
        if(image.imageView) {
            deviceFunctions()->vkDestroyImageView(logicalDevice(), image.imageView, nullptr);
            image.imageView = VK_NULL_HANDLE;
        }
        if(image.cmdBuf) {
            deviceFunctions()->vkFreeCommandBuffers(logicalDevice(), _device->graphicsCommandPool(), 1, &image.cmdBuf);
            image.cmdBuf = VK_NULL_HANDLE;
        }
        if(image.presTransCmdBuf) {
            deviceFunctions()->vkFreeCommandBuffers(logicalDevice(), _device->presentCommandPool(), 1, &image.presTransCmdBuf);
            image.presTransCmdBuf = VK_NULL_HANDLE;
        }
        if(image.msaaImageView) {
            deviceFunctions()->vkDestroyImageView(logicalDevice(), image.msaaImageView, nullptr);
            image.msaaImageView = VK_NULL_HANDLE;
        }
        if(image.msaaImage) {
            deviceFunctions()->vkDestroyImage(logicalDevice(), image.msaaImage, nullptr);
            image.msaaImage = VK_NULL_HANDLE;
        }
    }
    if(_msaaImageMem) {
        deviceFunctions()->vkFreeMemory(logicalDevice(), _msaaImageMem, nullptr);
        _msaaImageMem = VK_NULL_HANDLE;
    }
    if(_dsView) {
        deviceFunctions()->vkDestroyImageView(logicalDevice(), _dsView, nullptr);
        _dsView = VK_NULL_HANDLE;
    }
    if(_dsImage) {
        deviceFunctions()->vkDestroyImage(logicalDevice(), _dsImage, nullptr);
        _dsImage = VK_NULL_HANDLE;
    }
    if(_dsMem) {
        deviceFunctions()->vkFreeMemory(logicalDevice(), _dsMem, nullptr);
        _dsMem = VK_NULL_HANDLE;
    }
    if(_swapChain) {
        vkDestroySwapchainKHR(logicalDevice(), _swapChain, nullptr);
        _swapChain = VK_NULL_HANDLE;
    }
    if(_status == StatusReady)
        _status = StatusDeviceReady;
}

/******************************************************************************
* Handles a Vulkan device that was recently lost.
******************************************************************************/
bool VulkanViewportWindow::checkDeviceLost(VkResult err)
{
    if(err == VK_ERROR_DEVICE_LOST) {
        qWarning("VulkanViewportWindow: Device lost");
        logicalDeviceLost();
        qCDebug(lcGuiVk, "Releasing all resources due to device lost");
        releaseSwapChain();
        reset();
        qCDebug(lcGuiVk, "Restarting");
        ensureStarted();
        return true;
    }
    return false;
}

/******************************************************************************
* Starts rendering a frame.
******************************************************************************/
void VulkanViewportWindow::beginFrame()
{
    if(!_swapChain || _framePending)
        return;

    // Handle window being resized.
    if(size() * devicePixelRatio() != _swapChainImageSize) {
        recreateSwapChain();
        if(!_swapChain)
            return;
    }

    FrameResources& frame = _frameRes[_currentFrame];
    if(!frame.imageAcquired) {
        // Wait if we are too far ahead, i.e. the thread gets throttled based on the presentation rate
        // (note that we are using FIFO mode -> vsync)
        if(frame.fenceWaitable) {
            deviceFunctions()->vkWaitForFences(logicalDevice(), 1, &frame.fence, VK_TRUE, UINT64_MAX);
            deviceFunctions()->vkResetFences(logicalDevice(), 1, &frame.fence);
            frame.fenceWaitable = false;
        }
        // move on to next swapchain image
        VkResult err = vkAcquireNextImageKHR(logicalDevice(), _swapChain, UINT64_MAX,
                                             frame.imageSem, frame.fence, &_currentImage);
        if(err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR) {
            frame.imageSemWaitable = true;
            frame.imageAcquired = true;
            frame.fenceWaitable = true;
        } 
        else if(err == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            requestUpdate();
            return;
        } 
        else {
            if(!checkDeviceLost(err))
                qWarning("VulkanViewportWindow: Failed to acquire next swapchain image: %d", err);
            requestUpdate();
            return;
        }
    }
    // Make sure the previous draw for the same image has finished
    ImageResources& image = _imageRes[_currentImage];
    if(image.cmdFenceWaitable) {
        deviceFunctions()->vkWaitForFences(logicalDevice(), 1, &image.cmdFence, VK_TRUE, UINT64_MAX);
        deviceFunctions()->vkResetFences(logicalDevice(), 1, &image.cmdFence);
        image.cmdFenceWaitable = false;
    }
    // Build new draw command buffer
    if(image.cmdBuf) {
        deviceFunctions()->vkFreeCommandBuffers(logicalDevice(), _device->graphicsCommandPool(), 1, &image.cmdBuf);
        image.cmdBuf = 0;
    }
    VkCommandBufferAllocateInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, _device->graphicsCommandPool(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    VkResult err = deviceFunctions()->vkAllocateCommandBuffers(logicalDevice(), &cmdBufInfo, &image.cmdBuf);
    if(err != VK_SUCCESS) {
        if(!checkDeviceLost(err))
            qWarning("VulkanViewportWindow: Failed to allocate frame command buffer: %d", err);
        return;
    }
    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
    err = deviceFunctions()->vkBeginCommandBuffer(image.cmdBuf, &cmdBufBeginInfo);
    if(err != VK_SUCCESS) {
        if(!checkDeviceLost(err))
            qWarning("VulkanViewportWindow: Failed to begin frame command buffer: %d", err);
        return;
    }
    _framePending = true;
    startNextFrame();
    // Done for now - endFrame() will get invoked when frameReady() is called back.
}

/******************************************************************************
* Finishes rendering a frame.
******************************************************************************/
void VulkanViewportWindow::endFrame()
{
    FrameResources& frame = _frameRes[_currentFrame];
    ImageResources& image = _imageRes[_currentImage];
    if(_device->separatePresentQueue()) {
        // Add the swapchain image release to the command buffer that will be
        // submitted to the graphics queue.
        VkImageMemoryBarrier presTrans;
        memset(&presTrans, 0, sizeof(presTrans));
        presTrans.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presTrans.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presTrans.oldLayout = presTrans.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presTrans.srcQueueFamilyIndex = _device->graphicsQueueFamilyIndex();
        presTrans.dstQueueFamilyIndex = _device->presentQueueFamilyIndex();
        presTrans.image = image.image;
        presTrans.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presTrans.subresourceRange.levelCount = presTrans.subresourceRange.layerCount = 1;
        deviceFunctions()->vkCmdPipelineBarrier(image.cmdBuf,
                                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                       0, 0, nullptr, 0, nullptr,
                                       1, &presTrans);
    }
    VkResult err = deviceFunctions()->vkEndCommandBuffer(image.cmdBuf);
    if(err != VK_SUCCESS) {
        if(!checkDeviceLost(err))
            qWarning("VulkanViewportWindow: Failed to end frame command buffer: %d", err);
        return;
    }
    // Submit draw calls
    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &image.cmdBuf;
    if(frame.imageSemWaitable) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &frame.imageSem;
    }
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &frame.drawSem;
    VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &psf;
    Q_ASSERT(!image.cmdFenceWaitable);
    err = deviceFunctions()->vkQueueSubmit(_device->graphicsQueue(), 1, &submitInfo, image.cmdFence);
    if(err == VK_SUCCESS) {
        frame.imageSemWaitable = false;
        image.cmdFenceWaitable = true;
    } 
    else {
        if(!checkDeviceLost(err))
            qWarning("VulkanViewportWindow: Failed to submit to graphics queue: %d", err);
        return;
    }
    if(_device->separatePresentQueue()) {
        // Submit the swapchain image acquire to the present queue.
        submitInfo.pWaitSemaphores = &frame.drawSem;
        submitInfo.pSignalSemaphores = &frame.presTransSem;
        submitInfo.pCommandBuffers = &image.presTransCmdBuf; // must be USAGE_SIMULTANEOUS
        err = deviceFunctions()->vkQueueSubmit(_device->presentQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        if(err != VK_SUCCESS) {
            if(!checkDeviceLost(err))
                qWarning("VulkanViewportWindow: Failed to submit to present queue: %d", err);
            return;
        }
    }
    // Queue present
    VkPresentInfoKHR presInfo;
    memset(&presInfo, 0, sizeof(presInfo));
    presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presInfo.swapchainCount = 1;
    presInfo.pSwapchains = &_swapChain;
    presInfo.pImageIndices = &_currentImage;
    presInfo.waitSemaphoreCount = 1;
    presInfo.pWaitSemaphores = !_device->separatePresentQueue() ? &frame.drawSem : &frame.presTransSem;
    err = vkQueuePresentKHR(_device->graphicsQueue(), &presInfo);
    if(err != VK_SUCCESS) {
        if(err == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            requestUpdate();
            return;
        } 
        else if(err != VK_SUBOPTIMAL_KHR) {
            if(!checkDeviceLost(err))
                qWarning("VulkanViewportWindow: Failed to present: %d", err);
            return;
        }
    }
    frame.imageAcquired = false;
    vulkanInstance()->presentQueued(this);
    _currentFrame = (_currentFrame + 1) % concurrentFrameCount();
}

/******************************************************************************
* Releases all Vulkan resources.
******************************************************************************/
void VulkanViewportWindow::reset()
{
    if(!logicalDevice()) // Do not rely on 'status', a half done init must be cleaned properly too
        return;
    qCDebug(lcGuiVk, "VulkanViewportWindow reset");
    deviceFunctions()->vkDeviceWaitIdle(logicalDevice());
    releaseResources();
    deviceFunctions()->vkDeviceWaitIdle(logicalDevice());
    if(_defaultRenderPass) {
        deviceFunctions()->vkDestroyRenderPass(logicalDevice(), _defaultRenderPass, nullptr);
        _defaultRenderPass = VK_NULL_HANDLE;
    }
    _device->reset();
    _surface = VK_NULL_HANDLE;
    _status = StatusUninitialized;
}

/******************************************************************************
* This function must be called exactly once in response to each invocation of
* the startNextFrame() implementation. At the time of
* this call, the main command buffer, exposed via currentCommandBuffer(),
* must have all necessary rendering commands added to it since this function
* will trigger submitting the commands and queuing the present command.
* This function must only be called from the gui/main thread.
******************************************************************************/
void VulkanViewportWindow::frameReady()
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread(), "VulkanViewportWindow", "frameReady() can only be called from the GUI (main) thread");
    if(!_framePending) {
        qWarning("VulkanViewportWindow: frameReady() called without a corresponding startNextFrame()");
        return;
    }
    _framePending = false;
    endFrame();
}

// Note that the vertex data and the projection matrix assume OpenGL. With
// Vulkan Y is negated in clip space and the near/far plane is at 0/1 instead
// of -1/1. These will be corrected for by an extra transformation when
// calculating the modelview-projection matrix.
static float vertexData[] = { // Y up, front = CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
};

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);

void VulkanViewportWindow::startNextFrame()
{
    QVulkanDeviceFunctions* deviceFuncs = vulkanInstance()->deviceFunctions(logicalDevice());
    VkCommandBuffer cb = currentCommandBuffer();
    const QSize sz = swapChainImageSize();

    // Projection matrix
    m_proj = m_clipCorrect; // adjust for Vulkan-OpenGL clip space differences
    m_proj.perspective(45.0f, sz.width() / (float) sz.height(), 0.01f, 100.0f);
    m_proj.translate(0, 0, -4);

    VkClearColorValue clearColor = {{ 0, 0, 0, 1 }};
    VkClearDepthStencilValue clearDS = { 1, 0 };
    VkClearValue clearValues[3];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = clearDS;

    VkRenderPassBeginInfo rpBeginInfo;
    memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
    rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.renderPass = defaultRenderPass();
    rpBeginInfo.framebuffer = currentFramebuffer();
    rpBeginInfo.renderArea.extent.width = sz.width();
    rpBeginInfo.renderArea.extent.height = sz.height();
    rpBeginInfo.clearValueCount = sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    rpBeginInfo.pClearValues = clearValues;
    VkCommandBuffer cmdBuf = currentCommandBuffer();
    deviceFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	_updateRequested = false;

	// Do not re-enter rendering function of the same viewport.
	if(viewport() && !viewport()->isRendering()) {
        if(!viewport()->dataset()->viewportConfig()->isSuspended()) {
            try {
                // Let the Viewport class do the actual rendering work.
                viewport()->renderInteractive(_viewportRenderer);
            }
            catch(Exception& ex) {
                if(ex.context() == nullptr) ex.setContext(viewport()->dataset());
                ex.prependGeneralMessage(tr("An unexpected error occurred while rendering the viewport contents. The program will quit."));
                viewport()->dataset()->viewportConfig()->suspendViewportUpdates();

                QCoreApplication::removePostedEvents(nullptr, 0);
                if(mainWindow())
                    mainWindow()->closeMainWindow();
                ex.reportError(true);
                QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
                QCoreApplication::exit();
            }
        }
        else {
            // Make sure viewport gets refreshed as soon as updates are enabled again.
            viewport()->dataset()->viewportConfig()->updateViewports();
        }
    }

    quint8 *p;
    VkResult err = deviceFuncs->vkMapMemory(logicalDevice(), m_bufMem, m_uniformBufInfo[currentFrame()].offset, UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if(err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    QMatrix4x4 m = m_proj;
    m.rotate(m_rotation, 0, 1, 0);
    memcpy(p, m.constData(), 16 * sizeof(float));
    deviceFuncs->vkUnmapMemory(logicalDevice(), m_bufMem);

    // Not exactly a real animation system, just advance on every frame for now.
    m_rotation += 10.0f;

    deviceFuncs->vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    deviceFuncs->vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                               &m_descSet[currentFrame()], 0, nullptr);
    VkDeviceSize vbOffset = 0;
    deviceFuncs->vkCmdBindVertexBuffers(cb, 0, 1, &m_buf, &vbOffset);

    VkViewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = sz.width();
    viewport.height = sz.height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    deviceFuncs->vkCmdSetViewport(cb, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    deviceFuncs->vkCmdSetScissor(cb, 0, 1, &scissor);

    deviceFuncs->vkCmdDraw(cb, 3, 1, 0, 0);

    deviceFuncs->vkCmdEndRenderPass(cmdBuf);

    frameReady();
}

void VulkanViewportWindow::initResources()
{
    // Prepare the vertex and uniform data. The vertex data will never
    // change so one buffer is sufficient regardless of the value of
    // CONCURRENT_FRAME_COUNT. Uniform data is changing per
    // frame however so active frames have to have a dedicated copy.

    // Use just one memory allocation and one buffer. We will then specify the
    // appropriate offsets for uniform buffers in the VkDescriptorBufferInfo.
    // Have to watch out for
    // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment, though.

    // The uniform buffer is not strictly required in this example, we could
    // have used push constants as well since our single matrix (64 bytes) fits
    // into the spec mandated minimum limit of 128 bytes. However, once that
    // limit is not sufficient, the per-frame buffers, as shown below, will
    // become necessary.

    const int concurrentFrameCount = this->concurrentFrameCount();
    const VkPhysicalDeviceLimits* pdevLimits = &_device->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    qDebug("uniform buffer offset alignment is %u", (uint) uniAlign);
    VkBufferCreateInfo bufInfo;
    memset(&bufInfo, 0, sizeof(bufInfo));
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // Our internal layout is vertex, uniform, uniform, ... with each uniform buffer start offset aligned to uniAlign.
    const VkDeviceSize vertexAllocSize = aligned(sizeof(vertexData), uniAlign);
    const VkDeviceSize uniformAllocSize = aligned(UNIFORM_DATA_SIZE, uniAlign);
    bufInfo.size = vertexAllocSize + concurrentFrameCount * uniformAllocSize;
    bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkResult err = deviceFunctions()->vkCreateBuffer(logicalDevice(), &bufInfo, nullptr, &m_buf);
    if(err != VK_SUCCESS)
        qFatal("Failed to create buffer: %d", err);

    VkMemoryRequirements memReq;
    deviceFunctions()->vkGetBufferMemoryRequirements(logicalDevice(), m_buf, &memReq);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        hostVisibleMemoryIndex()
    };

    err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memAllocInfo, nullptr, &m_bufMem);
    if(err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", err);

    err = deviceFunctions()->vkBindBufferMemory(logicalDevice(), m_buf, m_bufMem, 0);
    if(err != VK_SUCCESS)
        qFatal("Failed to bind buffer memory: %d", err);

    quint8 *p;
    err = deviceFunctions()->vkMapMemory(logicalDevice(), m_bufMem, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
    if(err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    memcpy(p, vertexData, sizeof(vertexData));
    QMatrix4x4 ident;
    memset(m_uniformBufInfo, 0, sizeof(m_uniformBufInfo));
    for(int i = 0; i < concurrentFrameCount; ++i) {
        const VkDeviceSize offset = vertexAllocSize + i * uniformAllocSize;
        memcpy(p + offset, ident.constData(), 16 * sizeof(float));
        m_uniformBufInfo[i].buffer = m_buf;
        m_uniformBufInfo[i].offset = offset;
        m_uniformBufInfo[i].range = uniformAllocSize;
    }
    deviceFunctions()->vkUnmapMemory(logicalDevice(), m_bufMem);

    VkVertexInputBindingDescription vertexBindingDesc = {
        0, // binding
        5 * sizeof(float),
        VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32_SFLOAT,
            0
        },
        { // color
            1,
            0,
            VK_FORMAT_R32G32B32_SFLOAT,
            2 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttrDesc;

    // Set up descriptor set and its layout.
    VkDescriptorPoolSize descPoolSizes = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uint32_t(concurrentFrameCount) };
    VkDescriptorPoolCreateInfo descPoolInfo;
    memset(&descPoolInfo, 0, sizeof(descPoolInfo));
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.maxSets = concurrentFrameCount;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &descPoolSizes;
    err = deviceFunctions()->vkCreateDescriptorPool(logicalDevice(), &descPoolInfo, nullptr, &m_descPool);
    if(err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", err);

    VkDescriptorSetLayoutBinding layoutBinding = {
        0, // binding
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT,
        nullptr
    };
    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &layoutBinding
    };
    err = deviceFunctions()->vkCreateDescriptorSetLayout(logicalDevice(), &descLayoutInfo, nullptr, &m_descSetLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", err);

    for (int i = 0; i < concurrentFrameCount; ++i) {
        VkDescriptorSetAllocateInfo descSetAllocInfo = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            nullptr,
            m_descPool,
            1,
            &m_descSetLayout
        };
        err = deviceFunctions()->vkAllocateDescriptorSets(logicalDevice(), &descSetAllocInfo, &m_descSet[i]);
        if(err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", err);

        VkWriteDescriptorSet descWrite;
        memset(&descWrite, 0, sizeof(descWrite));
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = m_descSet[i];
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.pBufferInfo = &m_uniformBufInfo[i];
        deviceFunctions()->vkUpdateDescriptorSets(logicalDevice(), 1, &descWrite, 0, nullptr);
    }

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = deviceFunctions()->vkCreatePipelineCache(logicalDevice(), &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descSetLayout;
    err = deviceFunctions()->vkCreatePipelineLayout(logicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    // Shaders
    VkShaderModule vertShaderModule = _device->createShader(QStringLiteral(":/vulkanrenderer/color.vert.spv"));
    VkShaderModule fragShaderModule = _device->createShader(QStringLiteral(":/vulkanrenderer/color.frag.spv"));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderModule,
            "main",
            nullptr
        }
    };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia;
    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp;
    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs;
    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE; // we want the back face as well
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms;
    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enable multisampling.
    ms.rasterizationSamples = sampleCountFlagBits();
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds;
    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipelineInfo.pDepthStencilState = &ds;

    VkPipelineColorBlendStateCreateInfo cb;
    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // no blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att;
    memset(&att, 0, sizeof(att));
    att.colorWriteMask = 0xF;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn;
    memset(&dyn, 0, sizeof(dyn));
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dyn.pDynamicStates = dynEnable;
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = defaultRenderPass();

    err = deviceFunctions()->vkCreateGraphicsPipelines(logicalDevice(), m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if(err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    if(vertShaderModule)
        deviceFunctions()->vkDestroyShaderModule(logicalDevice(), vertShaderModule, nullptr);
    if(fragShaderModule)
        deviceFunctions()->vkDestroyShaderModule(logicalDevice(), fragShaderModule, nullptr);
}

void VulkanViewportWindow::initSwapChainResources()
{
    // Projection matrix
    //m_proj = _window->clipCorrectionMatrix(); // adjust for Vulkan-OpenGL clip space differences
    //const QSize sz = _window->swapChainImageSize();
    //m_proj.perspective(45.0f, sz.width() / (float) sz.height(), 0.01f, 100.0f);
    //m_proj.translate(0, 0, -4);
}

void VulkanViewportWindow::releaseResources()
{
    if(m_pipeline) {
        deviceFunctions()->vkDestroyPipeline(logicalDevice(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if(m_pipelineLayout) {
        deviceFunctions()->vkDestroyPipelineLayout(logicalDevice(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if(m_pipelineCache) {
        deviceFunctions()->vkDestroyPipelineCache(logicalDevice(), m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descSetLayout) {
        deviceFunctions()->vkDestroyDescriptorSetLayout(logicalDevice(), m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if (m_descPool) {
        deviceFunctions()->vkDestroyDescriptorPool(logicalDevice(), m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    if (m_buf) {
        deviceFunctions()->vkDestroyBuffer(logicalDevice(), m_buf, nullptr);
        m_buf = VK_NULL_HANDLE;
    }

    if (m_bufMem) {
        deviceFunctions()->vkFreeMemory(logicalDevice(), m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }
}

}	// End of namespace
