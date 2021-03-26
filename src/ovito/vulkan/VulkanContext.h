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
#include <ovito/core/dataset/data/DataBuffer.h>

#include <QLoggingCategory>
#include <QVulkanInstance>
#include <QVulkanDeviceFunctions>
#include "vma/VulkanMemoryAllocator.h"

#include <boost/any.hpp>

namespace Ovito {

OVITO_VULKANRENDERER_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcVulkan);

/**
 * \brief A cache data structure that accepts keys with arbitrary type and which handles resource lifetime.
 */
template<typename Value>
class VulkanResourceCache
{
public:

	using ResourceFrameHandle = int;

	/// Returns a reference to the value for the given key.
	/// Creates a new cache entry with a default-initialized value if the key doesn't exist.
	template<typename Key>
	Value& lookup(Key&& key, ResourceFrameHandle resourceFrame) {
		// Check if the key exists in the cache.
		for(CacheEntry& entry : _entries) {
			if(entry.key.type() == typeid(Key) && key == boost::any_cast<const Key&>(entry.key)) {
				// Register the frame in which the resource was actively used.
				if(std::find(entry.frames.begin(), entry.frames.end(), resourceFrame) == entry.frames.end())
					entry.frames.push_back(resourceFrame);
				// Return reference to the value.
				return entry.value;
			}
		}
		// Create a new entry if key doesn't exist yet.
		_entries.emplace_back(std::forward<Key>(key), resourceFrame);
		return _entries.back().value;
	}

	/// Frees objects in the cache which are no longer needed.
	template<typename ReleaseFunc>
	void release(ResourceFrameHandle resourceFrame, ReleaseFunc&& func) {
		for(auto entry = _entries.begin(); entry != _entries.end(); ) {
			auto frameIter = std::find(entry->frames.begin(), entry->frames.end(), resourceFrame);
			if(frameIter != entry->frames.end()) {
				if(entry->frames.size() != 1) {
					*frameIter = entry->frames.back();
					entry->frames.pop_back();
				}
				else {
					func(entry->value);
					entry = _entries.erase(entry);
					continue;
				}
			}
			++entry;
		}
	}

	/// Indicates whether the cache is currently empty.
	bool empty() const { return _entries.empty(); }

	/// Returns the current number of cache entries.
	size_t size() const { return _entries.size(); }

private:

	struct CacheEntry {
		template<typename Key> CacheEntry(Key&& _key, ResourceFrameHandle _frame) : key(std::forward<Key>(_key)), value{}, frames{{_frame}} {}
		boost::any key;
		Value value;
		QVarLengthArray<ResourceFrameHandle, 6> frames;
	};

	std::vector<CacheEntry> _entries;
};

/**
 * \brief A tagged tuple, which can be used as key for the VulkanResourceCache class.
 */
template<typename TagType, typename... TupleFields>
struct VulkanResourceKey : public std::tuple<TupleFields...>
{
	/// Inherit constructors of tuple type.
	using std::tuple<TupleFields...>::tuple;
};

/**
 * \brief Encapsulates a Vulkan logical device.
 */
class OVITO_VULKANRENDERER_EXPORT VulkanContext : public QObject
{
	Q_OBJECT

public:

	/// Returns a shared pointer to the global Vulkan instance.
	static std::shared_ptr<QVulkanInstance> vkInstance();

	/// Maximum number of rendering frames that can be potentially active at the same time.
	static const int MAX_CONCURRENT_FRAME_COUNT = 3;

	/// Data type used by the Vulkan resource manager when referring to an active frame being rendered on the CPU and/or the GPU.
	using ResourceFrameHandle = int;

	/// Constructor
	VulkanContext(QObject* parent = nullptr);

	/// Destructor.
	~VulkanContext() { reset(); }

	/// Returns the Vulkan instance associated with the device.
    QVulkanInstance* vulkanInstance() const { return _vulkanInstance.get(); }

	/// Returns the list of properties for the supported physical devices in the system.
	/// This function can be called before making the window visible.
	QVector<VkPhysicalDeviceProperties> availablePhysicalDevices();

	/// Requests the usage of the physical device with index \a idx. The index
	/// corresponds to the list returned from availablePhysicalDevices().
	void setPhysicalDeviceIndex(int idx);

	/// Returns the index of the physical device to be used.
	int physicalDeviceIndex() const { return _physDevIndex; }

	/// Returns the list of the extensions that are supported by logical devices
	/// created from the physical device selected by setPhysicalDeviceIndex().
	QVulkanInfoVector<QVulkanExtension> supportedDeviceExtensions();

	/// Returns a pointer to the properties for the active physical device.
	const VkPhysicalDeviceProperties* physicalDeviceProperties() const {
		if(_physDevIndex < _physDevProps.count())
	        return &_physDevProps[_physDevIndex];
	    qWarning("VulkanContext: Physical device properties not available");
	    return nullptr;
	}

