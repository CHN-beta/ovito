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

#include <ovito/particles/Particles.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "BondType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondType);
DEFINE_PROPERTY_FIELD(BondType, radius);
SET_PROPERTY_FIELD_LABEL(BondType, radius, "Radius");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(BondType, radius, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new BondType.
******************************************************************************/
BondType::BondType(DataSet* dataset) : ElementType(dataset), _radius(0)
{
}

/******************************************************************************
* Initializes the particle type's attributes to standard values.
******************************************************************************/
void BondType::initializeType(int propertyType, Application::ExecutionContext executionContext)
{
	ElementType::initializeType(propertyType, executionContext);

	setColor(getDefaultBondColor(static_cast<BondsObject::Type>(propertyType), nameOrNumericId(), numericId(), executionContext));
	setRadius(getDefaultBondRadius(static_cast<BondsObject::Type>(propertyType), nameOrNumericId(), numericId(), executionContext));
}

/******************************************************************************
* Creates an editable proxy object for this DataObject and synchronizes its parameters.
******************************************************************************/
void BondType::updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const
{
	ElementType::updateEditableProxies(state, dataPath);

	// Note: 'this' may no longer exist at this point, because the base method implementationmay
	// have already replaced it with a mutable copy.
	const BondType* self = static_object_cast<BondType>(dataPath.back());

	if(const BondType* proxy = static_object_cast<BondType>(self->editableProxy())) {
		if(proxy->radius() != self->radius()) {
			// Make this data object mutable first.
			BondType* mutableSelf = static_object_cast<BondType>(state.makeMutableInplace(dataPath));
			mutableSelf->setRadius(proxy->radius());
		}
	}
}

/******************************************************************************
* Returns the default color for a bond type ID.
******************************************************************************/
Color BondType::getDefaultBondColorForId(BondsObject::Type typeClass, int bondTypeId)
{
	// Initial standard colors assigned to new bond types:
	static const Color defaultTypeColors[] = {
		Color(1.0,  1.0,  0.0),
		Color(0.7,  0.0,  1.0),
		Color(0.2,  1.0,  1.0),
		Color(1.0,  0.4,  1.0),
		Color(0.4,  1.0,  0.4),
		Color(1.0,  0.4,  0.4),
		Color(0.4,  0.4,  1.0),
		Color(1.0,  1.0,  0.7),
		Color(0.97, 0.97, 0.97)
	};
	return defaultTypeColors[std::abs(bondTypeId) % (sizeof(defaultTypeColors) / sizeof(defaultTypeColors[0]))];
}

/******************************************************************************
* Returns the default color for a bond type name.
******************************************************************************/
Color BondType::getDefaultBondColor(BondsObject::Type typeClass, const QString& bondTypeName, int bondTypeId, Application::ExecutionContext executionContext)
{
	if(executionContext == Application::ExecutionContext::Interactive) {
		QSettings settings;
		settings.beginGroup("bonds/defaults/color");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(bondTypeName);
		if(v.isValid() && v.type() == QVariant::Color)
			return v.value<Color>();
	}

	return getDefaultBondColorForId(typeClass, bondTypeId);
}

/******************************************************************************
* Returns the default radius for a bond type name.
******************************************************************************/
FloatType BondType::getDefaultBondRadius(BondsObject::Type typeClass, const QString& bondTypeName, int bondTypeId, Application::ExecutionContext executionContext)
{
	if(executionContext == Application::ExecutionContext::Interactive) {
		QSettings settings;
		settings.beginGroup("bonds/defaults/radius");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(bondTypeName);
		if(v.isValid() && v.canConvert<FloatType>())
			return v.value<FloatType>();
	}

	return 0;
}

}	// End of namespace
}	// End of namespace
