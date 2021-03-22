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
#include <ovito/gui/desktop/viewport/WidgetViewportWindow.h>
#include <ovito/gui/desktop/viewport/ViewportMenu.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/gui/base/viewport/ViewportInputManager.h>
#include <ovito/core/viewport/ViewportSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include "ViewportsPanel.h"

namespace Ovito {

/******************************************************************************
* The constructor of the viewports panel class.
******************************************************************************/
ViewportsPanel::ViewportsPanel(MainWindow* mainWindow) : _mainWindow(mainWindow)
{
	// Activate the new viewport layout as soon as a new state file is loaded.
	connect(&mainWindow->datasetContainer(), &DataSetContainer::viewportConfigReplaced, this, &ViewportsPanel::onViewportConfigurationReplaced);
	connect(&mainWindow->datasetContainer(), &DataSetContainer::animationSettingsReplaced, this, &ViewportsPanel::onAnimationSettingsReplaced);

	// Track viewport input mode changes.
	connect(mainWindow->viewportInputManager(), &ViewportInputManager::inputModeChanged, this, &ViewportsPanel::onInputModeChanged);

	// Prevent the viewports from collpasing and disappearing completely. 
	setMinimumSize(40, 40);
}

/******************************************************************************
* Returns the widget that is associated with the given viewport.
******************************************************************************/
QWidget* ViewportsPanel::viewportWidget(Viewport* vp)
{
	if(WidgetViewportWindow* window = static_cast<WidgetViewportWindow*>(vp->window()))
		return window->widget();
	return nullptr;
}

/******************************************************************************
* This is called when a new viewport configuration has been loaded.
******************************************************************************/
void ViewportsPanel::onViewportConfigurationReplaced(ViewportConfiguration* newViewportConfiguration)
{
	disconnect(_activeViewportChangedConnection);
	disconnect(_maximizedViewportChangedConnection);
	_viewportConfig = newViewportConfiguration;
	recreateViewportWindows();
	if(_viewportConfig) {
		// Repaint the viewport borders when another viewport has been activated.
		_activeViewportChangedConnection = connect(_viewportConfig, &ViewportConfiguration::activeViewportChanged, this, (void (ViewportsPanel::*)())&ViewportsPanel::update);
		// Update layout when a viewport has been maximized.
		_maximizedViewportChangedConnection = connect(_viewportConfig, &ViewportConfiguration::maximizedViewportChanged, this, &ViewportsPanel::layoutViewports);
	}
}

/******************************************************************************
* Destroys all viewport windows in the panel and recreates them.
******************************************************************************/
void ViewportsPanel::recreateViewportWindows()
{
	// Delete all existing viewport widgets first.
	for(QWidget* widget : findChildren<QWidget*>())
		delete widget;

	if(_viewportConfig) {
		// Create windows for the new viewports.
		try {
			ViewportInputManager* inputManager = _mainWindow->viewportInputManager();
			for(Viewport* vp : _viewportConfig->viewports()) {
				OVITO_ASSERT(vp->window() == nullptr);
				WidgetViewportWindow* viewportWindow = WidgetViewportWindow::createViewportWindow(vp, inputManager, _mainWindow, this);
				if(!viewportWindow)
					vp->throwException(tr("Failed to create viewport window or there is no realtime graphics implementation available. Please check your OVITO installation and the graphics capabilities of your system."));
				if(_viewportConfig->activeViewport() == vp)
					viewportWindow->widget()->setFocus();
			}
		}
		catch(const Exception& ex) {
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
		}

		// Layout viewport widgets.
		layoutViewports();
	}
}

/******************************************************************************
* This is called when new animation settings have been loaded.
******************************************************************************/
void ViewportsPanel::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_autoKeyModeChangedConnection);
	disconnect(_timeChangeCompleteConnection);
	_animSettings = newAnimationSettings;

	if(newAnimationSettings) {
		_autoKeyModeChangedConnection = connect(newAnimationSettings, &AnimationSettings::autoKeyModeChanged, this, (void (ViewportsPanel::*)())&ViewportsPanel::update);
		_timeChangeCompleteConnection = connect(newAnimationSettings, &AnimationSettings::timeChangeComplete, this, (void (ViewportsPanel::*)())&ViewportsPanel::update);
	}
}

/******************************************************************************
* This is called when the current viewport input mode has changed.
******************************************************************************/
void ViewportsPanel::onInputModeChanged(ViewportInputMode* oldMode, ViewportInputMode* newMode)
{
	disconnect(_activeModeCursorChangedConnection);
	if(newMode) {
		_activeModeCursorChangedConnection = connect(newMode, &ViewportInputMode::curserChanged, this, &ViewportsPanel::onViewportModeCursorChanged);
		onViewportModeCursorChanged(newMode->cursor());
	}
	else onViewportModeCursorChanged(cursor());
}

/******************************************************************************
* This is called when the mouse cursor of the active input mode has changed.
******************************************************************************/
void ViewportsPanel::onViewportModeCursorChanged(const QCursor& cursor)
{
	if(!_viewportConfig) return;

	for(Viewport* vp : _viewportConfig->viewports()) {
		if(ViewportWindowInterface* window = vp->window()) {
			window->setCursor(cursor);
		}
	}
}

/******************************************************************************
* Renders the borders of the viewports.
******************************************************************************/
void ViewportsPanel::paintEvent(QPaintEvent* event)
{
	if(!_viewportConfig || !_animSettings) return;

	// Render border around active viewport.
	Viewport* vp = _viewportConfig->activeViewport();
	if(!vp) return;
	QWidget* vpWidget = viewportWidget(vp);
	if(!vpWidget || vpWidget->isHidden()) return;

	QPainter painter(this);

	// Choose a color for the viewport border.
	Color borderColor;
	if(_animSettings->autoKeyMode())
		borderColor = Viewport::viewportColor(ViewportSettings::COLOR_ANIMATION_MODE);
	else
		borderColor = Viewport::viewportColor(ViewportSettings::COLOR_ACTIVE_VIEWPORT_BORDER);

	painter.setPen((QColor)borderColor);
	QRect rect = vpWidget->geometry();
	rect.adjust(-1, -1, 0, 0);
	painter.drawRect(rect);
	rect.adjust(-1, -1, 1, 1);
	painter.drawRect(rect);
}

/******************************************************************************
* Handles size event for the window.
* Does the actual calculation of its children's positions and sizes.
******************************************************************************/
void ViewportsPanel::resizeEvent(QResizeEvent* event)
{
	layoutViewports();
}

/******************************************************************************
* Performs the layout of the viewport windows.
******************************************************************************/
void ViewportsPanel::layoutViewports()
{
	if(!_viewportConfig) return;
	const auto& viewports = _viewportConfig->viewports();
	Viewport* maximizedViewport = _viewportConfig->maximizedViewport();

	// Count the number of visible windows.
	int nvisible = 0;
	for(Viewport* viewport : viewports) {
		QWidget* vpWidget = viewportWidget(viewport);
		if(!vpWidget) continue;
		if(maximizedViewport == nullptr || maximizedViewport == viewport)
			nvisible++;
		else
			vpWidget->setVisible(false);
	}
	if(nvisible == 0) return;

	// Compute number of rows/columns
	int rows = (int)(sqrt((double)nvisible) + 0.5);
	int columns = (nvisible+rows-1) / rows;

	// Get client rect.
	QRect clientRect = rect();

	// Position items.
	int count = 0;
	bool needsRepaint = false;
	for(Viewport* viewport : viewports) {
		QWidget* vpWidget = viewportWidget(viewport);
		if(!vpWidget) continue;
		if(maximizedViewport != nullptr && maximizedViewport != viewport)
			continue;

		int x = count%columns;
		int y = count/columns;
		QRect rect(clientRect.topLeft(), QSize(0,0));
		rect.translate(clientRect.width() * x / columns, clientRect.height() * y / rows);
		rect.setWidth((clientRect.width() * (x+1) / columns) - rect.x());
		rect.setHeight((clientRect.height() * (y+1) / rows) - rect.y());
		rect.adjust(2,2,-2,-2);

		if(vpWidget->geometry() != rect) {
			vpWidget->setGeometry(rect);
			needsRepaint = true;
		}
		vpWidget->setVisible(true);
		count++;
	}

	if(needsRepaint)
		update();
}

/******************************************************************************
* Handles keyboard input for the viewport windows.
******************************************************************************/
bool ViewportsPanel::onKeyShortcut(QKeyEvent* event)
{
	// Suppress viewport navigation shortcuts when a list/table widget has the focus.
	QWidget* focusWidget = _mainWindow->focusWidget();
	if(qobject_cast<QAbstractItemView*>(focusWidget))
		return false;

	// Get the viewport the input pertains to.
	Viewport* vp = _viewportConfig ? _viewportConfig->activeViewport() : nullptr;
	if(!vp) return false;

	qreal delta = 1.0;
	if(event->key() == Qt::Key_Left) {
		if(!(event->modifiers() & Qt::ShiftModifier))
			_mainWindow->viewportInputManager()->orbitMode()->discreteStep(vp->window(), QPointF(-delta, 0));
		else
			_mainWindow->viewportInputManager()->panMode()->discreteStep(vp->window(), QPointF(-delta, 0));
		return true;
	}
	else if(event->key() == Qt::Key_Right) {
		if(!(event->modifiers() & Qt::ShiftModifier))
			_mainWindow->viewportInputManager()->orbitMode()->discreteStep(vp->window(), QPointF(delta, 0));
		else
			_mainWindow->viewportInputManager()->panMode()->discreteStep(vp->window(), QPointF(delta, 0));
		return true;
	}
	else if(event->key() == Qt::Key_Up) {
		if(!(event->modifiers() & Qt::ShiftModifier))
			_mainWindow->viewportInputManager()->orbitMode()->discreteStep(vp->window(), QPointF(0, -delta));
		else
			_mainWindow->viewportInputManager()->panMode()->discreteStep(vp->window(), QPointF(0, -delta));
		return true;
	}
	else if(event->key() == Qt::Key_Down) {
		if(!(event->modifiers() & Qt::ShiftModifier))
			_mainWindow->viewportInputManager()->orbitMode()->discreteStep(vp->window(), QPointF(0, delta));
		else
			_mainWindow->viewportInputManager()->panMode()->discreteStep(vp->window(), QPointF(0, delta));
		return true;
	}
	else if(event->matches(QKeySequence::ZoomIn)) {
		_mainWindow->viewportInputManager()->zoomMode()->zoom(vp, 50);
		return true;
	}
	else if(event->matches(QKeySequence::ZoomOut)) {
		_mainWindow->viewportInputManager()->zoomMode()->zoom(vp, -50);
		return true;
	}
	return false;
}

}	// End of namespace
