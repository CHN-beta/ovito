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
Color ElementType::getDefaultColor(int typeClass, const QString& typeName, int typeId, bool useUserDefaults)
{
	if(useUserDefaults) {
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

	if(getDefaultColor(typeClass, typeName, 0, false) != color)
		settings.setValue(typeName, QVariant::fromValue((QColor)color));
	else
		settings.remove(typeName);
}

/******************************************************************************
* Initializes the element type from a variable list of attributes delivered by a file importer.
******************************************************************************/
bool ElementType::initialize(bool isNewlyCreated, const QString& name, const QVariantMap& attributes, int typePropertyId)
{
	if(isNewlyCreated && Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
		loadUserDefaults();

	// Assign name string.
	if(name != this->name() && (isNewlyCreated || !name.isEmpty())) {
		if(!isSafeToModify())
			return false;
		setName(name);
	}

	// Initialize color value.
	if(attributes.contains(QStringLiteral("color"))) {
		Color c = attributes.value(QStringLiteral("color")).value<Color>();
		if(isNewlyCreated) {
			setColor(c);
		}
		else if(c != color()) {
			if(!isSafeToModify())
				return false;
			setColor(c);
		}
	}
	else if(isNewlyCreated) {
		setColor(getDefaultColor(PropertyObject::GenericTypeProperty, nameOrNumericId(), numericId()));
	}
	
	return true;
}


}	// End of namespace
}	// End of namespace
