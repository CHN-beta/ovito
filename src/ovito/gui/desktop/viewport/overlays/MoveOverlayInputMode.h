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
#include <ovito/gui/base/viewport/ViewportInputMode.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>

namespace Ovito {

/**
 * Viewport mouse input mode, which allows the user to interactively move a viewport overlay
 * using the mouse.
 */
class OVITO_GUI_EXPORT MoveOverlayInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	MoveOverlayInputMode(PropertiesEditor* editor);

	/// Called when the viewport input handler becomes the current one. 
	virtual void activated(bool temporary) override;

	/// Called when the viewport input handler no longer is the current one. 
	virtual void deactivated(bool temporary) override;

	/// Handles the mouse down events for a Viewport.
	virtual void mousePressEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindowInterface* vpwin, QMouseEvent* event) override;

	/// Returns the current viewport we are working in.
	Viewport* viewport() const { return _viewport; }

private:

	/// The current viewport we are working in.
	Viewport* _viewport = nullptr;

	/// The properties editor of the viewport overlay being moved.
	PropertiesEditor* _editor;

	/// Mouse position at first click.
	QPointF _startPoint;

	/// The current mouse position.
	QPointF _currentPoint;

	/// The cursor shown when the overlay can be moved.
	QCursor _moveCursor;

	/// The cursor shown when in the wrong viewport.
	QCursor _forbiddenCursor;
};

}	// End of namespace