	/// Sets the list of device \a extensions to be enabled. Unsupported extensions 
	/// are ignored. The swapchain extension will always be added automatically, 
	/// no need to include it in this list.
	void setDeviceExtensions(const QByteArrayList& extensions);

	/// Returns the active physical device.
	VkPhysicalDevice physicalDevice() const { return _physDevIndex < _physDevs.size() ? _physDevs[_physDevIndex] : VK_NULL_HANDLE; }

	/// Creates the logical Vulkan device.
	bool create(QWindow* window = nullptr);

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

    /// Returns a host visible memory type index suitable for general use.
    /// The returned memory type will be both host visible and coherent. In addition, it will also be cached, if possible.
	uint32_t hostVisibleMemoryIndex() const { return _hostVisibleMemIndex; }

    /// Returns a device local memory type index suitable for general use.
	/// Note: It is not guaranteed that this memory type is always suitable. 
	/// The correct, cross-implementation solution - especially for device local images - is to manually 
	/// pick a memory type after checking the mask returned from vkGetImageMemoryRequirements.
	uint32_t deviceLocalMemoryIndex() const { return _deviceLocalMemIndex; }

	/// Returns the Vulkan Memory Allocator (VMA) used for this device.
	VmaAllocator allocator() const { return _allocator; }

	/// Picks the right memory type for a Vulkan image.
	uint32_t chooseTransientImageMemType(VkImage img, uint32_t startIndex);

	/// Handles the situation when the Vulkan device was lost after a recent function call.
	bool checkDeviceLost(VkResult err);

	/// Returns the format to use for the standard depth-stencil buffer.
    VkFormat depthStencilFormat() const { return _dsFormat; }

	/// Returns the global Vulkan pipeline cache.
	VkPipelineCache pipelineCache() const { return _pipelineCache; }

	/// Helper routine for creating a Vulkan image.
	bool createVulkanImage(const QSize size, VkFormat format, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage, VkImageAspectFlags aspectMask, VkImage* images, VkDeviceMemory* mem, VkImageView* views, int count);

	/// Creates a default Vulkan render pass.
	VkRenderPass createDefaultRenderPass(VkFormat colorFormat, VkSampleCountFlagBits sampleCount);

	/// Loads a SPIR-V shader from a file.
	VkShaderModule createShader(const QString& filename);

	static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign) {
	    return (v + byteAlign - 1) & ~(byteAlign - 1);
	}

	/// Synchronously executes some memory transfer commands.
	void immediateTransferSubmit(std::function<void(VkCommandBuffer)>&& function);

	/// Returns the standard texture sampler, which uses nearest interpolation.
	VkSampler samplerNearest() const { return _samplerNearest; }

	/// Informs the resource manager that a new frame starts being rendered.
	ResourceFrameHandle acquireResourceFrame();

	/// Informs the resource manager that a frame has completely finished rendering and all related Vulkan resources can be released.
	void releaseResourceFrame(ResourceFrameHandle frame);

	/// Creates a new descriptor set from the pool and caches it, or returns an existing one for the given cache key.
	template<typename KeyType>
	std::pair<VkDescriptorSet, bool> createDescriptorSet(VkDescriptorSetLayout layout, KeyType&& cacheKey, ResourceFrameHandle resourceFrame) {
	    OVITO_ASSERT(std::find(_activeResourceFrames.begin(), _activeResourceFrames.end(), resourceFrame) != _activeResourceFrames.end());

		// Check if this descriptor set with for the cache key has already been created.
		VkDescriptorSet& descriptorSet = _descriptorSets.lookup(std::forward<KeyType>(cacheKey), resourceFrame);
		if(descriptorSet != VK_NULL_HANDLE)
			return { descriptorSet, false };

		// Otherwise create new descriptor set and store it in the cache.
		descriptorSet = createDescriptorSetImpl(layout);
		return { descriptorSet, true };
	}

	/// Uploads an OVITO DataBuffer to the Vulkan device.
	VkBuffer uploadDataBuffer(const ConstDataBufferPtr& dataBuffer, ResourceFrameHandle resourceFrame, VkBufferUsageFlagBits usage);

	/// Uploads some data to the Vulkan device as a buffer object and caches it.
	template<typename KeyType>
	VkBuffer createCachedBuffer(KeyType&& cacheKey, VkDeviceSize bufferSize, ResourceFrameHandle resourceFrame, VkBufferUsageFlagBits usage, std::function<void(void*)>&& fillMemoryFunc) {
	    OVITO_ASSERT(std::find(_activeResourceFrames.begin(), _activeResourceFrames.end(), resourceFrame) != _activeResourceFrames.end());

		// Check if this OVITO data buffer has already been uploaded to the GPU.
		DataBufferInfo& dataBufferInfo = _dataBuffers.lookup(std::forward<KeyType>(cacheKey), resourceFrame);

		// If not, do it now.
		if(dataBufferInfo.buffer == VK_NULL_HANDLE)
			dataBufferInfo = createCachedBufferImpl(bufferSize, usage, std::move(fillMemoryFunc));

		return dataBufferInfo.buffer;
	}

	/// Uploads an image to the Vulkan device as a texture image.
	VkImageView uploadImage(const QImage& image, ResourceFrameHandle resourceFrame);

	/// Indicates whether the current Vulkan device uses a unified memory architecture, i.e., 
	/// the device-local memory heap is also the CPU-local memory heap. 
	/// On UMA devices, no staging buffers are required.
	bool isUMA() const { return _isUMA; }

	/// Indicates whether the current Vulkan device supports the 'wideLines' feature.
	bool supportsWideLines() const { return _supportsWideLines; }

	/// Indicates whether the current Vulkan device supports the 'multiDrawIndirect' feature.
	bool supportsMultiDrawIndirect() const { return _supportsMultiDrawIndirect; }

	/// Indicates whether the current Vulkan device supports the 'drawIndirectFirstInstance' feature.
	bool supportsDrawIndirectFirstInstance() const { return _supportsDrawIndirectFirstInstance; }

	/// Indicates whether the current Vulkan device supports the 'extendedDynamicState' feature.
	bool supportsExtendedDynamicState() const { return _supportsExtendedDynamicState; }

