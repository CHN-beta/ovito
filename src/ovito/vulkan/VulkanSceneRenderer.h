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
#include "VulkanContext.h"
#include "VulkanLinePrimitive.h"
#include "VulkanParticlePrimitive.h"
#include "VulkanCylinderPrimitive.h"
#include "VulkanImagePrimitive.h"
#include "VulkanTextPrimitive.h"

namespace Ovito {

/**
 * \brief An Vulkan-based scene renderer. This serves as base class for both the interactive renderer used
 *        by the viewports and the standard output renderer.
 */
class OVITO_VULKANRENDERER_EXPORT VulkanSceneRenderer : public SceneRenderer
{
public:

	/// Defines a metaclass specialization for this renderer class.
	class OVITO_VULKANRENDERER_EXPORT OOMetaClass : public SceneRenderer::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using SceneRenderer::OOMetaClass::OOMetaClass;

		/// Is called by OVITO to query the class for any information that should be included in the application's system report.
		virtual void querySystemInformation(QTextStream& stream, DataSetContainer& container) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(VulkanSceneRenderer, OOMetaClass)

public:

	/// Constructor.
	explicit VulkanSceneRenderer(DataSet* dataset, std::shared_ptr<VulkanContext> vulkanContext, int concurrentFrameCount = 2);

	/// Destructor.
	virtual ~VulkanSceneRenderer();

	/// Returns the logical Vulkan context used by the renderer.
	const std::shared_ptr<VulkanContext>& context() const { return _context; }

	/// Returns the Vulkan logical device handle.
	VkDevice logicalDevice() const { return context()->logicalDevice(); }

	/// Returns the device-specific Vulkan function table. 
	QVulkanDeviceFunctions* deviceFunctions() const { return context()->deviceFunctions(); }

	/// This may be called on a renderer before startRender() to control its supersampling level.
	virtual void setAntialiasingHint(int antialiasingLevel) override { _antialiasingLevel = antialiasingLevel; }

	/// Renders the current animation frame.
	virtual bool renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation) override;

