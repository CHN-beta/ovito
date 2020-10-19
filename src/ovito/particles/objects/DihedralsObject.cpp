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
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "DihedralsObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(DihedralsObject);

/******************************************************************************
* Constructor.
******************************************************************************/
DihedralsObject::DihedralsObject(DataSet* dataset) : PropertyContainer(dataset)
{
	// Assign the default data object identifier.
	setIdentifier(OOClass().pythonName());
}

/******************************************************************************
* Creates a storage object for standard properties.
******************************************************************************/
PropertyPtr DihedralsObject::OOMetaClass::createStandardPropertyInternal(DataSet* dataset, size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case TypeProperty:
		dataType = PropertyObject::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case TopologyProperty:
		dataType = PropertyObject::Int64;
		componentCount = 4;
		stride = componentCount * sizeof(qlonglong);
		break;
	default:
		OVITO_ASSERT_MSG(false, "DihedralsObject::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid dihedral standard property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(elementCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		property->fillZero();
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void DihedralsObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Dihedrals"));
	setElementDescriptionName(QStringLiteral("dihedrals"));
	setPythonName(QStringLiteral("dihedrals"));

	const QStringList emptyList;
	const QStringList abcdList = QStringList() << "A" << "B" << "C" << "D";

	registerStandardProperty(TypeProperty, tr("Dihedral Type"), PropertyObject::Int, emptyList, &ElementType::OOClass(), tr("Dihedral types"));
	registerStandardProperty(TopologyProperty, tr("Topology"), PropertyObject::Int64, abcdList);
}

}	// End of namespace
}	// End of namespace
