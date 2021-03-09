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
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/app/Application.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/viewport/ViewportMenu.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>
#include <ovito/opengl/PickingOpenGLSceneRenderer.h>
#include "OpenGLViewportWindow.h"

namespace Ovito {

OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(OpenGLViewportWindow);

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLViewportWindow::OpenGLViewportWindow(Viewport* vp, ViewportInputManager* inputManager, MainWindow* mainWindow, QWidget* parentWidget) : 
		QOpenGLWidget(parentWidget),
		WidgetViewportWindow(mainWindow, vp),
		_inputManager(inputManager)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);

	// Determine OpenGL vendor string so other parts of the code can decide
	// which OpenGL features are safe to use.
	OpenGLSceneRenderer::determineOpenGLInfo();

	// Create the viewport renderer.
	// It is shared by all viewports of a dataset.
	for(Viewport* vp : viewport()->dataset()->viewportConfig()->viewports()) {
		if(vp->window() != nullptr) {
			_viewportRenderer = static_cast<OpenGLViewportWindow*>(vp->window())->_viewportRenderer;
			if(_viewportRenderer) break;
		}
	}
	if(!_viewportRenderer) {
		_viewportRenderer = new OpenGLSceneRenderer(viewport()->dataset());
		_viewportRenderer->setInteractive(true);
	}

	// Create the object picking renderer.
	_pickingRenderer = new PickingOpenGLSceneRenderer(viewport()->dataset());
	_pickingRenderer->setInteractive(true);
}

/******************************************************************************
* Returns the input manager handling mouse events of the viewport (if any).
******************************************************************************/
ViewportInputManager* OpenGLViewportWindow::inputManager() const
{
	return _inputManager.data();
}

/******************************************************************************
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void OpenGLViewportWindow::renderLater()
{
	_updateRequested = true;
	update();
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void OpenGLViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
		OVITO_ASSERT_MSG(!viewport()->isRendering(), "OpenGLViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		OVITO_ASSERT_MSG(!viewport()->dataset()->viewportConfig()->isRendering(), "OpenGLViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		repaint();
	}
}

/******************************************************************************
* Displays the context menu for this viewport.
******************************************************************************/
void OpenGLViewportWindow::showViewportMenu(const QPoint& pos)
{
	// Create the context menu for the viewport.
	ViewportMenu contextMenu(viewport(), this);

	// Show menu.
	contextMenu.show(pos);
}

