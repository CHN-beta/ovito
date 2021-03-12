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
    // Make this a Vulkan compatible window.
    setSurfaceType(QSurface::VulkanSurface);

	// Create the QVulkanWindow.
	setVulkanInstance(&VulkanSceneRenderer::vkInstance());

	// Embed the QVulkanWindow in a QWidget container.
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
        if (static_cast<QPlatformSurfaceEvent*>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
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
    if(this->status == StatusFailRetry)
        this->status = StatusUninitialized;
    if(this->status == StatusUninitialized) {
        init();
        if(this->status == StatusDeviceReady)
            recreateSwapChain();
    }
    if(this->status == StatusReady)
        requestUpdate();
}

/******************************************************************************
* Returns the list of properties for the supported physical devices in the system.
* This function can be called before making the window visible.
******************************************************************************/
QVector<VkPhysicalDeviceProperties> VulkanViewportWindow::availablePhysicalDevices()
{
    if(!this->physDevs.isEmpty() && !this->physDevProps.isEmpty())
        return this->physDevProps;
    QVulkanInstance* inst = vulkanInstance();
    OVITO_ASSERT(inst);
    QVulkanFunctions* f = inst->functions();
    uint32_t count = 1;
    VkResult err = f->vkEnumeratePhysicalDevices(inst->vkInstance(), &count, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("VulkanViewportWindow: Failed to get physical device count: %d", err);
        return this->physDevProps;
    }
    qCDebug(lcGuiVk, "%d physical devices", count);
    if (!count)
        return this->physDevProps;
    QVector<VkPhysicalDevice> devs(count);
    err = f->vkEnumeratePhysicalDevices(inst->vkInstance(), &count, devs.data());
    if(err != VK_SUCCESS) {
        qWarning("VulkanViewportWindow: Failed to enumerate physical devices: %d", err);
        return physDevProps;
    }
    this->physDevs = devs;
    this->physDevProps.resize(count);
    for(uint32_t i = 0; i < count; ++i) {
        VkPhysicalDeviceProperties *p = &this->physDevProps[i];
        f->vkGetPhysicalDeviceProperties(this->physDevs.at(i), p);
        qCDebug(lcGuiVk, "Physical device [%d]: name '%s' version %d.%d.%d", i, p->deviceName,
                VK_VERSION_MAJOR(p->driverVersion), VK_VERSION_MINOR(p->driverVersion),
                VK_VERSION_PATCH(p->driverVersion));
    }
    return this->physDevProps;
}

/******************************************************************************
* Requests the usage of the physical device with index \a idx. The index
* corresponds to the list returned from availablePhysicalDevices().
* By default the first physical device is used.
* 
* This function must be called before the window is made visible or at
* latest from preInitResources(), and has no effect if called afterwards.
******************************************************************************/
void VulkanViewportWindow::setPhysicalDeviceIndex(int idx)
{
    if(this->status != StatusUninitialized) {
        qWarning("VulkanViewportWindow: Attempted to set physical device when already initialized");
        return;
    }
    const int count = availablePhysicalDevices().count();
    if(idx < 0 || idx >= count) {
        qWarning("VulkanViewportWindow: Invalid physical device index %d (total physical devices: %d)", idx, count);
        return;
    }
    this->physDevIndex = idx;
}

/******************************************************************************
* Returns the list of the extensions that are supported by logical devices
* created from the physical device selected by setPhysicalDeviceIndex().
*
* This function can be called before making the window visible.
******************************************************************************/
QVulkanInfoVector<QVulkanExtension> VulkanViewportWindow::supportedDeviceExtensions()
{
    availablePhysicalDevices();
    if(this->physDevs.isEmpty()) {
        qWarning("VulkanViewportWindow: No physical devices found");
        return QVulkanInfoVector<QVulkanExtension>();
    }
    VkPhysicalDevice physDev = this->physDevs.at(this->physDevIndex);
    if(this->supportedDevExtensions.contains(physDev))
        return this->supportedDevExtensions.value(physDev);
    QVulkanFunctions* f = vulkanInstance()->functions();
    uint32_t count = 0;
    VkResult err = f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &count, nullptr);
    if(err == VK_SUCCESS) {
        QVector<VkExtensionProperties> extProps(count);
        err = f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &count, extProps.data());
        if(err == VK_SUCCESS) {
            QVulkanInfoVector<QVulkanExtension> exts;
            for(const VkExtensionProperties &prop : extProps) {
                QVulkanExtension ext;
                ext.name = prop.extensionName;
                ext.version = prop.specVersion;
                exts.append(ext);
            }
            this->supportedDevExtensions.insert(physDev, exts);
            qDebug(lcGuiVk) << "Supported device extensions:" << exts;
            return exts;
        }
    }
    qWarning("VulkanViewportWindow: Failed to query device extension count: %d", err);
    return {};
}

