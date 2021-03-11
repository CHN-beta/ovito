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

#include <QVulkanWindow>
#include <QVulkanWindowRenderer>

namespace Ovito {

class VulkanWindowRenderer; // Defined below

/**
 * \brief A viewport window implementation that is based on Vulkan.
 */
class OVITO_VULKANRENDERERGUI_EXPORT VulkanViewportWindow : public QVulkanWindow, public WidgetViewportWindow
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
		return QVulkanWindow::size() * QVulkanWindow::devicePixelRatio();
	}

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() override {
		return QVulkanWindow::size();
	}

	/// Returns the device pixel ratio of the viewport window's canvas.
	virtual qreal devicePixelRatio() override {
		return QVulkanWindow::devicePixelRatio();
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
	virtual QVulkanWindowRenderer* createRenderer() override;

	/// Is called when the draw calls for the next frame are to be added to the command buffer.
	void startNextFrame(VulkanWindowRenderer* renderer);

protected:

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
		QVulkanWindow::keyPressEvent(event);
	}

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
};

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
    QVulkanDeviceFunctions* _deviceFuncs;

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};

}	// End of namespace