Q_SIGNALS:

	/// Is emitted when the logical device is lost, meaning that a Vulkan failed with VK_ERROR_DEVICE_LOST.
	void logicalDeviceLost();

	/// Is emitted when the physical device is lost, meaning the creation of the logical device fails with VK_ERROR_DEVICE_LOST.
	void physicalDeviceLost();

	/// Is emitted right before the logical device is going to be destroyed (or was lost) and clients should release their Vulkan resources too.
	void releaseResourcesRequested();

private:

	struct DataBufferInfo {
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
	};

	/// Creates a new descriptor set from the pool.
	VkDescriptorSet createDescriptorSetImpl(VkDescriptorSetLayout layout);

	/// Uploads some data to the Vulkan device as a buffer object.
	DataBufferInfo createCachedBufferImpl(VkDeviceSize bufferSize, VkBufferUsageFlagBits usage, std::function<void(void*)>&& fillMemoryFunc);

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

	/// The command pool used for transferring data buffers and images to GPU memory.
    VkCommandPool _transferCmdPool = VK_NULL_HANDLE;

	/// Fence object used for synchronized data transfers to the GPU.
	VkFence _transferFence = VK_NULL_HANDLE;

	/// The standard texture sampler, which uses nearest interpolation.
	VkSampler _samplerNearest = VK_NULL_HANDLE;

	/// The format to use for the depth-stencil buffer.
    VkFormat _dsFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	/// A host visible memory type index suitable for general use.
    uint32_t _hostVisibleMemIndex;

	/// A device local memory type index suitable for general use.
    uint32_t _deviceLocalMemIndex;

	/// Indicates that this device uses a unified memory architecture, i.e., 
	/// the device-local memory heap is also the CPU-local memory heap. 
	/// On UMA devices, no staging buffers are needed.
	bool _isUMA = false;

	/// Indicates that the current Vulkan device supports the 'wideLines' feature.
	bool _supportsWideLines = false;

	/// Indicates that the current Vulkan device supports the 'multiDrawIndirect' feature.
	bool _supportsMultiDrawIndirect = false;

	/// Indicates that the current Vulkan device supports the 'drawIndirectFirstInstance' feature.
	bool _supportsDrawIndirectFirstInstance = false;

	/// Indicates that the current Vulkan device supports the 'extendedDynamicState' feature.
	bool _supportsExtendedDynamicState = false;

	/// The Vulkan Memory Allocator used for this device.
	VmaAllocator _allocator = VK_NULL_HANDLE;

	/// The Vulkan pipeline cache.
	VkPipelineCache _pipelineCache = VK_NULL_HANDLE;

	/// The pool for creating descriptor sets.
	VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

	/// Keeps track of the data buffers that have been uploaded to the Vulkan device.
	VulkanResourceCache<DataBufferInfo> _dataBuffers;

	/// Keeps track of the descriptor sets that have been allocated.
	VulkanResourceCache<VkDescriptorSet> _descriptorSets;

	struct TextureInfo {
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
	};

	/// Keeps track of the texture images that have been uploaded to the Vulkan device.
	VulkanResourceCache<TextureInfo> _textureImages;

	/// List of frames that are currently being rendered (by the CPU and/or the GPU).
	std::vector<ResourceFrameHandle> _activeResourceFrames;

	/// Counter that keeps track of how many resource frames have been acquired.
	ResourceFrameHandle _nextResourceFrame = 0;

public:

	/// Pointer to optional Vulkan extension function. 
	PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT = nullptr;
};

}	// End of namespace
