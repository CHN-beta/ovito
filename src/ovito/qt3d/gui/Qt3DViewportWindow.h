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
#include <ovito/gui/base/viewport/BaseViewportWindow.h>
#include <ovito/qt3d/Qt3DSceneRenderer.h>

#include <Qt3DWindow>

namespace Ovito {

/**
 * \brief A viewport window implementation that is based on Qt 3D.
 */
class OVITO_QT3DRENDERERGUI_EXPORT Qt3DViewportWindow : public Qt3DExtras::Qt3DWindow, public BaseViewportWindow
{
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE Qt3DViewportWindow(Viewport* vp, ViewportInputManager* inputManager, UserInterface* gui, QWidget* parentWidget);

	/// Returns the QWidget that is associated with this viewport window.
	virtual QWidget* widget() override { return _widget; }

	/// Returns the interactive scene renderer used by the viewport window to render the graphics.
	virtual SceneRenderer* sceneRenderer() const override { return _viewportRenderer; }

    /// Puts an update request for this window in the event loop.
	virtual void renderLater() override;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() override;

	/// Sets the mouse cursor shape for the window. 
	virtual void setCursor(const QCursor& cursor) override { QWindow::setCursor(cursor); }

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

	/// Returns the current position of the mouse cursor relative to the viewport window.
	virtual QPoint getCurrentMousePos() override { return _widget->mapFromGlobal(QCursor::pos()); }

	/// Returns whether the viewport window is currently visible on screen.
	virtual bool isVisible() const override { return _widget->isVisible(); }

	/// Determines the object that is located under the given mouse cursor position.
	virtual ViewportPickResult pick(const QPointF& pos) override;

protected:

	/// Handles double click events.
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override { BaseViewportWindow::mouseDoubleClickEvent(event); }

	/// Handles mouse press events.
	virtual void mousePressEvent(QMouseEvent* event) override { BaseViewportWindow::mousePressEvent(event); }

	/// Handles mouse release events.
	virtual void mouseReleaseEvent(QMouseEvent* event) override { BaseViewportWindow::mouseReleaseEvent(event); }

	/// Handles mouse move events.
	virtual void mouseMoveEvent(QMouseEvent* event) override { BaseViewportWindow::mouseMoveEvent(event); }

	/// Handles mouse wheel events.
	virtual void wheelEvent(QWheelEvent* event) override { BaseViewportWindow::wheelEvent(event); }

	/// Is called when the widgets looses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override { BaseViewportWindow::focusOutEvent(event); }

	/// Handles key-press events.
	virtual void keyPressEvent(QKeyEvent* event) override { 
		BaseViewportWindow::keyPressEvent(event); 
		QWindow::keyPressEvent(event);
	}

private:

	/// The container widget created for the QWindow.
	QWidget* _widget;

	/// This is the renderer of the interactive viewport.
	OORef<Qt3DSceneRenderer> _viewportRenderer;

	/// This renderer generates an offscreen rendering of the scene that allows picking of objects.
//	OORef<PickingVulkanSceneRenderer> _pickingRenderer;

	/// A flag that indicates that a viewport update has been requested.
	bool _updateRequested = false;
};

}	// End of namespace
