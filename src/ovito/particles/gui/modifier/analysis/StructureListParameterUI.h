///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#pragma once


#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <ovito/gui/properties/RefTargetListParameterUI.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * List box that displays the structure types.
 */
class OVITO_PARTICLESGUI_EXPORT StructureListParameterUI : public RefTargetListParameterUI
{
	Q_OBJECT
	OVITO_CLASS(StructureListParameterUI)

public:

	/// Constructor.
	StructureListParameterUI(PropertiesEditor* parentEditor, bool showCheckBoxes = false);

	/// This method is called when a new editable object has been activated.
	virtual void resetUI() override;

protected:

	/// Returns a data item from the list data model.
	virtual QVariant getItemData(RefTarget* target, const QModelIndex& index, int role) override;

	/// Returns the model/view item flags for the given entry.
	virtual Qt::ItemFlags getItemFlags(RefTarget* target, const QModelIndex& index) override;

	/// Sets the role data for the item at index to value.
	virtual bool setItemData(RefTarget* target, const QModelIndex& index, const QVariant& value, int role) override;

	/// Returns the number of columns for the table view.
	virtual int tableColumnCount() override { return 5; }

	/// Returns the header data under the given role for the given RefTarget.
	virtual QVariant getHorizontalHeaderData(int index, int role) override {
		if(role == Qt::DisplayRole) {
			if(index == 0)
				return QVariant();
			else if(index == 1)
				return QVariant::fromValue(tr("Structure"));
			else if(index == 2)
				return QVariant::fromValue(tr("Count"));
			else if(index == 3)
				return QVariant::fromValue(tr("Fraction"));
			else if(index == 4)
				return QVariant::fromValue(tr("Id"));
		}
		return RefTargetListParameterUI::getHorizontalHeaderData(index, role);
	}

	/// Do not open sub-editor for selected structure type.
	virtual void openSubEditor() override {}

protected Q_SLOTS:

	/// Is called when the user has double-clicked on one of the structure types in the list widget.
	void onDoubleClickStructureType(const QModelIndex& index);

private:

	/// Obtains the current structure counts from the pipeline.
	void updateStructureCounts();

	/// Controls whether a check box is shown next to each structure type.
	bool _showCheckBoxes;

	/// The data array containing the number of identified particles of each structure type.
	ConstPropertyPtr _structureCounts;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace