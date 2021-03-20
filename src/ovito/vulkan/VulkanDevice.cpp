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
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include "VulkanDevice.h"

namespace Ovito {

// The logging category used for Vulkan-related information.
Q_LOGGING_CATEGORY(lcVulkan, "ovito.vulkan");

/******************************************************************************
* Callback function for Vulkan debug layers.
******************************************************************************/
static bool vulkanDebugFilter(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage)
{
    return false;
}

/******************************************************************************
* Returns a reference to the global Vulkan instance.
******************************************************************************/
std::shared_ptr<QVulkanInstance> VulkanDevice::vkInstance()
{
	static std::weak_ptr<QVulkanInstance> globalInstance;
	if(std::shared_ptr<QVulkanInstance> inst = globalInstance.lock()) {
		return inst;
	}
	else {
		inst = std::make_shared<QVulkanInstance>();
#ifdef OVITO_DEBUG
		inst->setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
        inst->installDebugOutputFilter(&vulkanDebugFilter);
#endif
		if(!inst->create())
			throw Exception(tr("Failed to create Vulkan instance: %1").arg(inst->errorCode()));
		globalInstance = inst;
		return inst;
	}
}

/******************************************************************************
* Returns the list of properties for the supported physical devices in the system.
* This function can be called before creating the logical device.
******************************************************************************/
QVector<VkPhysicalDeviceProperties> VulkanDevice::availablePhysicalDevices()
{
    if(!_physDevs.isEmpty() && !_physDevProps.isEmpty())
        return _physDevProps;

    QVulkanFunctions* f = vulkanInstance()->functions();
    uint32_t count = 1;
    VkResult err = f->vkEnumeratePhysicalDevices(vulkanInstance()->vkInstance(), &count, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("VulkanDevice: Failed to get physical device count: %d", err);
        return _physDevProps;
    }
    qCDebug(lcVulkan, "%d physical devices", count);
    if (!count)
        return _physDevProps;
    QVector<VkPhysicalDevice> devs(count);
    err = f->vkEnumeratePhysicalDevices(vulkanInstance()->vkInstance(), &count, devs.data());
    if(err != VK_SUCCESS) {
        qWarning("VulkanDevice: Failed to enumerate physical devices: %d", err);
        return _physDevProps;
    }
    _physDevs = devs;
    _physDevProps.resize(count);
    for(uint32_t i = 0; i < count; ++i) {
        VkPhysicalDeviceProperties* p = &_physDevProps[i];
        f->vkGetPhysicalDeviceProperties(_physDevs.at(i), p);
        qCDebug(lcVulkan, "Physical device [%d]: name '%s' version %d.%d.%d", i, p->deviceName,
                VK_VERSION_MAJOR(p->driverVersion), VK_VERSION_MINOR(p->driverVersion),
                VK_VERSION_PATCH(p->driverVersion));
    }
    return _physDevProps;
}

/******************************************************************************
* Requests the usage of the physical device with index \a idx. The index
* corresponds to the list returned from availablePhysicalDevices().
* By default the first physical device is used.
* 
* This function must be called before the logical device is created.
******************************************************************************/
void VulkanDevice::setPhysicalDeviceIndex(int idx)
{
    if(_device != VK_NULL_HANDLE) {
        qWarning("VulkanDevice: Attempted to set physical device when already initialized");
        return;
    }
    const int count = availablePhysicalDevices().count();
    if(idx < 0 || idx >= count) {
        qWarning("VulkanDevice: Invalid physical device index %d (total physical devices: %d)", idx, count);
        return;
    }
    _physDevIndex = idx;
}

/******************************************************************************
* Returns the list of the extensions that are supported by logical devices
* created from the physical device selected by setPhysicalDeviceIndex().
*
* This function can be called before making creating the logical device.
******************************************************************************/
QVulkanInfoVector<QVulkanExtension> VulkanDevice::supportedDeviceExtensions()
{
    availablePhysicalDevices();
    if(_physDevs.isEmpty()) {
        qWarning("VulkanDevice: No physical devices found");
        return QVulkanInfoVector<QVulkanExtension>();
    }
    VkPhysicalDevice physDev = _physDevs.at(_physDevIndex);

	// Look up extensions in the cache.
    if(_supportedDevExtensions.contains(physDev))
        return _supportedDevExtensions.value(physDev);

    QVulkanFunctions* f = vulkanInstance()->functions();
    uint32_t count = 0;
    VkResult err = f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &count, nullptr);
    if(err == VK_SUCCESS) {
        QVector<VkExtensionProperties> extProps(count);
        err = f->vkEnumerateDeviceExtensionProperties(physDev, nullptr, &count, extProps.data());
        if(err == VK_SUCCESS) {
            QVulkanInfoVector<QVulkanExtension> exts;
            for(const VkExtensionProperties& prop : extProps) {
                QVulkanExtension ext;
                ext.name = prop.extensionName;
                ext.version = prop.specVersion;
                exts.append(ext);
            }
            _supportedDevExtensions.insert(physDev, exts);
//            qDebug(lcVulkan) << "Supported device extensions:" << exts;
            return exts;
        }
    }
    qWarning("VulkanDevice: Failed to query device extension count: %d", err);
    return {};
}

