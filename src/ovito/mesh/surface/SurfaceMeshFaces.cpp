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
#include "SurfaceMeshFaces.h"
#include "SurfaceMeshVis.h"

namespace Ovito::Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshFaces);

/******************************************************************************
* Creates a storage object for standard face properties.
******************************************************************************/
PropertyPtr SurfaceMeshFaces::OOMetaClass::createStandardPropertyInternal(DataSet* dataset, size_t faceCount, int type, bool initializeMemory, ObjectInitializationHints initializationHints, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case SelectionProperty:
	case RegionProperty:
	case FaceTypeProperty:
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
	case BurgersVectorProperty:
	case CrystallographicNormalProperty:
		dataType = PropertyObject::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Vector3));
		break;
	default:
		OVITO_ASSERT_MSG(false, "SurfaceMeshFaces::createStandardPropertyInternal", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard face property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = PropertyPtr::create(dataset, initializationHints, faceCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && containerPath.size() >= 2) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const SurfaceMesh* surfaceMesh = dynamic_object_cast<SurfaceMesh>(containerPath[containerPath.size()-2])) {
				ConstPropertyAccess<Color> regionColorProperty = surfaceMesh->regions()->getProperty(SurfaceMeshRegions::ColorProperty);
				ConstPropertyAccess<int> faceRegionProperty = surfaceMesh->faces()->getProperty(SurfaceMeshFaces::RegionProperty);
				if(regionColorProperty && faceRegionProperty && faceRegionProperty.size() == faceCount) {
					// Inherit face colors from regions.
					boost::transform(faceRegionProperty, PropertyAccess<Color>(property).begin(), 
						[&](int region) { return (region >= 0 && region < regionColorProperty.size()) ? regionColorProperty[region] : Color(1,1,1); });
					initializeMemory = false;
				}
				else if(SurfaceMeshVis* vis = surfaceMesh->visElement<SurfaceMeshVis>()) {
					// Initialize face colors from uniform color set in SurfaceMeshVis.
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
void SurfaceMeshFaces::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	setPropertyClassDisplayName(tr("Mesh Faces"));
	setElementDescriptionName(QStringLiteral("faces"));
	setPythonName(QStringLiteral("faces"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyObject::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyObject::Float, rgbList, nullptr, tr("Face colors"));
	registerStandardProperty(FaceTypeProperty, tr("Type"), PropertyObject::Int, emptyList);
	registerStandardProperty(RegionProperty, tr("Region"), PropertyObject::Int, emptyList);
	registerStandardProperty(BurgersVectorProperty, tr("Burgers Vector"), PropertyObject::Float, xyzList, nullptr, tr("Burgers vectors"));
	registerStandardProperty(CrystallographicNormalProperty, tr("Crystallographic Normal"), PropertyObject::Float, xyzList);
}

/******************************************************************************
* Generates a human-readable string representation of the data object reference.
******************************************************************************/
QString SurfaceMeshFaces::OOMetaClass::formatDataObjectPath(const ConstDataObjectPath& path) const
{
	QString str;
	for(const DataObject* obj : path) {
		if(!str.isEmpty())
			str += QStringLiteral(u" \u2192 ");  // Unicode arrow
		str += obj->objectTitle();
	}
	return str;
}

}	// End of namespace