	/// This method is called just before renderFrame() is called.
	virtual void beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp) override;

	/// Determines if this renderer can share geometry data and other resources with the given other renderer.
	virtual bool sharesResourcesWith(SceneRenderer* otherRenderer) const override;

	/// Renders a 2d polyline or polygon into an interactive viewport.
	virtual void render2DPolyline(const Point2* points, int count, const ColorA& color, bool closed) override;

	/// Registers a range of sub-IDs belonging to the current object being rendered.
	/// This is an internal method used by the PickingVulkanSceneRenderer class to implement the picking mechanism.
	virtual quint32 registerSubObjectIDs(quint32 subObjectCount, const ConstDataBufferPtr& indices = {}) { return 1; }

	/// Temporarily enables/disables the depth test while rendering.
	virtual void setDepthTestEnabled(bool enabled) override;

	/// Activates the special highlight rendering mode.
	virtual void setHighlightMode(int pass) override;

    /// Returns the number of frames that can be potentially active at the same time.
	int concurrentFrameCount() const { return _concurrentFrameCount; }

    /// Returns the current Vulkan swap chain frame index in the range [0, concurrentFrameCount() - 1].
	int currentSwapChainFrame() const { return _currentSwapChainFrame; }

	/// Returns the monotonically increasing identifier of the current Vulkan frame being rendered.
	VulkanContext::ResourceFrameHandle currentResourceFrame() const { return _currentResourceFrame; }

	/// Sets monotonically increasing identifier of the current Vulkan frame being rendered.
	void setCurrentResourceFrame(VulkanContext::ResourceFrameHandle frame) { _currentResourceFrame = frame; }

	/// Returns the active Vulkan command buffer.
	VkCommandBuffer currentCommandBuffer() const { return _currentCommandBuffer; }

	/// Sets the active Vulkan command buffer.
	void setCurrentCommandBuffer(VkCommandBuffer cmdBuf) { _currentCommandBuffer = cmdBuf; }

    /// Sets the current Vulkan swap chain frame index.
	void setCurrentSwapChainFrame(int frame) { _currentSwapChainFrame = frame; }

	/// Returns the default Vulkan render pass used by the renderer.
    VkRenderPass defaultRenderPass() const { return _defaultRenderPass; }

	/// Sets the default Vulkan render pass to be used by the renderer.
    void setDefaultRenderPass(VkRenderPass renderpass) { _defaultRenderPass = renderpass; }

	/// Returns the size in pixels of the Vulkan frame buffer we are rendering into.
	const QSize& frameBufferSize() const { return _frameBufferSize; }

	/// Sets the size in pixels of the Vulkan frame buffer we are rendering into.
	void setFrameBufferSize(const QSize& size) { _frameBufferSize = size; }

	/// Returns the sample count used by the current Vulkan target rendering buffer.
    VkSampleCountFlagBits sampleCount() const { return _sampleCount; }

	/// Creates a new line rendering primitive.
	virtual std::shared_ptr<LinePrimitive> createLinePrimitive() override;

	/// Creates a new particle rendering primitive.
	virtual std::shared_ptr<ParticlePrimitive> createParticlePrimitive(ParticlePrimitive::ParticleShape shape, ParticlePrimitive::ShadingMode shadingMode, ParticlePrimitive::RenderingQuality renderingQuality) override;

	/// Creates a new cylinder rendering primitive.
	virtual std::shared_ptr<CylinderPrimitive> createCylinderPrimitive(CylinderPrimitive::Shape shape, CylinderPrimitive::ShadingMode shadingMode, CylinderPrimitive::RenderingQuality renderingQuality) override;

	/// Creates a new image rendering primitive.
	virtual std::shared_ptr<ImagePrimitive> createImagePrimitive() override;

	/// Creates a new text rendering primitive.
	virtual std::shared_ptr<TextPrimitive> createTextPrimitive() override;

	/// Renders a line primitive.
	virtual void renderLines(const std::shared_ptr<LinePrimitive>& primitive) override;

	/// Renders a particles primitive.
	virtual void renderParticles(const std::shared_ptr<ParticlePrimitive>& primitive) override;

	/// Renders the cylinder or arrow primitives.
	virtual void renderCylinders(const std::shared_ptr<CylinderPrimitive>& primitive) override;

	/// Renders an image primitive.
	virtual void renderImage(const std::shared_ptr<ImagePrimitive>& primitive) override;

	/// Renders a text primitive.
	virtual void renderText(const std::shared_ptr<TextPrimitive>& primitive) override;

	/// Returns a 4x4 matrix that can be used to correct for coordinate system differences between OpenGL and Vulkan.
    const Matrix4& clipCorrection() const { return _clipCorrection; } 

	/// Returns the descriptor set layout for the global uniforms buffer.
	VkDescriptorSetLayout globalUniformsDescriptorSetLayout();

	/// Returns the Vulkan descriptor set for the global uniforms structure, which can be bound to a pipeline. 
	VkDescriptorSet getGlobalUniformsDescriptorSet();

protected:

	/// This method is called after the reference counter of this object has reached zero
	/// and before the object is being finally deleted. 
	virtual void aboutToBeDeleted() override;

	/// Returns the supersampling level.
	int antialiasingLevel() const { return _antialiasingLevel; }

protected Q_SLOTS:

	/// Releases all Vulkan resources held by the renderer class.
    virtual void releaseVulkanDeviceResources();

