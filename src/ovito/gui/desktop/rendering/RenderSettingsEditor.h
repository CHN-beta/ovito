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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/animation/TimeInterval.h>

namespace Ovito {

/**
 * The editor component for the RenderSettings class.
 */
class RenderSettingsEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(RenderSettingsEditor)

public:

	/// Constructor.
	Q_INVOKABLE RenderSettingsEditor() {}

	/// \brief Destructor.
	virtual ~RenderSettingsEditor() {
		clearAllReferences();
	}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a referenced object has changed.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex) override;

private Q_SLOTS:

	/// Lets the user choose a filename for the output image.
	void onChooseImageFilename();

	/// Is called when the user selects an output size preset from the drop-down list.
	void onSizePresetActivated(int index);

	/// Lets the user choose a different plug-in rendering engine.
	void onSwitchRenderer();

	/// This is called whenever the current viewport configuration of current dataset has been replaced by a new one.
	void onViewportConfigReplaced(ViewportConfiguration* newViewportConfiguration);

	/// This is called when another viewport became active.
	void onActiveViewportChanged(Viewport* activeViewport);

	/// Is called when the user toggles the preview mode checkbox.
	void onViewportPreviewModeToggled(bool checked);

private:

	/// Weak reference to the currently active viewport.
	DECLARE_REFERENCE_FIELD_FLAGS(Viewport*, activeViewport, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_SUB_ANIM | PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES);

	QComboBox* _sizePresetsBox;
	QCheckBox* _viewportPreviewModeBox;

	/// Signal connection for detecting active viewport changes.
	QMetaObject::Connection _activeViewportChangedConnection;
};

}	// End of namespace
