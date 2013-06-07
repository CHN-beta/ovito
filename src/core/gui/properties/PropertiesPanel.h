///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OVITO_PROPERTIES_PANEL_H
#define __OVITO_PROPERTIES_PANEL_H

#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/gui/properties/PropertiesEditor.h>
#include <core/gui/widgets/RolloutContainer.h>

namespace Ovito {

/******************************************************************************
* This panel lets the user edit the properties of some RefTarget derived object.
******************************************************************************/
class PropertiesPanel : public RolloutContainer
{
	Q_OBJECT
	
public:
	
	/// Constructs the panel.
	PropertiesPanel(QWidget* parent);

	/// Destructs the panel.
	virtual ~PropertiesPanel();
	
	/// Returns the target object being edited in the panel.
	RefTarget* editObject() const;
	
	/// Sets the target object being edited in the panel.
	void setEditObject(RefTarget* newEditObject);

	/// Sets the target object being edited in the panel.
	void setEditObject(const OORef<RefTarget>& newEditObject) { setEditObject(newEditObject.get()); }

	/// Returns the editor that is responsible for the object being edited.
	PropertiesEditor* editor() const { return _editor.get(); }
	
protected:

	/// The editor for the current object.
	OORef<PropertiesEditor> _editor;
};

};

#endif // __OVITO_PROPERTIES_PANEL_H