private:

	/// Creates the Vulkan resources needed by this renderer.
	void initResources();

	/// The logical Vulkan device used by the renderer.
	std::shared_ptr<VulkanContext> _context;

	/// Controls the number of sub-pixels to render.
	int _antialiasingLevel = 1;

	/// The number of frames that can be potentially active at the same time.
    int _concurrentFrameCount = 2;

    /// The current Vulkan swap chain frame index.
    uint32_t _currentSwapChainFrame = 0;

	/// Indicates whether depth testing is currently enabled for drawing commands.
	bool _depthTestEnabled = true;

	/// The default Vulkan render pass to be used by the renderer.
    VkRenderPass _defaultRenderPass = VK_NULL_HANDLE;

	/// The active command buffer for the current swap chain image.
	VkCommandBuffer _currentCommandBuffer = VK_NULL_HANDLE;

	/// The sample count used by the current Vulkan target rendering buffer.
    VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT;

	/// The size of the frame buffer we are rendering into.
	QSize _frameBufferSize;

	/// The monotonically increasing identifier of the current Vulkan frame being rendered.
	VulkanContext::ResourceFrameHandle _currentResourceFrame = 0;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::pair<AffineTransformation, std::shared_ptr<ParticlePrimitive>>> _translucentParticles;

	/// List of semi-transparent cylinder primitives collected during the first rendering pass, which need to be rendered during the second pass.
	std::vector<std::pair<AffineTransformation, std::shared_ptr<CylinderPrimitive>>> _translucentCylinders;

	/// List of semi-transparent particles primitives collected during the first rendering pass, which need to be rendered during the second pass.
//	std::vector<std::pair<AffineTransformation, std::shared_ptr<MeshPrimitive>>> _translucentMeshes;

	/// Indicates that the Vulkan resources needed by this renderer have been created.
	bool _resourcesInitialized = false;

	/// Data structure holding the Vulkan pipelines used by the line drawing primitive.
	VulkanLinePrimitive::Pipelines _linePrimitivePipelines;

	/// Data structure holding the Vulkan pipelines used by the particle drawing primitive.
	VulkanParticlePrimitive::Pipelines _particlePrimitivePipelines;

	/// Data structure holding the Vulkan pipelines used by the cylinder drawing primitive.
	VulkanCylinderPrimitive::Pipelines _cylinderPrimitivePipelines;

	/// Data structure holding the Vulkan pipelines used by the image drawing primitive.
	VulkanImagePrimitive::Pipelines _imagePrimitivePipelines;

	/// A 4x4 matrix that can be used to correct for coordinate system differences between OpenGL and Vulkan.
	/// By pre-multiplying the projection matrix with this matrix, applications can
	/// continue to assume that Y is pointing upwards, and can set minDepth and
	/// maxDepth in the viewport to 0 and 1, respectively, without having to do any
	/// further corrections to the vertex Z positions. Geometry from OpenGL
	/// applications can then be used as-is, assuming a rasterization state matching
	/// OpenGL culling and front face settings.
    const Matrix4 _clipCorrection{1.0, 0.0, 0.0, 0.0,
									0.0, -1.0, 0.0, 0.0,
									0.0, 0.0, 0.5, 0.5,
									0.0, 0.0, 0.0, 1.0};

	/// Data structure with some slowly or not varying data, which is made available to all shaders.
	struct GlobalUniforms
	{
		Matrix_4<float> projectionMatrix;
		Matrix_4<float> inverseProjectionMatrix;
		Point_2<float> viewportOrigin; 			// Corner of the current viewport rectangle in window coordinates.
		Vector_2<float> inverseViewportSize;	// One over the width/height of the viewport rectangle in window space.
		float znear, zfar;

		bool operator==(const GlobalUniforms& other) const {
			return projectionMatrix == other.projectionMatrix && viewportOrigin == other.viewportOrigin 
				&& inverseViewportSize == other.inverseViewportSize && znear == other.znear && zfar == other.zfar;
		}
	};

	VkDescriptorSetLayout _globalUniformsDescriptorSetLayout = VK_NULL_HANDLE;

	friend class VulkanTextPrimitive;
	friend class VulkanImagePrimitive;
	friend class VulkanLinePrimitive;
	friend class VulkanParticlePrimitive;
	friend class VulkanCylinderPrimitive;
};

}	// End of namespace
