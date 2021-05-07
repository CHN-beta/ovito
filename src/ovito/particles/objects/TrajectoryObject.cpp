////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 OVITO GmbH, Germany
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
#include "TrajectoryObject.h"
#include "TrajectoryVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(TrajectoryObject);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void TrajectoryObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Trajectories"));
	setElementDescriptionName(QStringLiteral("vertex"));
	setPythonName(QStringLiteral("trajectories"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	registerStandardProperty(PositionProperty, tr("Position"), PropertyObject::Float, xyzList);
	registerStandardProperty(SampleTimeProperty, tr("Time"), PropertyObject::Int, emptyList);
	registerStandardProperty(ParticleIdentifierProperty, tr("Particle Identifier"), PropertyObject::Int64, emptyList);
}

/******************************************************************************
* Creates a storage object for standard properties.
******************************************************************************/
PropertyPtr TrajectoryObject::OOMetaClass::createStandardPropertyInternal(DataSet* dataset, size_t elementCount, int type, bool initializeMemory, ExecutionContext executionContext, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case PositionProperty:
		dataType = PropertyObject::Float;
		componentCount = 3;
		stride = sizeof(Point3);
		break;
	case SampleTimeProperty:
		dataType = PropertyObject::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case ParticleIdentifierProperty:
		dataType = PropertyObject::Int64;
		componentCount = 1;
		stride = sizeof(qlonglong);
		break;
	default:
		OVITO_ASSERT_MSG(false, "TrajectoryObject::createStandardProperty()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	return PropertyPtr::create(dataset, executionContext, elementCount, dataType, componentCount, stride,
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Default constructor.
******************************************************************************/
TrajectoryObject::TrajectoryObject(DataSet* dataset) : PropertyContainer(dataset)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void TrajectoryObject::initializeObject(ExecutionContext executionContext)
{
	// Assign the default data object identifier.
	setIdentifier(OOClass().pythonName());

	// Create and attach a default visualization element for rendering the trajectory lines.
	if(!visElement())
		setVisElement(OORef<TrajectoryVis>::create(dataset(), executionContext));

	PropertyContainer::initializeObject(executionContext);
}

}	// End of namespace
}	// End of namespace
