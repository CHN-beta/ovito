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
#include <ovito/opengl/PickingOpenGLSceneRenderer.h>

#include <QOpenGLWidget>

namespace Ovito {

/**
 * \brief The internal render window/widget used by the Viewport class.
 */
class OVITO_OPENGLRENDERERGUI_EXPORT OpenGLViewportWindow : public QOpenGLWidget, public WidgetViewportWindow
{
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE OpenGLViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parentWidget);

	/// Returns the QWidget that is associated with this viewport window.
	virtual QWidget* widget() override { return this; }

    /// \brief Puts an update request event for this window on the event loop.
	virtual void renderLater() override;

	/// \brief Immediately redraws the contents of this window.
	virtual void renderNow() override;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() override;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() override {
		return size() * devicePixelRatio();
	}

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() override {
		return size();
	}

	/// Returns the device pixel ratio of the viewport window's canvas.
	virtual qreal devicePixelRatio() override {
		return QOpenGLWidget::devicePixelRatioF();
	}

	/// Lets the viewport window delete itself.
	/// This is called by the Viewport class destructor.
	virtual void destroyViewportWindow() override {
		deleteLater();
	}

	/// Makes the OpenGL context used by the viewport window for rendering the current context.
	virtual void makeOpenGLContextCurrent() override { makeCurrent(); }

	/// Returns whether the viewport window is currently visible on screen.
	virtual bool isVisible() const override { return QOpenGLWidget::isVisible(); }

	/// Returns the renderer generating an offscreen image of the scene used for object picking.
	PickingOpenGLSceneRenderer* pickingRenderer() const { return _pickingRenderer; }

	/// Determines the object that is located under the given mouse cursor position.
	virtual ViewportPickResult pick(const QPointF& pos) override;

protected:

	/// Is called whenever the widget needs to be painted.
	virtual void paintGL() override;

	/// Is called whenever the GL context needs to be initialized.
	virtual void initializeGL() override;

	/// Is called when the viewport becomes visible.
	virtual void showEvent(QShowEvent* event) override;

	/// Is called when the mouse cursor leaves the widget.
	virtual void leaveEvent(QEvent* event) override { WidgetViewportWindow::leaveEvent(event); }

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
		QOpenGLWidget::keyPressEvent(event);
	}

private:

	/// Renders the contents of the viewport window.
	void renderViewport();

private:

	/// A flag that indicates that a viewport update has been requested.
	bool _updateRequested = false;

	/// This is the renderer of the interactive viewport.
	OORef<OpenGLSceneRenderer> _viewportRenderer;

	/// This renderer generates an offscreen rendering of the scene that allows picking of objects.
	OORef<PickingOpenGLSceneRenderer> _pickingRenderer;
};

}	// End of namespace
