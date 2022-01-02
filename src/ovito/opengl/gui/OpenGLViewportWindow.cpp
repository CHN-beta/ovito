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

#include <ovito/gui/base/GUIBase.h>
#include <ovito/gui/base/mainwin/UserInterface.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/opengl/OpenGLSceneRenderer.h>
#include <ovito/opengl/PickingOpenGLSceneRenderer.h>
#include "OpenGLViewportWindow.h"

namespace Ovito {

OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(OpenGLViewportWindow);

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLViewportWindow::OpenGLViewportWindow(Viewport* vp, ViewportInputManager* inputManager, UserInterface* gui, QWidget* parentWidget) : 
		QOpenGLWidget(parentWidget),
		BaseViewportWindow(gui, inputManager, vp)
{
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);

	// Determine OpenGL vendor string so other parts of the code can decide
	// which OpenGL features are safe to use.
	OpenGLSceneRenderer::determineOpenGLInfo();

	// Create the viewport renderer.
	_viewportRenderer = new OpenGLSceneRenderer(viewport()->dataset());
	_viewportRenderer->setInteractive(true);

	// Create the object picking renderer.
	_pickingRenderer = new PickingOpenGLSceneRenderer(viewport()->dataset());
	_pickingRenderer->setInteractive(true);

	// Make sure the viewport window releases its resources before the application shuts down, e.g.
	// due to a Python script error.
	connect(QCoreApplication::instance(), &QObject::destroyed, this, [this]() { releaseResources(); });
}

/******************************************************************************
* Destructor.
******************************************************************************/
OpenGLViewportWindow::~OpenGLViewportWindow() 
{
	releaseResources();
}

/******************************************************************************
* Releases the renderer resources held by the viewport's surface and picking renderers. 
******************************************************************************/
void OpenGLViewportWindow::releaseResources()
{
	// Release any OpenGL resources held by the viewport renderers.
	if(_viewportRenderer && _viewportRenderer->currentResourceFrame()) {
		makeCurrent();
		OpenGLResourceManager::instance()->releaseResourceFrame(_viewportRenderer->currentResourceFrame());
		_viewportRenderer->setCurrentResourceFrame(0);
	}
	if(_pickingRenderer && _pickingRenderer->currentResourceFrame()) {
		makeCurrent();
		OpenGLResourceManager::instance()->releaseResourceFrame(_pickingRenderer->currentResourceFrame());
		_pickingRenderer->setCurrentResourceFrame(0);
	}
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
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult OpenGLViewportWindow::pick(const QPointF& pos)
{
	ViewportPickResult result;

	// Cannot perform picking while viewport is not visible or currently rendering or when updates are disabled.
	if(isVisible() && !viewport()->isRendering() && !viewport()->dataset()->viewportConfig()->isSuspended() && pickingRenderer()) {
		OpenGLResourceManager::ResourceFrameHandle previousResourceFrame = 0;
		try {
			if(pickingRenderer()->isRefreshRequired()) {
				// Request a new frame from the resource manager for this render pass.
				previousResourceFrame = pickingRenderer()->currentResourceFrame();
				pickingRenderer()->setCurrentResourceFrame(OpenGLResourceManager::instance()->acquireResourceFrame());
				pickingRenderer()->setPrimaryFramebuffer(defaultFramebufferObject());

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

		// Release the resources created by the OpenGL renderer during the last render pass before the current pass.
		if(previousResourceFrame)
			OpenGLResourceManager::instance()->releaseResourceFrame(previousResourceFrame);
	}
	return result;
}

/******************************************************************************
* Handles show events.
******************************************************************************/
void OpenGLViewportWindow::showEvent(QShowEvent* event)
{
	if(!event->spontaneous())
		update();
	QOpenGLWidget::showEvent(event);
}

/******************************************************************************
* Handles hide events.
******************************************************************************/
void OpenGLViewportWindow::hideEvent(QHideEvent* event)
{
	// Release all renderer resources when the window becomes hidden.
	releaseResources();

	QOpenGLWidget::hideEvent(event);
}

/******************************************************************************
* Is called whenever the widget needs to be painted.
******************************************************************************/
void OpenGLViewportWindow::paintGL()
{
	_updateRequested = false;

	// Do nothing if windows has been detached from its viewport.
	if(!viewport() || !viewport()->dataset())
		return;

	OVITO_ASSERT_MSG(!viewport()->isRendering(), "OpenGLViewportWindow::paintGL()", "Recursive viewport repaint detected.");
	OVITO_ASSERT_MSG(!viewport()->dataset()->viewportConfig()->isRendering(), "OpenGLViewportWindow::paintGL()", "Recursive viewport repaint detected.");

	// Do not re-enter rendering function of the same viewport.
	if(viewport()->isRendering())
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
			if(gui())
				gui()->shutdown();
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
			QCoreApplication::exit();
		}
		return;
	}

	// Invalidate picking buffer every time the visible contents of the viewport change.
	_pickingRenderer->reset();

	if(!viewport()->dataset()->viewportConfig()->isSuspended()) {

		// Request a new frame from the resource manager for this render pass.
		OpenGLResourceManager::ResourceFrameHandle previousResourceFrame = _viewportRenderer->currentResourceFrame();
		_viewportRenderer->setCurrentResourceFrame(OpenGLResourceManager::instance()->acquireResourceFrame());
		_viewportRenderer->setPrimaryFramebuffer(defaultFramebufferObject());

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
			ex.appendDetailMessage(openGLReport);

			QCoreApplication::removePostedEvents(nullptr, 0);
			ex.reportError(true);
			if(gui())
				gui()->shutdown();
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
			QCoreApplication::exit();
		}

		// Release the resources created by the OpenGL renderer during the last render pass before the current pass.
		if(previousResourceFrame) {
			OpenGLResourceManager::instance()->releaseResourceFrame(previousResourceFrame);
		}
	}
	else {
		// Make sure viewport gets refreshed as soon as updates are enabled again.
		viewport()->dataset()->viewportConfig()->updateViewports();
	}
}

}	// End of namespace