/******************************************************************************
* Sets the list of device \a extensions to be enabled. Unsupported extensions 
* are ignored.
*
* This function must be called before the logical device is created.
******************************************************************************/
void VulkanDevice::setDeviceExtensions(const QByteArrayList& extensions)
{
    if(_device != VK_NULL_HANDLE) {
        qWarning("VulkanDevice: Attempted to set device extensions when already initialized");
        return;
    }
    _requestedDevExtensions = extensions;
}

/******************************************************************************
* Creates the logical Vulkan device.  
******************************************************************************/
bool VulkanDevice::create(QWindow* window)
{
    OVITO_ASSERT(vulkanInstance());

    // Is the device already created?
    if(_device != VK_NULL_HANDLE)
        return true;

    _vulkanFunctions = vulkanInstance()->functions();
	
    qCDebug(lcVulkan, "VulkanDevice create");

	// Get the list of available physical devices.
    availablePhysicalDevices();
    if(_physDevs.isEmpty())
		throw Exception(tr("No Vulkan devices present in the system."));

    if(_physDevIndex < 0 || _physDevIndex >= _physDevs.count()) {
        qWarning("VulkanDevice: Invalid physical device index; defaulting to 0");
        _physDevIndex = 0;
    }

    qCDebug(lcVulkan, "Using physical device [%d]", _physDevIndex);

    VkPhysicalDevice physDev = physicalDevice();

	// Enumerate the device's queue families.
    uint32_t queueCount = 0;
    vulkanFunctions()->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, nullptr);
    QVector<VkQueueFamilyProperties> queueFamilyProps(queueCount);
    vulkanFunctions()->vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueCount, queueFamilyProps.data());

    _gfxQueueFamilyIdx = uint32_t(-1);
    _presQueueFamilyIdx = uint32_t(-1);
    for(int i = 0; i < queueFamilyProps.count(); ++i) {
        const bool supportsPresent = vulkanInstance()->supportsPresent(physDev, i, window);
        qCDebug(lcVulkan, "queue family %d: flags=0x%x count=%d supportsPresent=%d", i,
                queueFamilyProps[i].queueFlags, queueFamilyProps[i].queueCount, supportsPresent);
        if(_gfxQueueFamilyIdx == uint32_t(-1)
                && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                && supportsPresent)
            _gfxQueueFamilyIdx = i;
    }
    if(_gfxQueueFamilyIdx != uint32_t(-1))
        _presQueueFamilyIdx = _gfxQueueFamilyIdx;
    else {
        qCDebug(lcVulkan, "No queue with graphics+present; trying separate queues");
        for(int i = 0; i < queueFamilyProps.count(); ++i) {
            if(_gfxQueueFamilyIdx == uint32_t(-1) && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                _gfxQueueFamilyIdx = i;
            if(_presQueueFamilyIdx == uint32_t(-1) && vulkanInstance()->supportsPresent(physDev, i, window))
                _presQueueFamilyIdx = i;
        }
    }
    if(_gfxQueueFamilyIdx == uint32_t(-1))
		throw Exception(tr("Cannot initialize Vulkan rendering device. No graphics queue family found."));
    if(_presQueueFamilyIdx == uint32_t(-1))
		throw Exception(tr("Cannot initialize Vulkan rendering device. No present queue family found."));

#ifdef OVITO_DEBUG
    // Allow testing the separate present queue case in debug builds on AMD cards
    if(qEnvironmentVariableIsSet("QT_VK_PRESENT_QUEUE_INDEX"))
        _presQueueFamilyIdx = qEnvironmentVariableIntValue("QT_VK_PRESENT_QUEUE_INDEX");
#endif

    qCDebug(lcVulkan, "Using queue families: graphics = %u present = %u", _gfxQueueFamilyIdx, _presQueueFamilyIdx);

    // Filter out unsupported extensions in order to keep symmetry
    // with how QVulkanInstance behaves. Add the swapchain extension when 
	// the device is to be used for a window.
    QVector<const char*> devExts;
    QVulkanInfoVector<QVulkanExtension> supportedExtensions = supportedDeviceExtensions();
    QByteArrayList reqExts = _requestedDevExtensions;
	if(window != nullptr)
	    reqExts.append("VK_KHR_swapchain");
    for(const QByteArray& ext : reqExts) {
        if(supportedExtensions.contains(ext))
            devExts.append(ext.constData());
    }
    qCDebug(lcVulkan) << "Enabling device extensions:" << devExts;

	// Prepare data structure for logical device creation.
    VkDeviceQueueCreateInfo queueInfo[2];
    const float prio[] = { 0.0f };
    memset(queueInfo, 0, sizeof(queueInfo));
    queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo[0].queueFamilyIndex = _gfxQueueFamilyIdx;
    queueInfo[0].queueCount = 1;
    queueInfo[0].pQueuePriorities = prio;
    if(_gfxQueueFamilyIdx != _presQueueFamilyIdx) {
        queueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo[1].queueFamilyIndex = _presQueueFamilyIdx;
        queueInfo[1].queueCount = 1;
        queueInfo[1].pQueuePriorities = prio;
    }

    VkDeviceCreateInfo devInfo;
    memset(&devInfo, 0, sizeof(devInfo));
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.queueCreateInfoCount = !separatePresentQueue() ? 1 : 2;
    devInfo.pQueueCreateInfos = queueInfo;
    devInfo.enabledExtensionCount = devExts.count();
    devInfo.ppEnabledExtensionNames = devExts.constData();
    // Device layers are not supported by this implementation since that's an already deprecated
    // API. However, have a workaround for systems with older API and layers (f.ex. L4T
    // 24.2 for the Jetson TX1 provides API 1.0.13 and crashes when the validation layer
    // is enabled for the instance but not the device).
    uint32_t apiVersion = _physDevProps[_physDevIndex].apiVersion;
    if(VK_VERSION_MAJOR(apiVersion) == 1 && VK_VERSION_MINOR(apiVersion) == 0 && VK_VERSION_PATCH(apiVersion) <= 13) {
        // Make standard validation work at least.
        const QByteArray stdValName = QByteArrayLiteral("VK_LAYER_LUNARG_standard_validation");
        const char* stdValNamePtr = stdValName.constData();
        if(vulkanInstance()->layers().contains(stdValName)) {
            uint32_t count = 0;
            VkResult err = vulkanFunctions()->vkEnumerateDeviceLayerProperties(physDev, &count, nullptr);
            if(err == VK_SUCCESS) {
                QVector<VkLayerProperties> layerProps(count);
                err = vulkanFunctions()->vkEnumerateDeviceLayerProperties(physDev, &count, layerProps.data());
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

    // Query the device's available features.
    VkPhysicalDeviceFeatures availableFeatures;
    vulkanFunctions()->vkGetPhysicalDeviceFeatures(physDev, &availableFeatures);
    // Enable the feature which we can use.
    memset(&_physicalDeviceFeatures, 0, sizeof(_physicalDeviceFeatures));
    if(availableFeatures.wideLines)
        _physicalDeviceFeatures.wideLines = VK_TRUE;
    devInfo.pEnabledFeatures = &_physicalDeviceFeatures;

    VkResult err = vulkanFunctions()->vkCreateDevice(physDev, &devInfo, nullptr, &_device);
    if(err == VK_ERROR_DEVICE_LOST) {
        qWarning("VulkanDevice: Physical device lost");
        Q_EMIT physicalDeviceLost();
        // Clear the caches so the list of physical devices is re-queried
        _physDevs.clear();
        _physDevProps.clear();
        return false;
    }
    if(err != VK_SUCCESS)
        throw Exception(tr("Failed to create logical Vulkan device (error code %1).").arg(err));

	// Get the function pointers for device-specific Vulkan functions.
    _deviceFunctions = vulkanInstance()->deviceFunctions(_device);
    OVITO_ASSERT(_deviceFunctions);

	// Retrieve the queue handles from the device.
    deviceFunctions()->vkGetDeviceQueue(logicalDevice(), _gfxQueueFamilyIdx, 0, &_gfxQueue);
    if(!separatePresentQueue())
        _presQueue = _gfxQueue;
    else
        deviceFunctions()->vkGetDeviceQueue(logicalDevice(), _presQueueFamilyIdx, 0, &_presQueue);

	// Create command pools.
    VkCommandPoolCreateInfo poolInfo;
    memset(&poolInfo, 0, sizeof(poolInfo));
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = _gfxQueueFamilyIdx;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    err = deviceFunctions()->vkCreateCommandPool(logicalDevice(), &poolInfo, nullptr, &_cmdPool);
    if(err != VK_SUCCESS)
        throw Exception(tr("Failed to create Vulkan command pool (error code %1).").arg(err));
    if(separatePresentQueue()) {
        poolInfo.queueFamilyIndex = _presQueueFamilyIdx;
        poolInfo.flags = 0;
        err = deviceFunctions()->vkCreateCommandPool(logicalDevice(), &poolInfo, nullptr, &_presCmdPool);
        if(err != VK_SUCCESS)
	        throw Exception(tr("Failed to create Vulkan command pool for present queue (error code %1).").arg(err));
    }

    _hostVisibleMemIndex = 0;
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    bool hostVisibleMemIndexSet = false;
    vulkanFunctions()->vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &physDevMemProps);
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        const VkMemoryType* memType = physDevMemProps.memoryTypes;
        qCDebug(lcVulkan, "memtype %d: flags=0x%x", i, memType[i].propertyFlags);
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
    qCDebug(lcVulkan, "Picked memtype %d for host visible memory", _hostVisibleMemIndex);

    _deviceLocalMemIndex = 0;
    for(uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
        const VkMemoryType* memType = physDevMemProps.memoryTypes;
        // Just pick the first device local memtype.
        if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            _deviceLocalMemIndex = i;
            break;
        }
    }
    qCDebug(lcVulkan, "Picked memtype %d for device local memory", _deviceLocalMemIndex);

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
        vulkanFunctions()->vkGetPhysicalDeviceFormatProperties(physicalDevice(), _dsFormat, &fmtProp);
        if(fmtProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            break;
        ++dsFormatIdx;
    }
    if(dsFormatIdx == dsFormatCandidateCount)
        qWarning("VulkanDevice: Failed to find an optimal depth-stencil format");
    qCDebug(lcVulkan, "Depth-stencil format: %d", _dsFormat);

    // Create pipeline cache.
    VkPipelineCacheCreateInfo pipelineCacheInfo;
    memset(&pipelineCacheInfo, 0, sizeof(pipelineCacheInfo));
    pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = deviceFunctions()->vkCreatePipelineCache(logicalDevice(), &pipelineCacheInfo, nullptr, &_pipelineCache);
    if(err != VK_SUCCESS)
        throw Exception(tr("Failed to create Vulkan pipeline cache (error code %1).").arg(err));

	return true;
}

/******************************************************************************
* Picks the right memory type for a Vulkan image.
******************************************************************************/
uint32_t VulkanDevice::chooseTransientImageMemType(VkImage img, uint32_t startIndex)
{
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    vulkanFunctions()->vkGetPhysicalDeviceMemoryProperties(_physDevs[_physDevIndex], &physDevMemProps);
    VkMemoryRequirements memReq;
    deviceFunctions()->vkGetImageMemoryRequirements(logicalDevice(), img, &memReq);
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
* Releases all Vulkan resources.
******************************************************************************/
void VulkanDevice::reset()
{
    if(logicalDevice() == VK_NULL_HANDLE)
        return;

    // Tell clients of the class to also release their Vulkan resources.
    Q_EMIT releaseResourcesRequested();

    // Make sure our clients have release their resources properly.
    OVITO_ASSERT(_activeResourceFrames.empty());
    OVITO_ASSERT(_dataBuffers.empty());

    qCDebug(lcVulkan, "VulkanDevice reset");

    // Release command buffer pool used for graphics rendering.
    if(graphicsCommandPool() != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyCommandPool(logicalDevice(), graphicsCommandPool(), nullptr);
        _cmdPool = VK_NULL_HANDLE;
    }

    // Release command buffer pool used for presentation.
    if(presentCommandPool() != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyCommandPool(logicalDevice(), presentCommandPool(), nullptr);
        _presCmdPool = VK_NULL_HANDLE;
    }

    // Release pipeline cache.
    if(pipelineCache() != VK_NULL_HANDLE) {
        deviceFunctions()->vkDestroyPipelineCache(logicalDevice(), pipelineCache(), nullptr);
        _pipelineCache = VK_NULL_HANDLE;
    }

    // Release the logical device.
	deviceFunctions()->vkDestroyDevice(logicalDevice(), nullptr);
    // Discard cached device function pointers held by Qt.
	vulkanInstance()->resetDeviceFunctions(logicalDevice());

    // Reset internal handles.
	_device = VK_NULL_HANDLE;
	_deviceFunctions = nullptr;
}

/******************************************************************************
* Handles the situation when the Vulkan device was lost after a recent function call.
******************************************************************************/
bool VulkanDevice::checkDeviceLost(VkResult err)
{
    if(err == VK_ERROR_DEVICE_LOST) {
        qWarning("VulkanDevice: Device lost");
        qCDebug(lcVulkan, "Releasing all resources due to device lost");
        reset();
        qCDebug(lcVulkan, "Restarting after device lost");
        Q_EMIT logicalDeviceLost(); // This calls VulkanViewportWindow::ensureStarted()
        return true;
    }
    return false;
}

/******************************************************************************
* Helper routine for creating a Vulkan image.
******************************************************************************/
bool VulkanDevice::createVulkanImage(const QSize size,
                                        VkFormat format,
                                        VkSampleCountFlagBits sampleCount,
                                        VkImageUsageFlags usage,
                                        VkImageAspectFlags aspectMask,
                                        VkImage* images,
                                        VkDeviceMemory* mem,
                                        VkImageView* views,
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
        imgInfo.extent.width = size.width();
        imgInfo.extent.height = size.height();
        imgInfo.extent.depth = 1;
        imgInfo.mipLevels = 1;
        imgInfo.arrayLayers = 1;
        imgInfo.samples = sampleCount;
        imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage = usage;
        err = deviceFunctions()->vkCreateImage(logicalDevice(), &imgInfo, nullptr, images + i);
        if(err != VK_SUCCESS) {
            qWarning("VulkanDevice: Failed to create image: %d", err);
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
    memInfo.allocationSize = VulkanDevice::aligned(memReq.size, memReq.alignment) * count;
    uint32_t startIndex = 0;
    do {
        memInfo.memoryTypeIndex = chooseTransientImageMemType(images[0], startIndex);
        if(memInfo.memoryTypeIndex == uint32_t(-1)) {
            qWarning("VulkanDevice: No suitable memory type found");
            return false;
        }
        startIndex = memInfo.memoryTypeIndex + 1;
        qCDebug(lcVulkan, "Allocating %u bytes for transient image (memtype %u)", uint32_t(memInfo.allocationSize), memInfo.memoryTypeIndex);
        err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memInfo, nullptr, mem);
        if(err != VK_SUCCESS && err != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            qWarning("VulkanDevice: Failed to allocate image memory: %d", err);
            return false;
        }
    } 
    while(err != VK_SUCCESS);
    VkDeviceSize ofs = 0;
    for(int i = 0; i < count; ++i) {
        err = deviceFunctions()->vkBindImageMemory(logicalDevice(), images[i], *mem, ofs);
        if(err != VK_SUCCESS) {
            qWarning("VulkanDevice: Failed to bind image memory: %d", err);
            return false;
        }
        ofs += VulkanDevice::aligned(memReq.size, memReq.alignment);
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
            qWarning("VulkanDevice: Failed to create image view: %d", err);
            return false;
        }
    }
    return true;
}

/******************************************************************************
* Creates a default Vulkan render pass.
******************************************************************************/
VkRenderPass VulkanDevice::createDefaultRenderPass(VkFormat colorFormat, VkSampleCountFlagBits sampleCount)
{
    VkAttachmentDescription attDesc[3];
    memset(attDesc, 0, sizeof(attDesc));
    const bool msaa = sampleCount > VK_SAMPLE_COUNT_1_BIT;
    // This is either the non-msaa render target or the resolve target.
    attDesc[0].format = colorFormat;
    attDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // ignored when msaa
    attDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attDesc[1].format = depthStencilFormat();
    attDesc[1].samples = sampleCount;
    attDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attDesc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if(msaa) {
        // msaa render target
        attDesc[2].format = colorFormat;
        attDesc[2].samples = sampleCount;
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
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkResult err = deviceFunctions()->vkCreateRenderPass(logicalDevice(), &rpInfo, nullptr, &renderPass);
    if(err != VK_SUCCESS) {
        qWarning("VulkanDevice: Failed to create renderpass: %d", err);
        return VK_NULL_HANDLE;
    }
    return renderPass;
}

/******************************************************************************
* Loads a SPIR-V shader from a file.
******************************************************************************/
VkShaderModule VulkanDevice::createShader(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
		throw Exception(tr("File to load Vulkan shader file '%1': %2").arg(filename).arg(file.errorString()));
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
    VkShaderModule shaderModule;
    VkResult err = deviceFunctions()->vkCreateShaderModule(_device, &shaderInfo, nullptr, &shaderModule);
    if(err != VK_SUCCESS)
		throw Exception(tr("File to create Vulkan shader module '%1'. Error code: %2").arg(filename).arg(err));

    return shaderModule;
}

/******************************************************************************
* Informs the resource manager that a new frame starts being rendered.
******************************************************************************/
VulkanDevice::ResourceFrameHandle VulkanDevice::acquireResourceFrame()
{    
    if(_activeResourceFrames.empty()) {
        OVITO_ASSERT(_dataBuffers.empty());
        _activeResourceFrames.push_back(1);
    }
    else {
        _activeResourceFrames.push_back(_activeResourceFrames.back() + 1);
    }

    return _activeResourceFrames.back();
}

/******************************************************************************
* Informs the resource manager that a frame has completely finished rendering 
* and all related Vulkan resources can be released.
******************************************************************************/
void VulkanDevice::releaseResourceFrame(ResourceFrameHandle frame)
{
    OVITO_ASSERT(frame > 0);
    OVITO_ASSERT(std::is_sorted(_activeResourceFrames.begin(), _activeResourceFrames.end()));

    auto iter = std::lower_bound(_activeResourceFrames.begin(), _activeResourceFrames.end(), frame);
    OVITO_ASSERT(iter != _activeResourceFrames.end() && *iter == frame);
    _activeResourceFrames.erase(iter);

    // The oldest frame that is still active. We can release all resource from earlier frames.
    ResourceFrameHandle oldestFrame = _activeResourceFrames.empty() ? std::numeric_limits<ResourceFrameHandle>::max() : _activeResourceFrames.front();

    // Release all Vulkan resources that are no longer in use.
    for(auto iter =_dataBuffers.begin(); iter != _dataBuffers.end(); ) {
        if(iter->second.resourceFrame < oldestFrame) {
            OVITO_ASSERT(iter->second.buffer);
            OVITO_ASSERT(iter->second.bufferMem);
            deviceFunctions()->vkDestroyBuffer(logicalDevice(), iter->second.buffer, nullptr);
            deviceFunctions()->vkFreeMemory(logicalDevice(), iter->second.bufferMem, nullptr);
            iter = _dataBuffers.erase(iter);
        }
        else {
            ++iter;
        }
    }
}

/******************************************************************************
* Uploads an OVITO DataBuffer to the Vulkan device.
******************************************************************************/
VkBuffer VulkanDevice::uploadDataBuffer(const ConstDataBufferPtr& dataBuffer, ResourceFrameHandle resourceFrame)
{
    OVITO_ASSERT(dataBuffer);
    OVITO_ASSERT(logicalDevice());
    OVITO_ASSERT(std::binary_search(_activeResourceFrames.begin(), _activeResourceFrames.end(), resourceFrame));

	// This method must be called from the main thread where the Vulkan device lives.
	OVITO_ASSERT(QThread::currentThread() == this->thread());

    // Check if data buffer has already been uploaded.
    auto iter = _dataBuffers.find(dataBuffer);
    if(iter != _dataBuffers.end()) {
        // Update frame in which the resource was actively used.
        OVITO_ASSERT(resourceFrame >= iter->second.resourceFrame);
        iter->second.resourceFrame = resourceFrame;
        return iter->second.buffer;
    }

    DataBufferInfo bufferInfo;
    bufferInfo.resourceFrame = resourceFrame;

    // Create a vertex buffer.
    VkBufferCreateInfo bufferCreateInfo;
    memset(&bufferCreateInfo, 0, sizeof(bufferCreateInfo));
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    if(dataBuffer->dataType() == DataBuffer::Float) {
        bufferCreateInfo.size = dataBuffer->size() * dataBuffer->componentCount() * sizeof(float);
    }
    else {
        OVITO_ASSERT(false);
        dataBuffer->throwException(tr("Cannot create Vulkan vertex buffer for DataBuffer with data type %1.").arg(dataBuffer->dataType()));
    }
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // The buffer will only be used from the graphics queue, so we can stick to exclusive access.
    VkResult err = deviceFunctions()->vkCreateBuffer(logicalDevice(), &bufferCreateInfo, nullptr, &bufferInfo.buffer);
    if(err != VK_SUCCESS)
        dataBuffer->throwException(tr("Failed to create Vulkan vertex buffer (error code %1).").arg(err));

    // Allocate memory for the vertex buffer.
    VkMemoryRequirements memReq;
    deviceFunctions()->vkGetBufferMemoryRequirements(logicalDevice(), bufferInfo.buffer, &memReq);
    VkMemoryAllocateInfo memAllocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        hostVisibleMemoryIndex()
    };
    err = deviceFunctions()->vkAllocateMemory(logicalDevice(), &memAllocInfo, nullptr, &bufferInfo.bufferMem);
    if(err != VK_SUCCESS)
        dataBuffer->throwException(tr("Failed to allocate Vulkan vertex buffer memory (error code %1).").arg(err));

    // Bind the memory to the vertex buffer.
    err = deviceFunctions()->vkBindBufferMemory(logicalDevice(), bufferInfo.buffer, bufferInfo.bufferMem, 0);
    if(err != VK_SUCCESS)
        dataBuffer->throwException(tr("Failed to bind Vulkan vertex buffer memory (error code %1).").arg(err));

    // Fill the vertex buffer with data.
    void* p;
    err = deviceFunctions()->vkMapMemory(logicalDevice(), bufferInfo.bufferMem, 0, memReq.size, 0, &p);
    if(err != VK_SUCCESS)
        dataBuffer->throwException(tr("Failed to map memory of Vulkan vertex buffer (error code %1).").arg(err));
    if(dataBuffer->dataType() == DataBuffer::Float) {
        // Convert from FloatType to float data type.
        OVITO_ASSERT(dataBuffer->stride() == sizeof(FloatType) * dataBuffer->componentCount());
        float* dst = static_cast<float*>(p);
        ConstDataBufferAccess<FloatType, true> arrayAccess(dataBuffer);
        for(const FloatType* src = arrayAccess.cbegin(); src != arrayAccess.cend(); ++src, ++dst)
            *dst = static_cast<float>(*src);
    }
    deviceFunctions()->vkUnmapMemory(logicalDevice(), bufferInfo.bufferMem);

    // Insert buffer record into cache.
    _dataBuffers.insert(std::make_pair(dataBuffer, bufferInfo));

    return bufferInfo.buffer;
}

}	// End of namespace
