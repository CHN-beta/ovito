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

	/// Inherit constructor from base class.
	using ViewportWindowInterface::ViewportWindowInterface;

	/// Returns the QWidget that is associated with this viewport window.
	virtual QWidget* widget() = 0;
};

/// This macro registers a widget-based viewport window implementation.
#define OVITO_REGISTER_VIEWPORT_WINDOW_IMPLEMENTATION(WindowClass) \
	static const int __registration##WindowClass = (Ovito::WidgetViewportWindow::registry().push_back(&WindowClass::staticMetaObject), 0);

}	// End of namespace
