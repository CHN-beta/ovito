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

#pragma once


#include <ovito/mesh/Mesh.h>
#include <ovito/stdobj/properties/PropertyContainer.h>

namespace Ovito { namespace Mesh {

/**
 * \brief Stores all vertex-related properties of a SurfaceMesh.
 */
class OVITO_MESH_EXPORT SurfaceMeshVertices : public PropertyContainer
{
	/// Define a new property metaclass for this container type.
	class OOMetaClass : public PropertyContainerClass
	{
	public:

		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

		/// Create a storage object for standard vertex properties.
		virtual PropertyPtr createStandardPropertyInternal(DataSet* dataset, size_t vertexCount, int type, bool initializeMemory, Application::ExecutionContext executionContext, const ConstDataObjectPath& containerPath) const override;

		/// Generates a human-readable string representation of the data object reference.
		virtual QString formatDataObjectPath(const ConstDataObjectPath& path) const override;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(SurfaceMeshVertices, OOMetaClass);
	Q_CLASSINFO("DisplayName", "Mesh Vertices");

public:

	/// \brief The list of standard vertex properties.
	enum Type {
		UserProperty = PropertyObject::GenericUserProperty,	//< This is reserved for user-defined properties.
		SelectionProperty = PropertyObject::GenericSelectionProperty,
		ColorProperty = PropertyObject::GenericColorProperty,
		PositionProperty = PropertyObject::FirstSpecificProperty
	};

	/// \brief Constructor.
	Q_INVOKABLE SurfaceMeshVertices(DataSet* dataset) : PropertyContainer(dataset) {}

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(Application::ExecutionContext executionContext) override;	
};

}	// End of namespace
}	// End of namespace
