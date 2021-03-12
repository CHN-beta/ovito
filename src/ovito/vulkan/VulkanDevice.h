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

#pragma once


#include <ovito/core/Core.h>

#include <QVulkanInstance>

namespace Ovito {

/**
 * \brief Encapsulates a Vulkan logical device.
 */
class OVITO_VULKANRENDERER_EXPORT VulkanDevice : public QObject
{
	Q_OBJECT

public:

	/// Returns a shared pointer to the global Vulkan instance.
	static std::shared_ptr<QVulkanInstance> vkInstance();

	/// Returns the Vulkan instance associated with the device.
    QVulkanInstance* vulkanInstance() const { return _vulkanInstance.get(); }

	/// Returns the list of properties for the supported physical devices in the system.
	/// This function can be called before making the window visible.
	QVector<VkPhysicalDeviceProperties> availablePhysicalDevices();

	/// Requests the usage of the physical device with index \a idx. The index
	/// corresponds to the list returned from availablePhysicalDevices().
	void setPhysicalDeviceIndex(int idx);

	/// Returns the list of the extensions that are supported by logical devices
	/// created from the physical device selected by setPhysicalDeviceIndex().
	QVulkanInfoVector<QVulkanExtension> supportedDeviceExtensions();

	/// Returns a pointer to the properties for the active physical device.
	const VkPhysicalDeviceProperties* physicalDeviceProperties() const {
		if(_physDevIndex < _physDevProps.count())
	        return &_physDevProps[_physDevIndex];
	    qWarning("VulkanDevice: Physical device properties not available");
	    return nullptr;
	}

	/// Sets the list of device \a extensions to be enabled. Unsupported extensions 
	/// are ignored. The swapchain extension will always be added automatically, 
	/// no need to include it in this list.
	void setDeviceExtensions(const QByteArrayList& extensions);

	/// Returns the active physical device.
	VkPhysicalDevice physicalDevice() const { return _physDevIndex < _physDevs.size() ? _physDevs[_physDevIndex] : VK_NULL_HANDLE; }

	/// Creates the logical Vulkan device.
	bool create(QWindow* window);

	/// Releases all Vulkan resources.
	void reset();

	/// Returns the internal Vulkan logical device handle.
	VkDevice logicalDevice() const { return _device; }

	/// Returns the table of Vulkan device-independent functions.
	QVulkanFunctions* vulkanFunctions() const { return _vulkanFunctions; }

	/// Returns the device-specific Vulkan function table. 
	QVulkanDeviceFunctions* deviceFunctions() const { return _deviceFunctions; }

	/// Returns the index of the queue family used for graphics rendering.
    uint32_t graphicsQueueFamilyIndex() const { return _gfxQueueFamilyIdx; }

	/// Returns the index of the queue family used for window presentation.
    uint32_t presentQueueFamilyIndex() const { return _presQueueFamilyIdx; }

	/// Returns the Vulkan queue used for graphics rendering.
    VkQueue graphicsQueue() const { return _gfxQueue; }

	/// The Vulkan queue used for window presentation.
    VkQueue presentQueue() const { return _presQueue; }

	/// Returns whether a separate present queue family is used.
	bool separatePresentQueue() const { return _presQueueFamilyIdx != _gfxQueueFamilyIdx; }

	/// Returns the command pool for creating commands for the graphics queue.
    VkCommandPool graphicsCommandPool() const { return _cmdPool; };

	/// Returns the command pool for creating commands for the present queue.
    VkCommandPool presentCommandPool() const { return _presCmdPool; };

	/// Picks the right memory type for a Vulkan image.
	uint32_t chooseTransientImageMemType(VkImage img, uint32_t startIndex);

	/// Loads a SPIR-V shader from a file.
	VkShaderModule createShader(const QString& filename);

Q_SIGNALS:

	/// Is emitted when the physical device is lost, meaning the creation of the logical device fails with VK_ERROR_DEVICE_LOST.
	void physicalDeviceLost();

private:

	/// The global Vulkan instance associated with the device.
    std::shared_ptr<QVulkanInstance> _vulkanInstance = vkInstance();

	/// The table of Vulkan device-independent functions.
	QVulkanFunctions* _vulkanFunctions = nullptr;

	/// The internal Vulkan logical device handle.
	VkDevice _device = VK_NULL_HANDLE;

	/// The device-specific Vulkan function table. 
	QVulkanDeviceFunctions* _deviceFunctions = nullptr;

	/// The selected physical device index from which the logical device is created. 
    int _physDevIndex = 0;

	/// The list of physical Vulkan devices.
    QVector<VkPhysicalDevice> _physDevs;

	/// The properties of each physical Vulkan device in the system.
    QVector<VkPhysicalDeviceProperties> _physDevProps;

	/// The extensions supported by each physical Vulkan device.
    QHash<VkPhysicalDevice, QVulkanInfoVector<QVulkanExtension>> _supportedDevExtensions;

	/// The list of device extensions requested by the user of the class.
    QByteArrayList _requestedDevExtensions;

	/// The queue family used for graphics rendering.
    uint32_t _gfxQueueFamilyIdx;

	/// The queue family used for window presentation.
    uint32_t _presQueueFamilyIdx;

	/// The Vulkan queue used for graphics rendering.
    VkQueue _gfxQueue;

	/// The Vulkan queue used for window presentation.
    VkQueue _presQueue;

	/// The command pool for creating commands for the graphics queue.
    VkCommandPool _cmdPool = VK_NULL_HANDLE;

	/// The command pool for creating commands for the presentation queue.
    VkCommandPool _presCmdPool = VK_NULL_HANDLE;
};

}	// End of namespace
