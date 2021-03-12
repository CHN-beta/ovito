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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/viewport/WidgetViewportWindow.h>
#include <ovito/vulkan/VulkanSceneRenderer.h>

#include <QWindow>

namespace Ovito {

/**
 * \brief A viewport window implementation that is based on Vulkan.
 */
class OVITO_VULKANRENDERERGUI_EXPORT VulkanViewportWindow : public QWindow, public WidgetViewportWindow
{
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE VulkanViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parentWidget);

	/// Returns the QWidget that is associated with this viewport window.
	virtual QWidget* widget() override { return _widget; }

    /// \brief Puts an update request event for this window on the event loop.
	virtual void renderLater() override;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() override;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() override {
		return QWindow::size() * QWindow::devicePixelRatio();
	}

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() override {
		return QWindow::size();
	}

	/// Returns the device pixel ratio of the viewport window's canvas.
	virtual qreal devicePixelRatio() override {
		return QWindow::devicePixelRatio();
	}

	/// Lets the viewport window delete itself.
	/// This is called by the Viewport class destructor.
	virtual void destroyViewportWindow() override {
		_widget->deleteLater();
		this->deleteLater();
	}

	/// Returns whether the viewport window is currently visible on screen.
	virtual bool isVisible() const override { return _widget->isVisible(); }

	/// Returns the renderer generating an offscreen image of the scene used for object picking.
//	PickingOpenGLSceneRenderer* pickingRenderer() const { return _pickingRenderer; }

	/// Determines the object that is located under the given mouse cursor position.
	virtual ViewportPickResult pick(const QPointF& pos) override;

	/// Creates a new instance of the QVulkanWindowRenderer for this window.
//	virtual QVulkanWindowRenderer* createRenderer() override;

    void initResources();
    void initSwapChainResources();
    void releaseSwapChainResources() {}
    void releaseResources();
	void physicalDeviceLost() {}
    void logicalDeviceLost() {}

	/// Is called when the draw calls for the next frame are to be added to the command buffer.
	void startNextFrame();

	/// Sets the preferred \a formats of the swapchain.
	void setPreferredColorFormats(const QVector<VkFormat>& formats);

    /// Returns a host visible memory type index suitable for general use.
    /// The returned memory type will be both host visible and coherent. In addition, it will also be cached, if possible.
	uint32_t hostVisibleMemoryIndex() const { return _hostVisibleMemIndex; }

	/// Returns a typical render pass with one sub-pass.
	VkRenderPass defaultRenderPass() const { return _defaultRenderPass; }

	/// Returns the color buffer format used by the swapchain.
	VkFormat colorFormat() const { return _colorFormat; }

    /// Returns the image size of the swapchain.
    /// This usually matches the size of the window, but may also differ in case vkGetPhysicalDeviceSurfaceCapabilitiesKHR reports a fixed size.
	QSize swapChainImageSize() const { return _swapChainImageSize; }

	/// Returns the current sample count as a \c VkSampleCountFlagBits value.
    /// When targeting the default render target, the \c rasterizationSamples field
    /// of \c VkPipelineMultisampleStateCreateInfo must be set to this value.
	VkSampleCountFlagBits sampleCountFlagBits() const { return _sampleCount; }

    /// Returns the current frame index in the range [0, concurrentFrameCount() - 1].
	int currentFrame() const {
    	if(!_framePending)
        	qWarning("VulkanViewportWindow: Attempted to call currentFrame() without an active frame");
    	return _currentFrame; 
	}

    /// Returns the number of frames that can be potentially active at the same time.
	int concurrentFrameCount() const { return _frameLag; }	

	//// Returns The active command buffer for the current swap chain image.
	VkCommandBuffer currentCommandBuffer() const {
		if(!_framePending) {
			qWarning("VulkanViewportWindow: Attempted to call currentCommandBuffer() without an active frame");
			return VK_NULL_HANDLE;
		}
		return _imageRes[_currentImage].cmdBuf;
	}

	/// Returns a VkFramebuffer for the current swapchain image using the default render pass.
	VkFramebuffer currentFramebuffer() const {
		if(!_framePending) {
			qWarning("VulkanViewportWindow: Attempted to call currentFramebuffer() without an active frame");
			return VK_NULL_HANDLE;
		}
		return _imageRes[_currentImage].fb;
	}	

	/// Returns the Vulkan logical device handle.
	VkDevice logicalDevice() const { return _device->logicalDevice(); }

	/// Returns the device-specific Vulkan function table. 
	QVulkanDeviceFunctions* deviceFunctions() const { return _device->deviceFunctions(); }

protected:

	/// Handles events sent to the window by the system.
	virtual bool event(QEvent* e) override;

	/// Is called by the window system whenever an area of the window is invalidated, 
	/// for example due to the exposure in the windowing system changing
	virtual void exposeEvent(QExposeEvent* event) override;

	/// Handles double click events.
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override { WidgetViewportWindow::mouseDoubleClickEvent(event); }

	/// Handles mouse press events.
	virtual void mousePressEvent(QMouseEvent* event) override { WidgetViewportWindow::mousePressEvent(event); }

	/// Handles mouse release events.
	virtual void mouseReleaseEvent(QMouseEvent* event) override { WidgetViewportWindow::mouseReleaseEvent(event); }

