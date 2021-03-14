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
#include <ovito/core/rendering/SceneRenderer.h>
#include "VulkanDevice.h"

namespace Ovito {

/**
 * \brief An Vulkan-based scene renderer. This serves as base class for both the interactive renderer used
 *        by the viewports and the standard output renderer.
 */
class OVITO_VULKANRENDERER_EXPORT VulkanSceneRenderer : public SceneRenderer
{
public:

	/// Defines a metaclass specialization for this renderer class.
	class OOMetaClass : public SceneRenderer::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using SceneRenderer::OOMetaClass::OOMetaClass;

		/// Is called by OVITO to query the class for any information that should be included in the application's system report.
		virtual void querySystemInformation(QTextStream& stream) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(VulkanSceneRenderer, OOMetaClass)

public:

	/// Constructor.
	explicit VulkanSceneRenderer(DataSet* dataset, std::shared_ptr<VulkanDevice> vulkanDevice, int concurrentFrameCount = 2);

	/// Returns the logical Vulkan device used by the renderer.
	const std::shared_ptr<VulkanDevice>& device() const { return _device; }

	/// Returns the Vulkan logical device handle.
	VkDevice logicalDevice() const { return _device->logicalDevice(); }

	/// Returns the device-specific Vulkan function table. 
	QVulkanDeviceFunctions* deviceFunctions() const { return _device->deviceFunctions(); }

	/// This may be called on a renderer before startRender() to control its supersampling level.
	virtual void setAntialiasingHint(int antialiasingLevel) override { _antialiasingLevel = antialiasingLevel; }

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// This method is called after renderFrame() has been called.
	virtual void endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer) override;

	/// Determines if this renderer can share geometry data and other resources with the given other renderer.
	virtual bool sharesResourcesWith(SceneRenderer* otherRenderer) const override;

	/// Renders a 2d polyline or polygon into an interactive viewport.
	virtual void render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed) override;

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	/// This is an internal method used by the PickingVulkanSceneRenderer class to implement the picking mechanism.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount) { return 1; }

	/// Sets the frame buffer background color.
	void setClearColor(const ColorA& color);

	/// Sets the rendering region in the frame buffer.
	void setRenderingViewport(int x, int y, int width, int height);

	/// Clears the frame buffer contents.
	void clearFrameBuffer(bool clearDepthBuffer = true, bool clearStencilBuffer = true);

	/// Temporarily enables/disables the depth test while rendering.
	virtual void setDepthTestEnabled(bool enabled) override;

	/// Activates the special highlight rendering mode.
	virtual void setHighlightMode(int pass) override;

    /// Returns the number of frames that can be potentially active at the same time.
	int concurrentFrameCount() const { return _concurrentFrameCount; }

    /// Returns the current Vulkan swap chain frame index in the range [0, concurrentFrameCount() - 1].
	int currentSwapChainFrame() const { return _currentSwapChainFrame; }

	/// Returns the active Vulkan command buffer.
	VkCommandBuffer currentCommandBuffer() const { return _currentCommandBuffer; }

    /// Sets the current Vulkan swap chain frame index.
	void setCurrentSwapChainFrame(int frame) { _currentSwapChainFrame = frame; }

	/// Sets the default Vulkan render pass to be used by the renderer.
    void setDefaultRenderPass(VkRenderPass renderpass) { _defaultRenderPass = renderpass; }

	/// Sets the active Vulkan command buffer.
	void setCurrentCommandBuffer(VkCommandBuffer cmdBuf) { _currentCommandBuffer = cmdBuf; }

protected:

	/// Returns the supersampling level.
	int antialiasingLevel() const { return _antialiasingLevel; }

	void initResources();

private Q_SLOTS:

    void releaseResources();

private:

	/// The logical Vulkan device used by the renderer.
	std::shared_ptr<VulkanDevice> _device;

	/// Controls the number of sub-pixels to render.
	int _antialiasingLevel = 1;

	/// The number of frames that can be potentially active at the same time.
    int _concurrentFrameCount = 2;

    /// The current Vulkan swap chain frame index.
    uint32_t _currentSwapChainFrame = 0;

	/// The default Vulkan render pass to be used by the renderer.
    VkRenderPass _defaultRenderPass = VK_NULL_HANDLE;

	/// The active command buffer for the current swap chain image.
	VkCommandBuffer _currentCommandBuffer = VK_NULL_HANDLE;

	/// The size of the frame buffer we are rendering into.
	QSize _frameBufferSize;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<ParticlePrimitive>>> _translucentParticles;

	/// List of semi-transparent czlinder primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<CylinderPrimitive>>> _translucentCylinders;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::tuple<AffineTransformation, std::shared_ptr<MeshPrimitive>>> _translucentMeshes;

    QMatrix4x4 m_proj;
    float m_rotation = 0.0f;

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufInfo[VulkanDevice::MAX_CONCURRENT_FRAME_COUNT];
    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[VulkanDevice::MAX_CONCURRENT_FRAME_COUNT];
    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

	/// A 4x4 matrix that can be used to correct for coordinate system differences between OpenGL and Vulkan.
	/// By pre-multiplying the projection matrix with this matrix, applications can
	/// continue to assume that Y is pointing upwards, and can set minDepth and
	/// maxDepth in the viewport to 0 and 1, respectively, without having to do any
	/// further corrections to the vertex Z positions. Geometry from OpenGL
	/// applications can then be used as-is, assuming a rasterization state matching
	/// OpenGL culling and front face settings.
    QMatrix4x4 m_clipCorrect{1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, -1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.5f, 0.5f,
                            0.0f, 0.0f, 0.0f, 1.0f}; 
};

}	// End of namespace
