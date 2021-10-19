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


#include <ovito/gui/base/GUIBase.h>

namespace Ovito {

/**
 * \brief Abstract interface to the graphical user interface of the application.
 *
 * Note that is is possible to open multiple GUI windows per process.
 */
class OVITO_GUIBASE_EXPORT UserInterface
{
public:

	/// Constructor.
	explicit UserInterface(DataSetContainer& datasetContainer) : _datasetContainer(datasetContainer) {}

	/// Returns the container managing the current dataset.
	DataSetContainer& datasetContainer() { return _datasetContainer; }

	/// Sets the viewport input manager of the user interface.
	void setViewportInputManager(ViewportInputManager* manager) { _viewportInputManager = manager; }

	/// Returns the viewport input manager of the user interface.
	ViewportInputManager* viewportInputManager() const { return _viewportInputManager; }

	/// Gives the active viewport the input focus.
	virtual void setViewportInputFocus() {}
	
	/// Displays a message string in the status bar.
	virtual void showStatusBarMessage(const QString& message, int timeout = 0) {}

	/// Hides any messages currently displayed in the status bar.
	virtual void clearStatusBarMessage() {}

	/// Closes the user interface (and shuts down application if there are no more windows open).
	virtual void shutdown() {}

	/// Returns the manager of the user interface actions.
	ActionManager* actionManager() const { return _actionManager; }

	/// Queries the system's information and graphics capabilities.
	QString generateSystemReport();

	/// Shows the online manual and opens the given help page.
	static void openHelpTopic(const QString& page);

	/// Creates a frame buffer of the requested size for rendering and displays it in a window in the user interface.
	virtual std::shared_ptr<FrameBuffer> createAndShowFrameBuffer(int width, int height);

protected:

	/// Assigns an ActionManager.
	void setActionManager(ActionManager* manager) { _actionManager = manager; }

private:

	/// Container managing the DataSet currently being edited in this user interface.
	DataSetContainer& _datasetContainer;

	/// Viewport input manager of the user interface.
	ViewportInputManager* _viewportInputManager = nullptr;

	/// Actions of the user interface.
	ActionManager* _actionManager = nullptr;
};

}	// End of namespace