/******************************************************************************
* Sets the list of device \a extensions to be enabled. Unsupported extensions 
* are ignored. The swapchain extension will always be added automatically, 
* no need to include it in this list.
*
* This function must be called before the window is made visible or at
* latest in preInitResources(), and has no effect if called afterwards.
******************************************************************************/
void VulkanViewportWindow::setDeviceExtensions(const QByteArrayList& extensions)
{
    if(this->status != StatusUninitialized) {
        qWarning("VulkanViewportWindow: Attempted to set device extensions when already initialized");
        return;
    }
    this->requestedDevExtensions = extensions;
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
* This function must be called before the window is made visible or at
* latest in preInitResources(), and has no effect if called afterwards.
* 
* Reimplementing preInitResources() allows dynamically examining the list of 
* supported formats, should that be desired. There the surface is retrievable via
* QVulkanInstace::surfaceForWindow(), while this function can still safely be
* called to affect the later stages of initialization.
******************************************************************************/
void VulkanViewportWindow::setPreferredColorFormats(const QVector<VkFormat>& formats)
{
    if(this->status != StatusUninitialized) {
        qWarning("VulkanViewportWindow: Attempted to set preferred color format when already initialized");
        return;
    }
    this->requestedColorFormats = formats;
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
    Q_ASSERT(this->status == StatusUninitialized);
    qCDebug(lcGuiVk, "QVulkanWindow init");
    this->inst = vulkanInstance();
    if(!this->inst) {
        qWarning("QVulkanWindow: Attempted to initialize without a QVulkanInstance");
        // This is a simple user error, recheck on the next expose instead of
        // going into the permanent failure state.
        status = StatusFailRetry;
        return;
    }
    this->surface = QVulkanInstance::surfaceForWindow(this);
    if(this->surface == VK_NULL_HANDLE) {
        qWarning("QVulkanWindow: Failed to retrieve Vulkan surface for window");
        this->status = StatusFailRetry;
        return;
    }
    availablePhysicalDevices();
    if(this->physDevs.isEmpty()) {
        qWarning("QVulkanWindow: No physical devices found");
        this->status = StatusFail;
        return;
    }
    if(this->physDevIndex < 0 || this->physDevIndex >= this->physDevs.count()) {
        qWarning("QVulkanWindow: Invalid physical device index; defaulting to 0");
        this->physDevIndex = 0;
    }
    qCDebug(lcGuiVk, "Using physical device [%d]", this->physDevIndex);
    // Give a last chance to do decisions based on the physical device and the surface.
    preInitResources();
    VkPhysicalDevice physDev = this->physDevs.at(this->physDevIndex);
    QVulkanFunctions* f = this->inst->functions();
    uint32_t queueCount = 0;
    f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, nullptr);
    QVector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
    f->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, queueFamilyProps.data());
    this->gfxQueueFamilyIdx = uint32_t(-1);
    this->presQueueFamilyIdx = uint32_t(-1);
    for(int i = 0; i < queueFamilyProps.count(); ++i) {
        const bool supportsPresent = this->inst->supportsPresent(physDev, i, this);
        qCDebug(lcGuiVk, "queue family %d: flags=0x%x count=%d supportsPresent=%d", i,
                queueFamilyProps[i].queueFlags, queueFamilyProps[i].queueCount, supportsPresent);
        if(this->gfxQueueFamilyIdx == uint32_t(-1)
                && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                && supportsPresent)
            this->gfxQueueFamilyIdx = i;
    }
    if(this->gfxQueueFamilyIdx != uint32_t(-1))
        presQueueFamilyIdx = this->gfxQueueFamilyIdx;
    else {
        qCDebug(lcGuiVk, "No queue with graphics+present; trying separate queues");
        for(int i = 0; i < queueFamilyProps.count(); ++i) {
            if(this->gfxQueueFamilyIdx == uint32_t(-1) && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                this->gfxQueueFamilyIdx = i;
            if(this->presQueueFamilyIdx == uint32_t(-1) && this->inst->supportsPresent(physDev, i, this))
                this->presQueueFamilyIdx = i;
        }
    }
    if(this->gfxQueueFamilyIdx == uint32_t(-1)) {
        qWarning("QVulkanWindow: No graphics queue family found");
        this->status = StatusFail;
        return;
    }
    if(this->presQueueFamilyIdx == uint32_t(-1)) {
        qWarning("QVulkanWindow: No present queue family found");
        this->status = StatusFail;
        return;
    }
#ifdef OVITO_DEBUG
    // Allow testing the separate present queue case in debug builds on AMD cards
    if(qEnvironmentVariableIsSet("QT_VK_PRESENT_QUEUE_INDEX"))
        this->presQueueFamilyIdx = qEnvironmentVariableIntValue("QT_VK_PRESENT_QUEUE_INDEX");
#endif
    qCDebug(lcGuiVk, "Using queue families: graphics = %u present = %u", this->gfxQueueFamilyIdx, this->presQueueFamilyIdx);
    VkDeviceQueueCreateInfo queueInfo[2];
    const float prio[] = { 0 };
    memset(queueInfo, 0, sizeof(queueInfo));
    queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[0].queueFamilyIndex = this->gfxQueueFamilyIdx;
    queueInfo[0].queueCount = 1;
    queueInfo[0].pQueuePriorities = prio;
    if(this->gfxQueueFamilyIdx != this->presQueueFamilyIdx) {
        queueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo[1].queueFamilyIndex = this->presQueueFamilyIdx;
        queueInfo[1].queueCount = 1;
        queueInfo[1].pQueuePriorities = prio;
    }
    // Filter out unsupported extensions in order to keep symmetry
    // with how QVulkanInstance behaves. Add the swapchain extension.
    QVector<const char*> devExts;
    QVulkanInfoVector<QVulkanExtension> supportedExtensions = supportedDeviceExtensions();
    QByteArrayList reqExts = this->requestedDevExtensions;
    reqExts.append("VK_KHR_swapchain");
    for(const QByteArray& ext : reqExts) {
        if(supportedExtensions.contains(ext))
            devExts.append(ext.constData());
    }
    qCDebug(lcGuiVk) << "Enabling device extensions:" << devExts;
    VkDeviceCreateInfo devInfo;
    memset(&devInfo, 0, sizeof(devInfo));
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = (this->gfxQueueFamilyIdx == this->presQueueFamilyIdx) ? 1 : 2;
    devInfo.pQueueCreateInfos = queueInfo;
    devInfo.enabledExtensionCount = devExts.count();
    devInfo.ppEnabledExtensionNames = devExts.constData();
    // Device layers are not supported by this implementation since that's an already deprecated
    // API. However, have a workaround for systems with older API and layers (f.ex. L4T
    // 24.2 for the Jetson TX1 provides API 1.0.13 and crashes when the validation layer
    // is enabled for the instance but not the device).
    uint32_t apiVersion = physDevProps[physDevIndex].apiVersion;
    if(VK_VERSION_MAJOR(apiVersion) == 1 && VK_VERSION_MINOR(apiVersion) == 0 && VK_VERSION_PATCH(apiVersion) <= 13) {
        // Make standard validation work at least.
        const QByteArray stdValName = QByteArrayLiteral("VK_LAYER_LUNARG_standard_validation");
        const char* stdValNamePtr = stdValName.constData();
        if(this->inst->layers().contains(stdValName)) {
            uint32_t count = 0;
            VkResult err = f->vkEnumerateDeviceLayerProperties(physDev, &count, nullptr);
            if(err == VK_SUCCESS) {
                QVector<VkLayerProperties> layerProps(count);
                err = f->vkEnumerateDeviceLayerProperties(physDev, &count, layerProps.data());
                if(err == VK_SUCCESS) {
                    for(const VkLayerProperties &prop : layerProps) {
                        if(!strncmp(prop.layerName, stdValNamePtr, stdValName.count())) {
                            devInfo.enabledLayerCount = 1;
                            devInfo.ppEnabledLayerNames = &stdValNamePtr;
                            break;
                        }
                    }
                }
            }
        }
    }
    VkResult err = f->vkCreateDevice(physDev, &devInfo, nullptr, &dev);
    if(err == VK_ERROR_DEVICE_LOST) {
        qWarning("QVulkanWindow: Physical device lost");
        physicalDeviceLost();
        // Clear the caches so the list of physical devices is re-queried
        this->physDevs.clear();
        this->physDevProps.clear();
        this->status = StatusUninitialized;
        qCDebug(lcGuiVk, "Attempting to restart in 2 seconds");
        QTimer::singleShot(2000, this, &VulkanViewportWindow::ensureStarted);
        return;
    }
    if(err != VK_SUCCESS) {
        qWarning("QVulkanWindow: Failed to create device: %d", err);
        this->status = StatusFail;
        return;
    }

    this->devFuncs = this->inst->deviceFunctions(dev);
    OVITO_ASSERT(this->devFuncs);
    this->devFuncs->vkGetDeviceQueue(dev, this->gfxQueueFamilyIdx, 0, &this->gfxQueue);
    if(this->gfxQueueFamilyIdx == this->presQueueFamilyIdx)
        this->presQueue = this->gfxQueue;
    else
        this->devFuncs->vkGetDeviceQueue(dev, this->presQueueFamilyIdx, 0, &this->presQueue);
    VkCommandPoolCreateInfo poolInfo;
    memset(&poolInfo, 0, sizeof(poolInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = this->gfxQueueFamilyIdx;
    err = this->devFuncs->vkCreateCommandPool(dev, &poolInfo, nullptr, &cmdPool);
    if(err != VK_SUCCESS) {
        qWarning("QVulkanWindow: Failed to create command pool: %d", err);
        this->status = StatusFail;
        return;
    }
    if(this->gfxQueueFamilyIdx != this->presQueueFamilyIdx) {
        poolInfo.queueFamilyIndex = this->presQueueFamilyIdx;
        err = devFuncs->vkCreateCommandPool(dev, &poolInfo, nullptr, &presCmdPool);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to create command pool for present queue: %d", err);
            this->status = StatusFail;
            return;
        }
    }
    this->hostVisibleMemIndex = 0;
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    bool hostVisibleMemIndexSet = false;
    f->vkGetPhysicalDeviceMemoryProperties(physDev, &physDevMemProps);
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        const VkMemoryType* memType = physDevMemProps.memoryTypes;
        qCDebug(lcGuiVk, "memtype %d: flags=0x%x", i, memType[i].propertyFlags);
        // Find a host visible, host coherent memtype. If there is one that is
        // cached as well (in addition to being coherent), prefer that.
        const int hostVisibleAndCoherent = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if((memType[i].propertyFlags & hostVisibleAndCoherent) == hostVisibleAndCoherent) {
            if(!hostVisibleMemIndexSet || (memType[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
                hostVisibleMemIndexSet = true;
                this->hostVisibleMemIndex = i;
            }
        }
    }
    qCDebug(lcGuiVk, "Picked memtype %d for host visible memory", this->hostVisibleMemIndex);
    this->deviceLocalMemIndex = 0;
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        const VkMemoryType* memType = physDevMemProps.memoryTypes;
        // Just pick the first device local memtype.
        if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            this->deviceLocalMemIndex = i;
            break;
        }
    }
    qCDebug(lcGuiVk, "Picked memtype %d for device local memory", this->deviceLocalMemIndex);
    if(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR || !vkGetPhysicalDeviceSurfaceFormatsKHR) {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(this->inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(this->inst->getInstanceProcAddr("vkGetPhysicalDeviceSurfaceFormatsKHR"));
        if(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR || !vkGetPhysicalDeviceSurfaceFormatsKHR) {
            qWarning("QVulkanWindow: Physical device surface queries not available");
            this->status = StatusFail;
            return;
        }
    }
    // Figure out the color format here. Must not wait until recreateSwapChain()
    // because the renderpass should be available already from initResources (so that apps do not have to defer pipeline creation to initSwapChainResources), 
    // but the renderpass needs the final color format.
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, nullptr);
    QVector<VkSurfaceFormatKHR> formats(formatCount);
    if(formatCount)
        vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, formats.data());
    _colorFormat = VK_FORMAT_B8G8R8A8_UNORM; // our documented default if all else fails
    this->colorSpace = VkColorSpaceKHR(0); // this is in fact VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    // Pick the preferred format, if there is one.
    if(!formats.isEmpty() && formats[0].format != VK_FORMAT_UNDEFINED) {
        _colorFormat = formats[0].format;
        this->colorSpace = formats[0].colorSpace;
    }
    // Try to honor the user request.
    if(!formats.isEmpty() && !requestedColorFormats.isEmpty()) {
        for(VkFormat reqFmt : qAsConst(requestedColorFormats)) {
            auto r = std::find_if(formats.cbegin(), formats.cend(), [reqFmt](const VkSurfaceFormatKHR &sfmt) { return sfmt.format == reqFmt; });
            if(r != formats.cend()) {
                _colorFormat = r->format;
                this->colorSpace = r->colorSpace;
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
        this->dsFormat = dsFormatCandidates[dsFormatIdx];
        VkFormatProperties fmtProp;
        f->vkGetPhysicalDeviceFormatProperties(physDev, this->dsFormat, &fmtProp);
        if (fmtProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            break;
        ++dsFormatIdx;
    }
    if(dsFormatIdx == dsFormatCandidateCount)
        qWarning("QVulkanWindow: Failed to find an optimal depth-stencil format");
    qCDebug(lcGuiVk, "Color format: %d Depth-stencil format: %d", _colorFormat, this->dsFormat);
    if(!createDefaultRenderPass())
        return;
    initResources();
    this->status = StatusDeviceReady;
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
    attDesc[1].format = this->dsFormat;
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
    VkResult err = devFuncs->vkCreateRenderPass(dev, &rpInfo, nullptr, &_defaultRenderPass);
    if(err != VK_SUCCESS) {
        qWarning("QVulkanWindow: Failed to create renderpass: %d", err);
        return false;
    }
    return true;
}

/******************************************************************************
* Recreates the Vulkan swapchain.  
******************************************************************************/
void VulkanViewportWindow::recreateSwapChain()
{
    OVITO_ASSERT(this->status >= StatusDeviceReady);
    _swapChainImageSize = size() * devicePixelRatio(); // note: may change below due to surfaceCaps
    if(_swapChainImageSize.isEmpty()) // handle null window size gracefully
        return;
    QVulkanInstance* inst = vulkanInstance();
    QVulkanFunctions* f = inst->functions();
    this->devFuncs->vkDeviceWaitIdle(dev);
    if(!vkCreateSwapchainKHR) {
        vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(f->vkGetDeviceProcAddr(dev, "vkCreateSwapchainKHR"));
        vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(f->vkGetDeviceProcAddr(dev, "vkDestroySwapchainKHR"));
        vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(f->vkGetDeviceProcAddr(dev, "vkGetSwapchainImagesKHR"));
        vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(f->vkGetDeviceProcAddr(dev, "vkAcquireNextImageKHR"));
        vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(f->vkGetDeviceProcAddr(dev, "vkQueuePresentKHR"));
    }
    VkPhysicalDevice physDev = this->physDevs.at(physDevIndex);
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &surfaceCaps);
    uint32_t reqBufferCount = swapChainBufferCount;
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
    VkSwapchainKHR oldSwapChain = swapChain;
    VkSwapchainCreateInfoKHR swapChainInfo;
    memset(&swapChainInfo, 0, sizeof(swapChainInfo));
    swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainInfo.surface = surface;
    swapChainInfo.minImageCount = reqBufferCount;
    swapChainInfo.imageFormat = _colorFormat;
    swapChainInfo.imageColorSpace = colorSpace;
    swapChainInfo.imageExtent = bufferSize;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = usage;
    swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainInfo.preTransform = preTransform;
    swapChainInfo.compositeAlpha = compositeAlpha;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = true;
    swapChainInfo.oldSwapchain = oldSwapChain;
    qCDebug(lcGuiVk, "Creating new swap chain of %d buffers, size %dx%d", reqBufferCount, bufferSize.width, bufferSize.height);
    VkSwapchainKHR newSwapChain;
    VkResult err = vkCreateSwapchainKHR(dev, &swapChainInfo, nullptr, &newSwapChain);
    if(err != VK_SUCCESS) {
        qWarning("QVulkanWindow: Failed to create swap chain: %d", err);
        return;
    }
    if(oldSwapChain)
        releaseSwapChain();
    swapChain = newSwapChain;
    uint32_t actualSwapChainBufferCount = 0;
    err = vkGetSwapchainImagesKHR(dev, swapChain, &actualSwapChainBufferCount, nullptr);
    if(err != VK_SUCCESS || actualSwapChainBufferCount < 2) {
        qWarning("QVulkanWindow: Failed to get swapchain images: %d (count=%d)", err, actualSwapChainBufferCount);
        return;
    }
    qCDebug(lcGuiVk, "Actual swap chain buffer count: %d", actualSwapChainBufferCount);
    if(actualSwapChainBufferCount > MAX_SWAPCHAIN_BUFFER_COUNT) {
        qWarning("QVulkanWindow: Too many swapchain buffers (%d)", actualSwapChainBufferCount);
        return;
    }
    swapChainBufferCount = actualSwapChainBufferCount;
    VkImage swapChainImages[MAX_SWAPCHAIN_BUFFER_COUNT];
    err = vkGetSwapchainImagesKHR(dev, swapChain, &actualSwapChainBufferCount, swapChainImages);
    if(err != VK_SUCCESS) {
        qWarning("QVulkanWindow: Failed to get swapchain images: %d", err);
        return;
    }
    if(!createTransientImage(dsFormat,
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                              VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                              &dsImage,
                              &dsMem,
                              &dsView,
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
                                  &msaaImageMem,
                                  msaaViews,
                                  swapChainBufferCount))
        {
            return;
        }
    }
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    for(int i = 0; i < swapChainBufferCount; ++i) {
        ImageResources &image(imageRes[i]);
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
        err = devFuncs->vkCreateImageView(dev, &imgViewInfo, nullptr, &image.imageView);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to create swapchain image view %d: %d", i, err);
            return;
        }
        err = devFuncs->vkCreateFence(dev, &fenceInfo, nullptr, &image.cmdFence);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to create command buffer fence: %d", err);
            return;
        }
        image.cmdFenceWaitable = true; // fence was created in signaled state
        VkImageView views[3] = { image.imageView,
                                 dsView,
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
        VkResult err = devFuncs->vkCreateFramebuffer(dev, &fbInfo, nullptr, &image.fb);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to create framebuffer: %d", err);
            return;
        }
        if(gfxQueueFamilyIdx != presQueueFamilyIdx) {
            // pre-build the static image-acquire-on-present-queue command buffer
            VkCommandBufferAllocateInfo cmdBufInfo = {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, presCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
            err = devFuncs->vkAllocateCommandBuffers(dev, &cmdBufInfo, &image.presTransCmdBuf);
            if(err != VK_SUCCESS) {
                qWarning("QVulkanWindow: Failed to allocate acquire-on-present-queue command buffer: %d", err);
                return;
            }
            VkCommandBufferBeginInfo cmdBufBeginInfo = {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
                VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr };
            err = devFuncs->vkBeginCommandBuffer(image.presTransCmdBuf, &cmdBufBeginInfo);
            if(err != VK_SUCCESS) {
                qWarning("QVulkanWindow: Failed to begin acquire-on-present-queue command buffer: %d", err);
                return;
            }
            VkImageMemoryBarrier presTrans;
            memset(&presTrans, 0, sizeof(presTrans));
            presTrans.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            presTrans.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            presTrans.oldLayout = presTrans.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            presTrans.srcQueueFamilyIndex = gfxQueueFamilyIdx;
            presTrans.dstQueueFamilyIndex = presQueueFamilyIdx;
            presTrans.image = image.image;
            presTrans.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            presTrans.subresourceRange.levelCount = presTrans.subresourceRange.layerCount = 1;
            devFuncs->vkCmdPipelineBarrier(image.presTransCmdBuf,
                                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                           0, 0, nullptr, 0, nullptr,
                                           1, &presTrans);
            err = devFuncs->vkEndCommandBuffer(image.presTransCmdBuf);
            if(err != VK_SUCCESS) {
                qWarning("QVulkanWindow: Failed to end acquire-on-present-queue command buffer: %d", err);
                return;
            }
        }
    }
    this->currentImage = 0;
    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    for(int i = 0; i < _frameLag; ++i) {
        FrameResources &frame(frameRes[i]);
        frame.imageAcquired = false;
        frame.imageSemWaitable = false;
        devFuncs->vkCreateFence(dev, &fenceInfo, nullptr, &frame.fence);
        frame.fenceWaitable = true; // fence was created in signaled state
        devFuncs->vkCreateSemaphore(dev, &semInfo, nullptr, &frame.imageSem);
        devFuncs->vkCreateSemaphore(dev, &semInfo, nullptr, &frame.drawSem);
        if(this->gfxQueueFamilyIdx != this->presQueueFamilyIdx)
            devFuncs->vkCreateSemaphore(dev, &semInfo, nullptr, &frame.presTransSem);
    }
    _currentFrame = 0;
    initSwapChainResources();
    this->status = StatusReady;
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
        err = this->devFuncs->vkCreateImage(this->dev, &imgInfo, nullptr, images + i);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to create image: %d", err);
            return false;
        }
        // Assume the reqs are the same since the images are same in every way.
        // Still, call GetImageMemReq for every image, in order to prevent the
        // validation layer from complaining.
        this->devFuncs->vkGetImageMemoryRequirements(this->dev, images[i], &memReq);
    }
    VkMemoryAllocateInfo memInfo;
    memset(&memInfo, 0, sizeof(memInfo));
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.allocationSize = aligned(memReq.size, memReq.alignment) * count;
    uint32_t startIndex = 0;
    do {
        memInfo.memoryTypeIndex = chooseTransientImageMemType(images[0], startIndex);
        if(memInfo.memoryTypeIndex == uint32_t(-1)) {
            qWarning("QVulkanWindow: No suitable memory type found");
            return false;
        }
        startIndex = memInfo.memoryTypeIndex + 1;
        qCDebug(lcGuiVk, "Allocating %u bytes for transient image (memtype %u)",
                uint32_t(memInfo.allocationSize), memInfo.memoryTypeIndex);
        err = this->devFuncs->vkAllocateMemory(this->dev, &memInfo, nullptr, mem);
        if(err != VK_SUCCESS && err != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            qWarning("QVulkanWindow: Failed to allocate image memory: %d", err);
            return false;
        }
    } 
    while(err != VK_SUCCESS);
    VkDeviceSize ofs = 0;
    for(int i = 0; i < count; ++i) {
        err = this->devFuncs->vkBindImageMemory(this->dev, images[i], *mem, ofs);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to bind image memory: %d", err);
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
        err = this->devFuncs->vkCreateImageView(this->dev, &imgViewInfo, nullptr, views + i);
        if(err != VK_SUCCESS) {
            qWarning("QVulkanWindow: Failed to create image view: %d", err);
            return false;
        }
    }
    return true;
}

