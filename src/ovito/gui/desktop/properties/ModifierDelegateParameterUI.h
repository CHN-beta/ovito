////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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
#include "ParameterUI.h"

namespace Ovito {

/**
 * \brief UI component that allows the user to select the delegate for a DelegatingModifier.
 */
class OVITO_GUI_EXPORT ModifierDelegateParameterUI : public ParameterUI
{
	Q_OBJECT
	OVITO_CLASS(ModifierDelegateParameterUI)

public:

	/// Constructor.
	ModifierDelegateParameterUI(QObject* parent, const OvitoClass& delegateType);

	/// Destructor.
	virtual ~ModifierDelegateParameterUI();

	/// This returns the combo box managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// This method updates the displayed value of the parameter UI.
	virtual void updateUI() override;

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// Sets the tooltip text for the combo box widget.
	void setToolTip(const QString& text) const {
		if(comboBox()) comboBox()->setToolTip(text);
	}

	/// Sets the What's This helper text for the combo box.
	void setWhatsThis(const QString& text) const {
		if(comboBox()) comboBox()->setWhatsThis(text);
	}

	/// This method populates the combobox widget.
	static void populateComboBox(QComboBox* comboBox, Modifier* modifier, RefTarget* delegate, const DataObjectReference& inputDataObject, const OvitoClass& delegateType);

public:

	Q_PROPERTY(QComboBox comboBox READ comboBox);

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// The combo box of the UI component.
	QPointer<QComboBox> _comboBox;

	/// The type of modifier delegates, which the user can choose from.
	const OvitoClass& _delegateType;
};

}	// End of namespace