	/// Handles mouse move events.
	virtual void mouseMoveEvent(QMouseEvent* event) override { WidgetViewportWindow::mouseMoveEvent(event); }

	/// Handles mouse wheel events.
	virtual void wheelEvent(QWheelEvent* event) override { WidgetViewportWindow::wheelEvent(event); }

	/// Is called when the widgets looses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override { WidgetViewportWindow::focusOutEvent(event); }

	/// Handles key-press events.
	virtual void keyPressEvent(QKeyEvent* event) override { 
		WidgetViewportWindow::keyPressEvent(event); 
		QWindow::keyPressEvent(event);
	}

private Q_SLOTS:

	/// Keeps trying to initialize the Vulkan window surface.
	void ensureStarted();

private:

	/// Initializes the Vulkan objects of the window after it has been exposed for first time.  
	void init();

	/// Creates the default Vulkan render pass.  
	bool createDefaultRenderPass();

	/// Recreates the Vulkan swapchain.  
	void recreateSwapChain();

	/// Releases the resources of the Vulkan swapchain.  
	void releaseSwapChain();

	/// Creates a Vulkan image.
	bool createTransientImage(VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask, VkImage *images, VkDeviceMemory *mem, VkImageView *views, int count);

	/// Handles a Vulkan device that was recently lost.
	bool checkDeviceLost(VkResult err);

	/// Starts rendering a frame.
	void beginFrame();

	/// Finishes rendering a frame.
	void endFrame();

	/// This function must be called exactly once in response to each invocation of the startNextFrame() implementation. 
	void frameReady();

	/// Releases all Vulkan resources.
	void reset();

private:

	/// The container widget created for the QVulkanWindow.
	QWidget* _widget;

	/// A flag that indicates that a viewport update has been requested.
	bool _updateRequested = false;

	/// This is the renderer of the interactive viewport.
	OORef<VulkanSceneRenderer> _viewportRenderer;

	/// This renderer generates an offscreen rendering of the scene that allows picking of objects.
//	OORef<PickingOpenGLSceneRenderer> _pickingRenderer;

    QMatrix4x4 m_proj;
    float m_rotation = 0.0f;

	enum Status {
        StatusUninitialized,
        StatusFail,
        StatusFailRetry,
        StatusDeviceReady,
        StatusReady
    };

    Status _status = StatusUninitialized;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    QVector<VkFormat> _requestedColorFormats;
    VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT;
	std::shared_ptr<VulkanDevice> _device;
    uint32_t _hostVisibleMemIndex;
    uint32_t _deviceLocalMemIndex;
    VkFormat _colorFormat;
    VkColorSpaceKHR _colorSpace;
    VkFormat _dsFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    static const int MAX_SWAPCHAIN_BUFFER_COUNT = 3;
	static const int MAX_CONCURRENT_FRAME_COUNT = 3;
    static const int MAX_FRAME_LAG = MAX_CONCURRENT_FRAME_COUNT;
    VkPresentModeKHR _presentMode = VK_PRESENT_MODE_FIFO_KHR;
    int _swapChainBufferCount = 2;
    int _frameLag = 2;
    QSize _swapChainImageSize;
    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    struct ImageResources {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
        VkFence cmdFence = VK_NULL_HANDLE;
        bool cmdFenceWaitable = false;
        VkFramebuffer fb = VK_NULL_HANDLE;
        VkCommandBuffer presTransCmdBuf = VK_NULL_HANDLE;
        VkImage msaaImage = VK_NULL_HANDLE;
        VkImageView msaaImageView = VK_NULL_HANDLE;
    } _imageRes[MAX_SWAPCHAIN_BUFFER_COUNT];
    VkDeviceMemory _msaaImageMem = VK_NULL_HANDLE;
    uint32_t _currentImage;
    struct FrameResources {
        VkFence fence = VK_NULL_HANDLE;
        bool fenceWaitable = false;
        VkSemaphore imageSem = VK_NULL_HANDLE;
        VkSemaphore drawSem = VK_NULL_HANDLE;
        VkSemaphore presTransSem = VK_NULL_HANDLE;
        bool imageAcquired = false;
        bool imageSemWaitable = false;
    } _frameRes[MAX_FRAME_LAG];
    uint32_t _currentFrame;
    VkRenderPass _defaultRenderPass = VK_NULL_HANDLE;
    VkDeviceMemory _dsMem = VK_NULL_HANDLE;
    VkImage _dsImage = VK_NULL_HANDLE;
    VkImageView _dsView = VK_NULL_HANDLE;
    bool _framePending = false;

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

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufInfo[MAX_CONCURRENT_FRAME_COUNT];
    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[MAX_CONCURRENT_FRAME_COUNT];
    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

#if 0
/**
 * \brief The QVulkanWindowRenderer associated with the Vulkan viewport window.
 */
class VulkanWindowRenderer : public QVulkanWindowRenderer
{
public:

	/// Constructor.
	explicit VulkanWindowRenderer(VulkanViewportWindow* window);

	/// Is called when it is time to create the renderer's graphics resources.
	virtual void initResources() override;

	virtual void initSwapChainResources() override;
    virtual void releaseSwapChainResources() override;
    virtual void releaseResources() override;
	virtual void startNextFrame() override { _window->startNextFrame(this); }

public:

	VulkanViewportWindow* _window;
};
#endif

}	// End of namespace
