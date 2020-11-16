////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include "ElementType.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(ElementType);
DEFINE_PROPERTY_FIELD(ElementType, numericId);
DEFINE_PROPERTY_FIELD(ElementType, color);
DEFINE_PROPERTY_FIELD(ElementType, name);
DEFINE_PROPERTY_FIELD(ElementType, enabled);
SET_PROPERTY_FIELD_LABEL(ElementType, numericId, "Id");
SET_PROPERTY_FIELD_LABEL(ElementType, color, "Color");
SET_PROPERTY_FIELD_LABEL(ElementType, name, "Name");
SET_PROPERTY_FIELD_LABEL(ElementType, enabled, "Enabled");

/******************************************************************************
* Constructs a new ElementType.
******************************************************************************/
ElementType::ElementType(DataSet* dataset) : DataObject(dataset),
	_numericId(0),
	_color(1,1,1),
	_enabled(true)
{
}

/******************************************************************************
* Initializes the element type's attributes to standard values.
******************************************************************************/
void ElementType::initializeType(int propertyType, Application::ExecutionContext executionContext)
{
	setColor(getDefaultColor(propertyType, nameOrNumericId(), numericId(), executionContext));
}

/******************************************************************************
* Returns the default color for a numeric type ID.
******************************************************************************/
const Color& ElementType::getDefaultColorForId(int typeClass, int typeId)
{
	// Palette of standard colors initially assigned to new element types:
	static const Color defaultTypeColors[] = {
		Color(0.97, 0.97, 0.97),// 0
		Color(1.0,  0.4,  0.4), // 1
		Color(0.4,  0.4,  1.0), // 2
		Color(1.0,  1.0,  0.0), // 3
		Color(1.0,  0.4,  1.0), // 4
		Color(0.4,  1.0,  0.2), // 5
		Color(0.8,  1.0,  0.7), // 6
		Color(0.7,  0.0,  1.0), // 7
		Color(0.2,  1.0,  1.0), // 8
	};
	return defaultTypeColors[std::abs(typeId) % (sizeof(defaultTypeColors) / sizeof(defaultTypeColors[0]))];
}

/******************************************************************************
* Returns the default color for a element type name.
******************************************************************************/
Color ElementType::getDefaultColor(int typeClass, const QString& typeName, int typeId, Application::ExecutionContext executionContext)
{
	if(executionContext == Application::ExecutionContext::Interactive) {
		QSettings settings;
		settings.beginGroup("defaults/color");
		settings.beginGroup(QString::number(typeClass));
		QVariant v = settings.value(typeName);
		if(v.isValid() && v.type() == QVariant::Color)
			return v.value<Color>();
	}

	return getDefaultColorForId(typeClass, typeId);
}

/******************************************************************************
* Changes the default color for an element type name.
******************************************************************************/
void ElementType::setDefaultColor(int typeClass, const QString& typeName, const Color& color)
{
	QSettings settings;
	settings.beginGroup("defaults/color");
	settings.beginGroup(QString::number(typeClass));

	if(getDefaultColor(typeClass, typeName, 0, Application::ExecutionContext::Scripting) != color)
		settings.setValue(typeName, QVariant::fromValue((QColor)color));
	else
		settings.remove(typeName);
}

/******************************************************************************
* Creates an editable proxy object for this DataObject and synchronizes its parameters.
******************************************************************************/
void ElementType::updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const
{
	// Note: 'this' may no longer exist at this point, because the sub-class implementation of the method may
	// have already replaced it with a mutable copy.
	const ElementType* self = static_object_cast<ElementType>(dataPath.back());

	if(const ElementType* proxy = static_object_cast<ElementType>(self->editableProxy())) {
		// The numeric ID of a type and some other attributes should never change.
		OVITO_ASSERT(proxy->numericId() == self->numericId());
		OVITO_ASSERT(proxy->enabled() == self->enabled());

		if(proxy->name() != self->name() || proxy->color() != self->color()) {
			// Make this data object mutable first.
			ElementType* mutableSelf = static_object_cast<ElementType>(state.makeMutableInplace(dataPath));
		
			mutableSelf->setName(proxy->name());
			mutableSelf->setColor(proxy->color());
		}
	}
	else {
		// Create and initialize a new proxy.
		OORef<ElementType> newProxy = OORef<ElementType>::makeCopy(self);
		OVITO_ASSERT(newProxy->numericId() == self->numericId());
		OVITO_ASSERT(newProxy->enabled() == self->enabled());

		// Make this element type mutable and attach the proxy object to it.
		state.makeMutableInplace(dataPath)->setEditableProxy(std::move(newProxy));
	}

	DataObject::updateEditableProxies(state, dataPath);
}

}	// End of namespace
}	// End of namespace
