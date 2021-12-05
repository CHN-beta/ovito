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


#include <ovito/stdobj/gui/StdObjGui.h>
#include <ovito/stdobj/gui/widgets/PropertySelectionComboBox.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/gui/desktop/properties/ParameterUI.h>

namespace Ovito::StdObj {

/**
 * \brief This parameter UI lets the user select a property.
 */
class OVITO_STDOBJGUI_EXPORT PropertyReferenceParameterUI : public PropertyParameterUI
{
	OVITO_CLASS(PropertyReferenceParameterUI)

	Q_PROPERTY(QComboBox comboBox READ comboBox)

public:

    enum PropertyComponentsMode {
		ShowOnlyComponents,
		ShowNoComponents,
		ShowComponentsAndVectorProperties
	};
    Q_ENUM(PropertyComponentsMode);

	/// Constructor.
	PropertyReferenceParameterUI(PropertiesEditor* parentEditor, const char* propertyName, PropertyContainerClassPtr containerClass = nullptr, PropertyComponentsMode componentsMode = ShowOnlyComponents, bool inputProperty = true);

	/// Constructor.
	PropertyReferenceParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor* propField, PropertyContainerClassPtr containerClass = nullptr, PropertyComponentsMode componentsMode = ShowOnlyComponents, bool inputProperty = true);

	/// Destructor.
	virtual ~PropertyReferenceParameterUI();

	/// This returns the combo box managed by this ParameterUI.
	QComboBox* comboBox() const { return _comboBox; }

	/// This method is called when a new editable object has been assigned to the properties owner this
	/// parameter UI belongs to.
	virtual void resetUI() override;

	/// This method updates the displayed value of the property UI.
	virtual void updateUI() override;

	/// Sets the enabled state of the UI.
	virtual void setEnabled(bool enabled) override;

	/// Sets the tooltip text for the combo box widget.
	void setToolTip(const QString& text) const {
		if(comboBox()) comboBox()->setToolTip(text);
	}

	/// Sets the What's This helper text for the combo box.
	void setWhatsThis(const QString& text) const {
		if(comboBox()) comboBox()->setWhatsThis(text);
	}

	/// Returns the data object reference to the property container from which the user can select a property.
	const PropertyContainerReference& containerRef() const { return _containerRef; }

	/// Sets the reference to the property container from which the user can select a property.
	void setContainerRef(const PropertyContainerReference& containerRef);

	/// Returns the container from which properties can be selected.
	const DataOORef<const PropertyContainer>& container() const { return _container; }

	/// Sets the concrete container from which properties can be selected.
	void setContainer(const PropertyContainer* container);

	/// Installs optional callback function that allows clients to filter the displayed property list.
	void setPropertyFilter(std::function<bool(const PropertyObject*)> filter) {
		_propertyFilter = std::move(filter);
	}

public Q_SLOTS:

	/// Takes the value entered by the user and stores it in the property field
	/// this property UI is bound to.
	void updatePropertyValue();

private:

	/// Returns the value currently set for the property field.
	PropertyReference getPropertyReference();

	/// Populates the combox box with items.
	void addItemsToComboBox(const PipelineFlowState& state);

	/// Populates the combox box with items.
	void addItemsToComboBox(const PropertyContainer* container);

	/// Returns the type of property container from which the user can choose a property.
	const PropertyContainerClass* containerClass() const { 
		if(container()) 
			return &container()->getOOMetaClass();
		else
			return containerRef().dataClass();
	}

protected:

	/// The combo box of the UI component.
	QPointer<PropertySelectionComboBox> _comboBox;

	/// Controls whether the combo box should display a separate entry for each component of a property.
	PropertyComponentsMode _componentsMode;

	/// Controls whether the combo box should list input or output properties.
	bool _isInputProperty;

	/// Data object reference to the container from which properties can be selected.
	PropertyContainerReference _containerRef;

	/// The container from which properties can be selected.
	DataOORef<const PropertyContainer> _container;

	/// An optional callback function that allows clients to filter the displayed property list.
	std::function<bool(const PropertyObject*)> _propertyFilter;
};

}	// End of namespace
