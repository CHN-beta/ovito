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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>

namespace Ovito::CrystalAnalysis {

/**
 * \brief A properties editor for the MicrostructurePhase class.
 */
class MicrostructurePhaseEditor : public PropertiesEditor
{
	OVITO_CLASS(MicrostructurePhaseEditor)

public:

	/// Default constructor.
	Q_INVOKABLE MicrostructurePhaseEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Is called when the user has double-clicked on one of the entries in the list widget.
	void onDoubleClickBurgersFamily(const QModelIndex& index);

private:

	RefTargetListParameterUI* familiesListUI;
};

}	// End of namespace
