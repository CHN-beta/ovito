////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 OVITO GmbH, Germany
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
#include <ovito/core/viewport/Viewport.h>
#include "ViewportWindow.h"

namespace Ovito {

/**
 * \brief The context menu of the viewports.
 */
class ViewportMenu : public QMenu
{
	Q_OBJECT

public:

	/// Initializes the menu.
	ViewportMenu(ViewportWindow* vpWindow);

	/// Displays the menu.
	void show(const QPoint& pos);

private Q_SLOTS:

	void onRenderPreviewMode(bool checked);
	void onShowGrid(bool checked);
	void onConstrainRotation(bool checked);
	void onShowViewTypeMenu();
	void onViewType(QAction* action);
	void onAdjustView();
	void onViewNode(QAction* action);
	void onCreateCamera();

private:

	/// The viewport this menu belongs to.
	Viewport* _viewport;

	/// The window this menu is shown in.
	ViewportWindow* _vpWindow;

	/// The view type sub-menu.
	QMenu* _viewTypeMenu;
};

}	// End of namespace
