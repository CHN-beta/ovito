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
#include <ovito/core/viewport/ViewportWindowInterface.h>

namespace Ovito {

/**
 * \brief Abstract interface for QWidget-based viewport window implementations.
 */
class OVITO_GUI_EXPORT WidgetViewportWindow : public ViewportWindowInterface
{
public:

	/// Registry for viewport window implementations.
	using Registry = QVarLengthArray<const QMetaObject*, 2>;

	/// Returns the global registry, which allows enumerating all installed viewport window implementations.
	static Registry& registry();

public:

	/// Constructor.
	WidgetViewportWindow(MainWindowInterface* mainWindow, ViewportInputManager* inputManager, Viewport* vp);

	/// Returns the QWidget that is associated with this viewport window.
	virtual QWidget* widget() = 0;

	/// Returns the input manager handling mouse events of the viewport (if any).
	ViewportInputManager* inputManager() const;

	/// Returns the list of gizmos to render in the viewport.
	virtual const std::vector<ViewportGizmo*>& viewportGizmos() override;

	/// Sets the mouse cursor shape for the window. 
	virtual void setCursor(const QCursor& cursor) override { widget()->setCursor(cursor); }

	/// Renders custom GUI elements in the viewport on top of the scene.
	virtual void renderGui(SceneRenderer* renderer) override;

	/// \brief Displays the context menu for the viewport.
	/// \param pos The position in where the context menu should be displayed.
	void showViewportMenu(const QPoint& pos = QPoint(0,0));

	/// Returns the zone in the upper left corner of the viewport where the context menu can be activated by the user.
	const QRectF& contextMenuArea() const { return _contextMenuArea; }

protected:

	/// Is called when the mouse cursor leaves the widget.
	void leaveEvent(QEvent* event);

	/// Handles double click events.
	void mouseDoubleClickEvent(QMouseEvent* event);

	/// Handles mouse press events.
	void mousePressEvent(QMouseEvent* event);

	/// Handles mouse release events.
	void mouseReleaseEvent(QMouseEvent* event);

	/// Handles mouse move events.
	void mouseMoveEvent(QMouseEvent* event);

	/// Handles mouse wheel events.
	void wheelEvent(QWheelEvent* event);

	/// Is called when the widgets looses the input focus.
	void focusOutEvent(QFocusEvent* event);

	/// Handles key-press events.
	void keyPressEvent(QKeyEvent* event);

private:

	/// The input manager handling mouse events of the viewport.
	QPointer<ViewportInputManager> _inputManager;

	/// The zone in the upper left corner of the viewport where
	/// the context menu can be activated by the user.
	QRectF _contextMenuArea;

	/// Indicates that the mouse cursor is currently positioned inside the
	/// viewport area that activates the viewport context menu.
	bool _cursorInContextMenuArea = false;	
};

/// This macro registers a widget-based viewport window implementation.
#define OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(WindowClass) \
	static const int __registration##WindowClass = (Ovito::WidgetViewportWindow::registry().push_back(&WindowClass::staticMetaObject), 0);

}	// End of namespace