/******************************************************************************
* Picks the right memory type for a Vulkan image.
******************************************************************************/
uint32_t VulkanViewportWindow::chooseTransientImageMemType(VkImage img, uint32_t startIndex)
{
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    this->inst->functions()->vkGetPhysicalDeviceMemoryProperties(this->physDevs[physDevIndex], &physDevMemProps);
    VkMemoryRequirements memReq;
    this->devFuncs->vkGetImageMemoryRequirements(this->dev, img, &memReq);
    uint32_t memTypeIndex = uint32_t(-1);
    if(memReq.memoryTypeBits) {
        // Find a device local + lazily allocated, or at least device local memtype.
        const VkMemoryType *memType = physDevMemProps.memoryTypes;
        bool foundDevLocal = false;
        for(uint32_t i = startIndex; i < physDevMemProps.memoryTypeCount; ++i) {
            if(memReq.memoryTypeBits & (1 << i)) {
                if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    if(!foundDevLocal) {
                        foundDevLocal = true;
                        memTypeIndex = i;
                    }
                    if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                        memTypeIndex = i;
                        break;
                    }
                }
            }
        }
    }
    return memTypeIndex;
}

/******************************************************************************
* Releases the resources of the Vulkan swapchain.  
******************************************************************************/
void VulkanViewportWindow::releaseSwapChain()
{
    if(!this->dev || !this->swapChain) // do not rely on 'status', a half done init must be cleaned properly too
        return;
    qCDebug(lcGuiVk, "Releasing swapchain");
   this-> devFuncs->vkDeviceWaitIdle(this->dev);
    releaseSwapChainResources();
    this->devFuncs->vkDeviceWaitIdle(this->dev);
    for(int i = 0; i < _frameLag; ++i) {
        FrameResources& frame = this->frameRes[i];
        if(frame.fence) {
            if(frame.fenceWaitable)
                this->devFuncs->vkWaitForFences(this->dev, 1, &frame.fence, VK_TRUE, UINT64_MAX);
            this->devFuncs->vkDestroyFence(this->dev, frame.fence, nullptr);
            frame.fence = VK_NULL_HANDLE;
            frame.fenceWaitable = false;
        }
        if(frame.imageSem) {
            this->devFuncs->vkDestroySemaphore(this->dev, frame.imageSem, nullptr);
            frame.imageSem = VK_NULL_HANDLE;
        }
        if(frame.drawSem) {
            this->devFuncs->vkDestroySemaphore(this->dev, frame.drawSem, nullptr);
            frame.drawSem = VK_NULL_HANDLE;
        }
        if(frame.presTransSem) {
            this->devFuncs->vkDestroySemaphore(this->dev, frame.presTransSem, nullptr);
            frame.presTransSem = VK_NULL_HANDLE;
        }
    }
    for(int i = 0; i < this->swapChainBufferCount; ++i) {
        ImageResources& image = this->imageRes[i];
        if(image.cmdFence) {
            if(image.cmdFenceWaitable)
                this->devFuncs->vkWaitForFences(this->dev, 1, &image.cmdFence, VK_TRUE, UINT64_MAX);
            this->devFuncs->vkDestroyFence(this->dev, image.cmdFence, nullptr);
            image.cmdFence = VK_NULL_HANDLE;
            image.cmdFenceWaitable = false;
        }
        if(image.fb) {
            this->devFuncs->vkDestroyFramebuffer(this->dev, image.fb, nullptr);
            image.fb = VK_NULL_HANDLE;
        }
        if(image.imageView) {
            this->devFuncs->vkDestroyImageView(this->dev, image.imageView, nullptr);
            image.imageView = VK_NULL_HANDLE;
        }
        if(image.cmdBuf) {
            this->devFuncs->vkFreeCommandBuffers(this->dev, cmdPool, 1, &image.cmdBuf);
            image.cmdBuf = VK_NULL_HANDLE;
        }
        if(image.presTransCmdBuf) {
            this->devFuncs->vkFreeCommandBuffers(this->dev, presCmdPool, 1, &image.presTransCmdBuf);
            image.presTransCmdBuf = VK_NULL_HANDLE;
        }
        if(image.msaaImageView) {
            this->devFuncs->vkDestroyImageView(this->dev, image.msaaImageView, nullptr);
            image.msaaImageView = VK_NULL_HANDLE;
        }
        if(image.msaaImage) {
            this->devFuncs->vkDestroyImage(this->dev, image.msaaImage, nullptr);
            image.msaaImage = VK_NULL_HANDLE;
        }
    }
    if(this->msaaImageMem) {
        this->devFuncs->vkFreeMemory(this->dev, this->msaaImageMem, nullptr);
        this->msaaImageMem = VK_NULL_HANDLE;
    }
    if(this->dsView) {
        this->devFuncs->vkDestroyImageView(this->dev, this->dsView, nullptr);
        this->dsView = VK_NULL_HANDLE;
    }
    if(this->dsImage) {
        this->devFuncs->vkDestroyImage(this->dev, this->dsImage, nullptr);
        this->dsImage = VK_NULL_HANDLE;
    }
    if(this->dsMem) {
        this->devFuncs->vkFreeMemory(this->dev, this->dsMem, nullptr);
        this->dsMem = VK_NULL_HANDLE;
    }
    if(this->swapChain) {
        vkDestroySwapchainKHR(this->dev, this->swapChain, nullptr);
        this->swapChain = VK_NULL_HANDLE;
    }
    if(this->status == StatusReady)
        this->status = StatusDeviceReady;
}

