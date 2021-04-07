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
#include <ovito/gui/desktop/properties/PropertiesEditor.h>
#include <ovito/core/utilities/DeferredMethodInvocation.h>

namespace Ovito {

/**
 * \brief A properties editor for the TextLabelOverlay class.
 */
class TextLabelOverlayEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(TextLabelOverlayEditor)

public:

	/// Constructor.
	Q_INVOKABLE TextLabelOverlayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private Q_SLOTS:

	/// Updates the combobox list showing the available data sources.
	void updateSourcesList();

	/// Updates the UI.
	void updateEditorFields();

private:

	QLabel* _attributeNamesList;
	AutocompleteTextEdit* _textEdit;
	DeferredMethodInvocation<TextLabelOverlayEditor, &TextLabelOverlayEditor::updateEditorFields> updateEditorFieldsLater;
};

}	// End of namespace