/******************************************************************************
* Returns the list of gizmos to render in the viewport.
******************************************************************************/
const std::vector<ViewportGizmo*>& OpenGLViewportWindow::viewportGizmos()
{
	return inputManager()->viewportGizmos();
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult OpenGLViewportWindow::pick(const QPointF& pos)
{
	ViewportPickResult result;

	// Cannot perform picking while viewport is not visible or currently rendering or when updates are disabled.
	if(isVisible() && !viewport()->isRendering() && !viewport()->dataset()->viewportConfig()->isSuspended() && pickingRenderer()) {
		try {
			if(pickingRenderer()->isRefreshRequired()) {
				// Let the viewport do the actual rendering work.
				viewport()->renderInteractive(pickingRenderer());
			}

			// Query which object is located at the given window position.
			const QPoint pixelPos = (pos * devicePixelRatio()).toPoint();
			const PickingOpenGLSceneRenderer::ObjectRecord* objInfo;
			quint32 subobjectId;
			std::tie(objInfo, subobjectId) = pickingRenderer()->objectAtLocation(pixelPos);
			if(objInfo) {
				result.setPipelineNode(objInfo->objectNode);
				result.setPickInfo(objInfo->pickInfo);
				result.setHitLocation(pickingRenderer()->worldPositionFromLocation(pixelPos));
				result.setSubobjectId(subobjectId);
			}
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
	return result;
}

/******************************************************************************
* Is called whenever the GL context needs to be initialized.
******************************************************************************/
void OpenGLViewportWindow::initializeGL()
{
}

/******************************************************************************
* Is called whenever the widget needs to be painted.
******************************************************************************/
void OpenGLViewportWindow::paintGL()
{
	OVITO_ASSERT_MSG(!viewport()->isRendering(), "OpenGLViewportWindow::paintGL()", "Recursive viewport repaint detected.");
	OVITO_ASSERT_MSG(!viewport()->dataset()->viewportConfig()->isRendering(), "OpenGLViewportWindow::paintGL()", "Recursive viewport repaint detected.");
	renderNow();
}

/******************************************************************************
* Handles the show events.
******************************************************************************/
void OpenGLViewportWindow::showEvent(QShowEvent* event)
{
	if(!event->spontaneous())
		update();
}

/******************************************************************************
* Handles double click events.
******************************************************************************/
void OpenGLViewportWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
void OpenGLViewportWindow::mousePressEvent(QMouseEvent* event)
{
	viewport()->dataset()->viewportConfig()->setActiveViewport(viewport());

	// Intercept mouse clicks on the viewport caption.
	if(_contextMenuArea.contains(ViewportInputMode::getMousePosition(event))) {
		showViewportMenu(event->pos());
		return;
	}

	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
void OpenGLViewportWindow::mouseReleaseEvent(QMouseEvent* event)
{
	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
void OpenGLViewportWindow::mouseMoveEvent(QMouseEvent* event)
{
	if(_contextMenuArea.contains(ViewportInputMode::getMousePosition(event)) && !_cursorInContextMenuArea) {
		_cursorInContextMenuArea = true;
		viewport()->updateViewport();
	}
	else if(!_contextMenuArea.contains(ViewportInputMode::getMousePosition(event)) && _cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}

	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
void OpenGLViewportWindow::wheelEvent(QWheelEvent* event)
{
	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
void OpenGLViewportWindow::leaveEvent(QEvent* event)
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
void OpenGLViewportWindow::focusOutEvent(QFocusEvent* event)
{
	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
void OpenGLViewportWindow::keyPressEvent(QKeyEvent* event)
{
	if(_inputManager) {
		if(ViewportInputMode* mode = _inputManager->activeMode()) {
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
	QOpenGLWidget::keyPressEvent(event);
}

/******************************************************************************
* Renders custom GUI elements in the viewport on top of the scene.
******************************************************************************/
void OpenGLViewportWindow::renderGui(SceneRenderer* renderer)
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

/******************************************************************************
* Immediately redraws the contents of this window.
******************************************************************************/
void OpenGLViewportWindow::renderNow()
{
	_updateRequested = false;

	// Do not re-enter rendering function of the same viewport.
	if(!viewport() || viewport()->isRendering())
		return;

	QSurfaceFormat format = context()->format();
	// OpenGL in a VirtualBox machine Windows guest reports "2.1 Chromium 1.9" as version string, which is
	// not correctly parsed by Qt. We have to workaround this.
	if(OpenGLSceneRenderer::openGLVersion().startsWith("2.1 ")) {
		format.setMajorVersion(2);
		format.setMinorVersion(1);
	}

	if(format.majorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MAJOR || (format.majorVersion() == OVITO_OPENGL_MINIMUM_VERSION_MAJOR && format.minorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MINOR)) {
		// Avoid infinite recursion.
		static bool errorMessageShown = false;
		if(!errorMessageShown) {
			errorMessageShown = true;
			viewport()->dataset()->viewportConfig()->suspendViewportUpdates();
			Exception ex(tr(
					"The OpenGL graphics driver installed on this system does not support OpenGL version %6.%7 or newer.\n\n"
					"Ovito requires modern graphics hardware and up-to-date graphics drivers to display 3D content. Your current system configuration is not compatible with Ovito and the application will quit now.\n\n"
					"To avoid this error, please install the newest graphics driver of the hardware vendor or, if necessary, consider replacing your graphics card with a newer model.\n\n"
					"The installed OpenGL graphics driver reports the following information:\n\n"
					"OpenGL vendor: %1\n"
					"OpenGL renderer: %2\n"
					"OpenGL version: %3.%4 (%5)\n\n"
					"Ovito requires at least OpenGL version %6.%7.")
					.arg(QString(OpenGLSceneRenderer::openGLVendor()))
					.arg(QString(OpenGLSceneRenderer::openGLRenderer()))
					.arg(format.majorVersion())
					.arg(format.minorVersion())
					.arg(QString(OpenGLSceneRenderer::openGLVersion()))
					.arg(OVITO_OPENGL_MINIMUM_VERSION_MAJOR)
					.arg(OVITO_OPENGL_MINIMUM_VERSION_MINOR)
				);
			QCoreApplication::removePostedEvents(nullptr, 0);
			if(QWidget* parentWindow = window())
				parentWindow->close();
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
			QCoreApplication::exit();
		}
		return;
	}

	// Invalidate picking buffer every time the visible contents of the viewport change.
	_pickingRenderer->reset();

	if(!viewport()->dataset()->viewportConfig()->isSuspended()) {
		try {
			// Let the Viewport class do the actual rendering work.
			viewport()->renderInteractive(_viewportRenderer);
		}
		catch(Exception& ex) {
			if(ex.context() == nullptr) ex.setContext(viewport()->dataset());
			ex.prependGeneralMessage(tr("An unexpected error occurred while rendering the viewport contents. The program will quit."));
			viewport()->dataset()->viewportConfig()->suspendViewportUpdates();

			QString openGLReport;
			QTextStream stream(&openGLReport, QIODevice::WriteOnly | QIODevice::Text);
			stream << "OpenGL version: " << context()->format().majorVersion() << QStringLiteral(".") << context()->format().minorVersion() << "\n";
			stream << "OpenGL profile: " << (context()->format().profile() == QSurfaceFormat::CoreProfile ? "core" : (context()->format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "none")) << "\n";
			stream << "OpenGL vendor: " << QString(OpenGLSceneRenderer::openGLVendor()) << "\n";
			stream << "OpenGL renderer: " << QString(OpenGLSceneRenderer::openGLRenderer()) << "\n";
			stream << "OpenGL version string: " << QString(OpenGLSceneRenderer::openGLVersion()) << "\n";
			stream << "OpenGL shading language: " << QString(OpenGLSceneRenderer::openGLSLVersion()) << "\n";
			stream << "OpenGL shader programs: " << QOpenGLShaderProgram::hasOpenGLShaderPrograms() << "\n";
			stream << "OpenGL geometry shaders: " << QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry, context()) << "\n";
			stream << "Using geometry shaders: " << OpenGLSceneRenderer::geometryShadersEnabled() << "\n";
			ex.appendDetailMessage(openGLReport);

			QCoreApplication::removePostedEvents(nullptr, 0);
			if(QWidget* parentWindow = window())
				parentWindow->close();
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
			QCoreApplication::exit();
		}
	}
	else {
		// Make sure viewport gets refreshed as soon as updates are enabled again.
		viewport()->dataset()->viewportConfig()->updateViewports();
	}
}

}	// End of namespace
