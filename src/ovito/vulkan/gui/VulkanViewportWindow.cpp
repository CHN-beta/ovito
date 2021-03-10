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

namespace Ovito {

OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(VulkanViewportWindow);

/******************************************************************************
* Constructor.
******************************************************************************/
VulkanViewportWindow::VulkanViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parentWidget) : 
		QObject(parentWidget),
		WidgetViewportWindow(mainWindow, inputManager, vp)
{
//	setMouseTracking(true);
//	setFocusPolicy(Qt::StrongFocus);

	_window = new QVulkanWindow();
	_widget = QWidget::createWindowContainer(_window, parentWidget);

	// Create the viewport renderer.
	// It is shared by all viewports of a dataset.
	for(Viewport* vp : viewport()->dataset()->viewportConfig()->viewports()) {
		if(VulkanViewportWindow* window = dynamic_cast<VulkanViewportWindow*>(vp->window())) {
			_viewportRenderer = window->_viewportRenderer;
			if(_viewportRenderer) break;
		}
	}
	if(!_viewportRenderer) {
//		_viewportRenderer = new OpenGLSceneRenderer(viewport()->dataset());
//		_viewportRenderer->setInteractive(true);
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
//	update();
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
//		repaint();
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

#if 0
/******************************************************************************
* Handles the show events.
******************************************************************************/
void VulkanViewportWindow::showEvent(QShowEvent* event)
{
	if(!event->spontaneous())
		update();
}
#endif

/******************************************************************************
* Immediately redraws the contents of this window.
******************************************************************************/
void VulkanViewportWindow::renderNow()
{
	_updateRequested = false;

	// Do not re-enter rendering function of the same viewport.
	if(!viewport() || viewport()->isRendering())
		return;
}

}	// End of namespace
