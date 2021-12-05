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

#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include "SurfaceMeshVertices.h"
#include "SurfaceMeshVis.h"

namespace Ovito::Mesh {


/******************************************************************************
* Creates a storage object for standard vertex properties.
******************************************************************************/
PropertyPtr SurfaceMeshVertices::OOMetaClass::createStandardPropertyInternal(DataSet* dataset, size_t vertexCount, int type, bool initializeMemory, ObjectInitializationHints initializationHints, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case PositionProperty:
		dataType = PropertyObject::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Point3));
		break;
	case SelectionProperty:
		dataType = PropertyObject::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case ColorProperty:
		dataType = PropertyObject::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	default:
		OVITO_ASSERT_MSG(false, "SurfaceMeshVertices::createStandardPropertyInternal", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard vertex property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = PropertyPtr::create(dataset, initializationHints, vertexCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && containerPath.size() >= 2) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(containerPath[containerPath.size()-2])) {
				if(SurfaceMeshVis* vis = surfaceMesh->visElement<SurfaceMeshVis>()) {
					property->fill(vis->surfaceColor());
					initializeMemory = false;
				}
			}
		}
	}

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		property->fillZero();
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void SurfaceMeshVertices::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Mesh Vertices"));
	setElementDescriptionName(QStringLiteral("vertices"));
	setPythonName(QStringLiteral("vertices"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyObject::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyObject::Float, rgbList, nullptr, tr("Vertex colors"));
	registerStandardProperty(PositionProperty, tr("Position"), PropertyObject::Float, xyzList, nullptr, tr("Vertex positions"));
}

/******************************************************************************
* Generates a human-readable string representation of the data object reference.
******************************************************************************/
QString SurfaceMeshVertices::OOMetaClass::formatDataObjectPath(const ConstDataObjectPath& path) const
{
	QString str;
	for(const DataObject* obj : path) {
		if(!str.isEmpty())
			str += QStringLiteral(u" \u2192 ");  // Unicode arrow
		str += obj->objectTitle();
	}
	return str;
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void SurfaceMeshVertices::initializeObject(ObjectInitializationHints hints)
{
	// Assign the default data object identifier.
	setIdentifier(OOClass().pythonName());

	// Create the standard 'Position' property.
	createProperty(SurfaceMeshVertices::PositionProperty, false, hints);

	PropertyContainer::initializeObject(hints);
}

}	// End of namespace