/******************************************************************************
* Handles a Vulkan device that was recently lost.
******************************************************************************/
bool VulkanViewportWindow::checkDeviceLost(VkResult err)
{
    if(err == VK_ERROR_DEVICE_LOST) {
        qWarning("QVulkanWindow: Device lost");
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
    if(!this->swapChain || this->framePending)
        return;

    // Handle window being resized.
    if(size() * devicePixelRatio() != _swapChainImageSize) {
        recreateSwapChain();
        if(!this->swapChain)
            return;
    }

    FrameResources& frame = this->frameRes[_currentFrame];
    if(!frame.imageAcquired) {
        // Wait if we are too far ahead, i.e. the thread gets throttled based on the presentation rate
        // (note that we are using FIFO mode -> vsync)
        if(frame.fenceWaitable) {
            this->devFuncs->vkWaitForFences(this->dev, 1, &frame.fence, VK_TRUE, UINT64_MAX);
            this->devFuncs->vkResetFences(this->dev, 1, &frame.fence);
            frame.fenceWaitable = false;
        }
        // move on to next swapchain image
        VkResult err = vkAcquireNextImageKHR(this->dev, swapChain, UINT64_MAX,
                                             frame.imageSem, frame.fence, &this->currentImage);
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
                qWarning("QVulkanWindow: Failed to acquire next swapchain image: %d", err);
            requestUpdate();
            return;
        }
    }
    // Make sure the previous draw for the same image has finished
    ImageResources& image = this->imageRes[this->currentImage];
    if(image.cmdFenceWaitable) {
        this->devFuncs->vkWaitForFences(this->dev, 1, &image.cmdFence, VK_TRUE, UINT64_MAX);
        this->devFuncs->vkResetFences(this->dev, 1, &image.cmdFence);
        image.cmdFenceWaitable = false;
    }
    // Build new draw command buffer
    if(image.cmdBuf) {
        this->devFuncs->vkFreeCommandBuffers(this->dev, this->cmdPool, 1, &image.cmdBuf);
        image.cmdBuf = 0;
    }
    VkCommandBufferAllocateInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, this->cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
    VkResult err = this->devFuncs->vkAllocateCommandBuffers(this->dev, &cmdBufInfo, &image.cmdBuf);
    if(err != VK_SUCCESS) {
        if(!checkDeviceLost(err))
            qWarning("QVulkanWindow: Failed to allocate frame command buffer: %d", err);
        return;
    }
    VkCommandBufferBeginInfo cmdBufBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
    err = devFuncs->vkBeginCommandBuffer(image.cmdBuf, &cmdBufBeginInfo);
    if(err != VK_SUCCESS) {
        if(!checkDeviceLost(err))
            qWarning("QVulkanWindow: Failed to begin frame command buffer: %d", err);
        return;
    }
    this->framePending = true;
    startNextFrame();
    // Done for now - endFrame() will get invoked when frameReady() is called back.
}

