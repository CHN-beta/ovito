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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/StringParameterUI.h>
#include <ovito/gui/desktop/properties/RefTargetListParameterUI.h>
#include "MicrostructurePhaseEditor.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(MicrostructurePhaseEditor);
SET_OVITO_OBJECT_EDITOR(MicrostructurePhase, MicrostructurePhaseEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void MicrostructurePhaseEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Structure type"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	// Derive a custom class from the list parameter UI to
	// give the items a color.
	class CustomRefTargetListParameterUI : public RefTargetListParameterUI {
	public:
		CustomRefTargetListParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& refField)
			: RefTargetListParameterUI(parentEditor, refField, RolloutInsertionParameters(), nullptr) {}
	protected:

		virtual QVariant getItemData(RefTarget* target, const QModelIndex& index, int role) override {
			if(target) {
				if(role == Qt::DisplayRole) {
					if(index.column() == 1)
						return target->objectTitle();
				}
				else if(role == Qt::DecorationRole) {
					if(index.column() == 0)
						return (QColor)static_object_cast<BurgersVectorFamily>(target)->color();
				}
				else if(role == Qt::CheckStateRole) {
					if(index.column() == 0)
						return static_object_cast<BurgersVectorFamily>(target)->enabled() ? Qt::Checked : Qt::Unchecked;
				}
			}
			return {};
		}

		/// Sets the role data for the item at index to value.
		virtual bool setItemData(RefTarget* target, const QModelIndex& index, const QVariant& value, int role) override {
			if(index.column() == 0 && role == Qt::CheckStateRole) {
				if(ElementType* stype = static_object_cast<ElementType>(objectAtIndex(index.row()))) {
					bool enabled = (value.toInt() == Qt::Checked);
					undoableTransaction(tr("Enable/disable type"), [stype, enabled]() {
						stype->setEnabled(enabled);
					});
					return true;
				}
			}
			return RefTargetListParameterUI::setItemData(target, index, value, role);
		}

		/// Returns the model/view item flags for the given entry.
		virtual Qt::ItemFlags getItemFlags(RefTarget* target, const QModelIndex& index) override {
			if(index.column() == 0)
				return RefTargetListParameterUI::getItemFlags(target, index) | Qt::ItemIsUserCheckable;
			else
				return RefTargetListParameterUI::getItemFlags(target, index);
		}

		/// Returns the number of columns for the table view.
		virtual int tableColumnCount() override { return 2; }

		/// Returns the header data under the given role for the given RefTarget.
		/// This method is part of the data model used by the list widget and can be overridden
		/// by sub-classes.
		virtual QVariant getHorizontalHeaderData(int index, int role) override {
			if(index == 0)
				return tr("Color");
			else
				return tr("Name");
		}

		/// Do not open sub-editor for selected item.
		virtual void openSubEditor() override {}
	};

	layout1->addWidget(new QLabel(tr("Burgers vector families:")));
	familiesListUI = new CustomRefTargetListParameterUI(this, PROPERTY_FIELD(MicrostructurePhase::burgersVectorFamilies));
	layout1->addWidget(familiesListUI->tableWidget(200));
	familiesListUI->tableWidget()->setAutoScroll(false);
	connect(familiesListUI->tableWidget(), &QTableWidget::doubleClicked, this, &MicrostructurePhaseEditor::onDoubleClickBurgersFamily);

	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors.</p>"));
	label->setWordWrap(true);
	layout1->addWidget(label);
}

/******************************************************************************
* Is called when the user has double-clicked on one of the entries in the
* list widget.
******************************************************************************/
void MicrostructurePhaseEditor::onDoubleClickBurgersFamily(const QModelIndex& index)
{
	// Let the user select a color for the Burgers vector family.
	BurgersVectorFamily* family = static_object_cast<BurgersVectorFamily>(familiesListUI->selectedObject());
	if(!family) return;

	QColor oldColor = Color(family->color());
	QColor newColor = QColorDialog::getColor(oldColor, container());
	if(!newColor.isValid() || newColor == oldColor)
		return;

	undoableTransaction(tr("Change Burgers vector family color"), [family, &newColor]() {
		family->setColor(Color(newColor));
	});
}

}	// End of namespace
}	// End of namespace
