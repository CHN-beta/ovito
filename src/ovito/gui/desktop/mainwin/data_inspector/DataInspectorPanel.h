////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/core/oo/RefTargetListener.h>
#include "DataInspectionApplet.h"

namespace Ovito {

/**
 * The data inspection panel.
 */
class OVITO_GUI_EXPORT DataInspectorPanel : public QWidget
{
	Q_OBJECT

public:

	/// \brief Constructor.
	DataInspectorPanel(MainWindow* mainWindow);

	/// Selects a specific data object in the data inspector.
	bool selectDataObject(PipelineObject* dataSource, const QString& objectIdentifierHint, const QVariant& modeHint);

public Q_SLOTS:

	/// Hides the inspector panel.
	void collapse();

	/// Shows the inspector panel.
	void open();

	/// Expands or collapses the panel depending on the current state.
	void toggle() { onTabBarClicked(-1); }

protected Q_SLOTS:

	/// This is called whenever the scene node selection has changed.
	void onSceneSelectionChanged();

	/// This is called whenever the selected scene node sends an event.
	void onSceneNodeNotificationEvent(RefTarget* source, const ReferenceEvent& event);

	/// Is emitted whenever the scene of the current dataset has been changed and is being made ready for rendering.
	void onScenePreparationBegin();

	/// Is called whenever the scene became ready for rendering.
	void onScenePreparationEnd();

	/// Updates the contents displayed in the data inpector.
	void updateInspector();

	/// Is called when the user clicked on the tab bar.
	void onTabBarClicked(int index);

	/// Is called when the user selects a new tab.
	void onCurrentTabChanged(int index);

	/// Is called whenever the user has switched to a different page of the inspector.
	void onCurrentPageChanged(int index);

protected:

	virtual void mouseReleaseEvent(QMouseEvent* event) override {
		if(event->button() == Qt::LeftButton && ViewportInputMode::getMousePosition(event).y() < _tabBar->height()) {
			toggle();
			event->accept();
		}
		QWidget::mouseReleaseEvent(event);
	}

	void resizeEvent(QResizeEvent* event) override;

	/// Handles timer events for this object.
	virtual void timerEvent(QTimerEvent* event) override;

private:

	/// Updates the list of visible tabs.
	void updateTabs(const DataCollection* dataCollection);

	/// Returns the dataset container this panel is associated with.
	GuiDataSetContainer& datasetContainer() const { return _mainWindow->datasetContainer(); }

private:

	/// Pointer to the main window this inspector panel is part of.
	MainWindow* _mainWindow;

	/// The list of all installed data inspection applets.
	std::vector<OORef<DataInspectionApplet>> _applets;

	/// This maps applet indices to active tab indices.
	std::vector<int> _appletsToTabs;

	/// This maps tab indices to applets.
	std::vector<int> _tabsToApplets;

	/// The tab display.
	QTabBar* _tabBar;

	/// The container for the applet widgets.
	QStackedWidget* _appletContainer;

	/// Listenes to messages from the currently selected object node.
	RefTargetListener<PipelineSceneNode> _selectedNodeListener;

	/// This timer is used to play the activity animation with some delay.
	QBasicTimer _activityDelayTimer;

	/// Animation shown in the title bar to indicate process.
	QMovie _waitingForSceneAnim;

	/// UI element indicating that we are waiting for computations to complete.
	QLabel* _waitingForSceneIndicator;

	/// UI element for opening/closing the inspector panel.
	QPushButton* _expandCollapseButton;

	// The icon for the expand button state.
	QIcon _expandIcon{":/guibase/actions/modify/modifier_move_up.bw.svg"};

	// The icon for the collapse button state.
	QIcon _collapseIcon{":/guibase/actions/modify/modifier_move_down.bw.svg"};

	/// The active page of the inspector.
	int _activeAppletIndex = -1;

	/// Indicates whether the inspector panel is currently open or collapsed.
	bool _inspectorActive = false;
};

}	// End of namespace