/******************************************************************************
* Finishes rendering a frame.
******************************************************************************/
void VulkanViewportWindow::endFrame()
{
    FrameResources& frame = this->frameRes[_currentFrame];
    ImageResources& image = this->imageRes[this->currentImage];
    if(this->gfxQueueFamilyIdx != this->presQueueFamilyIdx) {
        // Add the swapchain image release to the command buffer that will be
        // submitted to the graphics queue.
        VkImageMemoryBarrier presTrans;
        memset(&presTrans, 0, sizeof(presTrans));
        presTrans.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presTrans.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        presTrans.oldLayout = presTrans.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presTrans.srcQueueFamilyIndex = this->gfxQueueFamilyIdx;
        presTrans.dstQueueFamilyIndex = this->presQueueFamilyIdx;
        presTrans.image = image.image;
        presTrans.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presTrans.subresourceRange.levelCount = presTrans.subresourceRange.layerCount = 1;
        this->devFuncs->vkCmdPipelineBarrier(image.cmdBuf,
                                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                       0, 0, nullptr, 0, nullptr,
                                       1, &presTrans);
    }
    VkResult err = this->devFuncs->vkEndCommandBuffer(image.cmdBuf);
    if(err != VK_SUCCESS) {
        if(!checkDeviceLost(err))
            qWarning("QVulkanWindow: Failed to end frame command buffer: %d", err);
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
    err = this->devFuncs->vkQueueSubmit(this->gfxQueue, 1, &submitInfo, image.cmdFence);
    if(err == VK_SUCCESS) {
        frame.imageSemWaitable = false;
        image.cmdFenceWaitable = true;
    } 
    else {
        if(!checkDeviceLost(err))
            qWarning("QVulkanWindow: Failed to submit to graphics queue: %d", err);
        return;
    }
    if(this->gfxQueueFamilyIdx != this->presQueueFamilyIdx) {
        // Submit the swapchain image acquire to the present queue.
        submitInfo.pWaitSemaphores = &frame.drawSem;
        submitInfo.pSignalSemaphores = &frame.presTransSem;
        submitInfo.pCommandBuffers = &image.presTransCmdBuf; // must be USAGE_SIMULTANEOUS
        err = this->devFuncs->vkQueueSubmit(this->presQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if(err != VK_SUCCESS) {
            if(!checkDeviceLost(err))
                qWarning("QVulkanWindow: Failed to submit to present queue: %d", err);
            return;
        }
    }
    // Queue present
    VkPresentInfoKHR presInfo;
    memset(&presInfo, 0, sizeof(presInfo));
    presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presInfo.swapchainCount = 1;
    presInfo.pSwapchains = &this->swapChain;
    presInfo.pImageIndices = &this->currentImage;
    presInfo.waitSemaphoreCount = 1;
    presInfo.pWaitSemaphores = (this->gfxQueueFamilyIdx == this->presQueueFamilyIdx) ? &frame.drawSem : &frame.presTransSem;
    err = vkQueuePresentKHR(this->gfxQueue, &presInfo);
    if(err != VK_SUCCESS) {
        if(err == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            requestUpdate();
            return;
        } 
        else if(err != VK_SUBOPTIMAL_KHR) {
            if(!checkDeviceLost(err))
                qWarning("QVulkanWindow: Failed to present: %d", err);
            return;
        }
    }
    frame.imageAcquired = false;
    this->inst->presentQueued(this);
    _currentFrame = (_currentFrame + 1) % concurrentFrameCount();
}

/******************************************************************************
* Releases all Vulkan resources.
******************************************************************************/
void VulkanViewportWindow::reset()
{
    if(!this->dev) // Do not rely on 'status', a half done init must be cleaned properly too
        return;
    qCDebug(lcGuiVk, "QVulkanWindow reset");
    this->devFuncs->vkDeviceWaitIdle(this->dev);
    releaseResources();
    this->devFuncs->vkDeviceWaitIdle(this->dev);
    if(_defaultRenderPass) {
        this->devFuncs->vkDestroyRenderPass(this->dev, _defaultRenderPass, nullptr);
        _defaultRenderPass = VK_NULL_HANDLE;
    }
    if(this->cmdPool) {
        this->devFuncs->vkDestroyCommandPool(this->dev, this->cmdPool, nullptr);
        this->cmdPool = VK_NULL_HANDLE;
    }
    if(this->presCmdPool) {
        this->devFuncs->vkDestroyCommandPool(this->dev, this->presCmdPool, nullptr);
        this->presCmdPool = VK_NULL_HANDLE;
    }
    if(this->dev) {
        this->devFuncs->vkDestroyDevice(this->dev, nullptr);
        this->inst->resetDeviceFunctions(this->dev);
        this->dev = VK_NULL_HANDLE;
        vkCreateSwapchainKHR = nullptr; // re-resolve swapchain funcs later on since some come via the device
    }
    this->surface = VK_NULL_HANDLE;
    this->status = StatusUninitialized;
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
    if(!this->framePending) {
        qWarning("QVulkanWindow: frameReady() called without a corresponding startNextFrame()");
        return;
    }
    this->framePending = false;
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
    QVulkanDeviceFunctions* deviceFuncs = vulkanInstance()->deviceFunctions(this->dev);
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
    VkResult err = deviceFuncs->vkMapMemory(this->dev, m_bufMem, m_uniformBufInfo[currentFrame()].offset, UNIFORM_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if(err != VK_SUCCESS)
        qFatal("Failed to map memory: %d", err);
    QMatrix4x4 m = m_proj;
    m.rotate(m_rotation, 0, 1, 0);
    memcpy(p, m.constData(), 16 * sizeof(float));
    deviceFuncs->vkUnmapMemory(this->dev, m_bufMem);

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
    _deviceFuncs = vulkanInstance()->deviceFunctions(this->dev);

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
    const VkPhysicalDeviceLimits* pdevLimits = &physicalDeviceProperties()->limits;
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

    VkResult err = _deviceFuncs->vkCreateBuffer(this->dev, &bufInfo, nullptr, &m_buf);
    if(err != VK_SUCCESS)
        qFatal("Failed to create buffer: %d", err);

    VkMemoryRequirements memReq;
    _deviceFuncs->vkGetBufferMemoryRequirements(this->dev, m_buf, &memReq);

    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        hostVisibleMemoryIndex()
    };

    err = _deviceFuncs->vkAllocateMemory(this->dev, &memAllocInfo, nullptr, &m_bufMem);
    if(err != VK_SUCCESS)
        qFatal("Failed to allocate memory: %d", err);

    err = _deviceFuncs->vkBindBufferMemory(this->dev, m_buf, m_bufMem, 0);
    if(err != VK_SUCCESS)
        qFatal("Failed to bind buffer memory: %d", err);

    quint8 *p;
    err = _deviceFuncs->vkMapMemory(this->dev, m_bufMem, 0, memReq.size, 0, reinterpret_cast<void **>(&p));
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
    _deviceFuncs->vkUnmapMemory(this->dev, m_bufMem);

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
    err = _deviceFuncs->vkCreateDescriptorPool(this->dev, &descPoolInfo, nullptr, &m_descPool);
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
    err = _deviceFuncs->vkCreateDescriptorSetLayout(this->dev, &descLayoutInfo, nullptr, &m_descSetLayout);
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
        err = _deviceFuncs->vkAllocateDescriptorSets(this->dev, &descSetAllocInfo, &m_descSet[i]);
        if(err != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", err);

        VkWriteDescriptorSet descWrite;
        memset(&descWrite, 0, sizeof(descWrite));
        descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descWrite.dstSet = m_descSet[i];
        descWrite.descriptorCount = 1;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descWrite.pBufferInfo = &m_uniformBufInfo[i];
        _deviceFuncs->vkUpdateDescriptorSets(this->dev, 1, &descWrite, 0, nullptr);
    }

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = _deviceFuncs->vkCreatePipelineCache(this->dev, &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descSetLayout;
    err = _deviceFuncs->vkCreatePipelineLayout(this->dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", err);

    // Shaders
    VkShaderModule vertShaderModule = VulkanSceneRenderer::createShader(this->dev, QStringLiteral(":/vulkanrenderer/color.vert.spv"));
    VkShaderModule fragShaderModule = VulkanSceneRenderer::createShader(this->dev, QStringLiteral(":/vulkanrenderer/color.frag.spv"));

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

    err = _deviceFuncs->vkCreateGraphicsPipelines(this->dev, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if(err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", err);

    if(vertShaderModule)
        _deviceFuncs->vkDestroyShaderModule(this->dev, vertShaderModule, nullptr);
    if(fragShaderModule)
        _deviceFuncs->vkDestroyShaderModule(this->dev, fragShaderModule, nullptr);
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
        _deviceFuncs->vkDestroyPipeline(this->dev, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if(m_pipelineLayout) {
        _deviceFuncs->vkDestroyPipelineLayout(this->dev, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if(m_pipelineCache) {
        _deviceFuncs->vkDestroyPipelineCache(this->dev, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descSetLayout) {
        _deviceFuncs->vkDestroyDescriptorSetLayout(this->dev, m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if (m_descPool) {
        _deviceFuncs->vkDestroyDescriptorPool(this->dev, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    if (m_buf) {
        _deviceFuncs->vkDestroyBuffer(this->dev, m_buf, nullptr);
        m_buf = VK_NULL_HANDLE;
    }

    if (m_bufMem) {
        _deviceFuncs->vkFreeMemory(this->dev, m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }
}

}	// End of namespace
