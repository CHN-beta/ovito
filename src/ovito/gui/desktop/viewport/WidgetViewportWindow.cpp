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
#include <ovito/gui/desktop/viewport/ViewportMenu.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/DataSet.h>
#include "WidgetViewportWindow.h"

namespace Ovito {

/******************************************************************************
* Returns the global editor registry, which can be used to look up the editor
* class for editable RefTarget class.
******************************************************************************/
WidgetViewportWindow::Registry& WidgetViewportWindow::registry()
{
	static Registry singleton;
	return singleton;
}

/******************************************************************************
* Factory method which creates a new viewport window widget. Depending on the 
* user's settings this can be either a OpenGL or a Vulkan window.
******************************************************************************/
WidgetViewportWindow* WidgetViewportWindow::createViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parent)
{
	// Select the viewport window implementation to use.
	QSettings settings;
	const QMetaObject* viewportImplementation = nullptr;
	for(const QMetaObject* metaType : WidgetViewportWindow::registry()) {
		if(qstrcmp(metaType->className(), "Ovito::OpenGLViewportWindow") == 0) {
			viewportImplementation = metaType;
		}
		else if(qstrcmp(metaType->className(), "Ovito::VulkanViewportWindow") == 0 && settings.value("rendering/selected_graphics_api").toString() == "Vulkan") {
			viewportImplementation = metaType;
			break;
		}
	}

	if(viewportImplementation)
		return dynamic_cast<WidgetViewportWindow*>(viewportImplementation->newInstance(Q_ARG(Viewport*, vp), Q_ARG(ViewportInputManager*, inputManager), Q_ARG(MainWindow*, mainWindow), Q_ARG(QWidget*, parent)));

	return nullptr;
}

/******************************************************************************
* Constructor.
******************************************************************************/
WidgetViewportWindow::WidgetViewportWindow(MainWindowInterface* mainWindow, ViewportInputManager* inputManager, Viewport* vp)
	: ViewportWindowInterface(mainWindow, vp), _inputManager(inputManager)
{
}

/******************************************************************************
* Displays the context menu for this viewport.
******************************************************************************/
void WidgetViewportWindow::showViewportMenu(const QPoint& pos)
{
	// Create the context menu for the viewport.
	ViewportMenu contextMenu(viewport(), widget());

	// Show menu.
	contextMenu.show(pos);
}

/******************************************************************************
* Returns the input manager handling mouse events of the viewport (if any).
******************************************************************************/
ViewportInputManager* WidgetViewportWindow::inputManager() const
{
	return _inputManager.data();
}

/******************************************************************************
* Returns the list of gizmos to render in the viewport.
******************************************************************************/
const std::vector<ViewportGizmo*>& WidgetViewportWindow::viewportGizmos()
{
	return inputManager()->viewportGizmos();
}

/******************************************************************************
* Handles double click events.
******************************************************************************/
void WidgetViewportWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mouseDoubleClickEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse press events.
******************************************************************************/
void WidgetViewportWindow::mousePressEvent(QMouseEvent* event)
{
	viewport()->dataset()->viewportConfig()->setActiveViewport(viewport());

	// Intercept mouse clicks on the viewport caption.
	if(contextMenuArea().contains(ViewportInputMode::getMousePosition(event))) {
		showViewportMenu(event->pos());
		return;
	}

	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mousePressEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse release events.
******************************************************************************/
void WidgetViewportWindow::mouseReleaseEvent(QMouseEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mouseReleaseEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse move events.
******************************************************************************/
void WidgetViewportWindow::mouseMoveEvent(QMouseEvent* event)
{
	if(contextMenuArea().contains(ViewportInputMode::getMousePosition(event)) && !_cursorInContextMenuArea && event->buttons() == Qt::NoButton) {
		_cursorInContextMenuArea = true;
		viewport()->updateViewport();
	}
	else if(!contextMenuArea().contains(ViewportInputMode::getMousePosition(event)) && _cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}

	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->mouseMoveEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles mouse wheel events.
******************************************************************************/
void WidgetViewportWindow::wheelEvent(QWheelEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->wheelEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport mouse event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Is called when the mouse cursor leaves the widget.
******************************************************************************/
void WidgetViewportWindow::leaveEvent(QEvent* event)
{
	if(_cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}
	if(mainWindow())
		mainWindow()->clearStatusBarMessage();
}

/******************************************************************************
* Is called when the widgets looses the input focus.
******************************************************************************/
void WidgetViewportWindow::focusOutEvent(QFocusEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				mode->focusOutEvent(this, event);
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Handles key-press events.
******************************************************************************/
void WidgetViewportWindow::keyPressEvent(QKeyEvent* event)
{
	if(inputManager()) {
		if(ViewportInputMode* mode = inputManager()->activeMode()) {
			try {
				if(mode->keyPressEvent(this, event))
					return; // Do not propagate handled key events to base class.
			}
			catch(const Exception& ex) {
				qWarning() << "Uncaught exception in viewport key-press event handler:";
				ex.logError();
			}
		}
	}
}

/******************************************************************************
* Renders custom GUI elements in the viewport on top of the scene.
******************************************************************************/
void WidgetViewportWindow::renderGui(SceneRenderer* renderer)
{
	if(viewport()->renderPreviewMode()) {
		// Render render frame.
		renderRenderFrame(renderer);
	}
	else {
		// Render orientation tripod.
		renderOrientationIndicator(renderer);
	}

	// Render viewport caption.
	_contextMenuArea = renderViewportTitle(renderer, _cursorInContextMenuArea);
}


}	// End of namespace
