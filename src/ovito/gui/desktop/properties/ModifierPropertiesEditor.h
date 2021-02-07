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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/gui/desktop/widgets/display/StatusWidget.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>

namespace Ovito {

/**
 * \brief Base class for property editors for Modifier subclasses.
 */
class OVITO_GUI_EXPORT ModifierPropertiesEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ModifierPropertiesEditor)

public:

	/// Constructor.
	ModifierPropertiesEditor();

	/// \brief The virtual destructor.
	virtual ~ModifierPropertiesEditor() { clearAllReferences(); }

	/// Returns a widget that displays status messages of the modifier.
	/// Editor class implementation can add this widget to their user interface.
	StatusWidget* statusLabel();

	/// Returns the list of all ModifierApplications of the modifier currently being edited.
	QVector<ModifierApplication*> modifierApplications() const;

	/// Return the input data of the Modifier being edited (for the selected ModifierApplication).
	PipelineFlowState getModifierInput() const;

	/// Return the output data of the Modifier being edited (for the selected ModifierApplication).
	PipelineFlowState getModifierOutput() const;

Q_SIGNALS:

	/// \brief This signal is emitted whenever the current modifier has generated new results as part of a
	///        pipeline re-evaluation.
    void modifierEvaluated();

	/// \brief This signal is emitted whenever the status of the current modifier or its modifier application has changed.
    void modifierStatusChanged();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex) override;

private Q_SLOTS:

	/// Updates the text of the result label.
	void updateStatusLabel();

private:

	// UI component for displaying the modifier's status.
	QPointer<StatusWidget> _statusLabel;

	/// The modifier application being edited.
	DECLARE_REFERENCE_FIELD_FLAGS(ModifierApplication*, modifierApplication, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);
};

}	// End of namespace
