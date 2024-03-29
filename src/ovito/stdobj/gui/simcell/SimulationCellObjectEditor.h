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


#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>

namespace Ovito::StdObj {

/**
 * \brief A properties editor for the SimulationCellObject class.
 */
class SimulationCellEditor : public PropertiesEditor
{
	OVITO_CLASS(SimulationCellEditor)

public:

	/// Default constructor.
	Q_INVOKABLE SimulationCellEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Updates the values displayed in the editor panel.
	void updateSimulationBoxSize();

private:

	BooleanParameterUI* _pbczPUI;
	QLineEdit* _boxSizeFields[3];
	QLineEdit* _cellVectorFields[4][3];
};

}	// End of namespace
