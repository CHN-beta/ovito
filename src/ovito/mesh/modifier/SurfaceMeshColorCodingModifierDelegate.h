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


#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdmod/modifiers/ColorCodingModifier.h>

namespace Ovito::Mesh {

using namespace Ovito::StdMod;

/**
 * \brief Delegate function for the ColorCodingModifier that operates on surface mesh vertices.
 */
class OVITO_MESHMOD_EXPORT SurfaceMeshVerticesColorCodingModifierDelegate : public ColorCodingModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ColorCodingModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ColorCodingModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return SurfaceMeshVertices::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("surface_vertices"); }
	};

	OVITO_CLASS_META(SurfaceMeshVerticesColorCodingModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Mesh Vertices");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshVerticesColorCodingModifierDelegate(ObjectCreationParams params) : ColorCodingModifierDelegate(params) {}

protected:

	/// \brief returns the ID of the standard property that will receive the computed colors.
	virtual int outputColorPropertyId() const override { return SurfaceMeshVertices::ColorProperty; }
};

/**
 * \brief Delegate function for the ColorCodingModifier that operates on surface mesh faces.
 */
class OVITO_MESHMOD_EXPORT SurfaceMeshFacesColorCodingModifierDelegate : public ColorCodingModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ColorCodingModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ColorCodingModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return SurfaceMeshFaces::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("surface_faces"); }
	};

	OVITO_CLASS_META(SurfaceMeshFacesColorCodingModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Mesh Faces");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshFacesColorCodingModifierDelegate(ObjectCreationParams params) : ColorCodingModifierDelegate(params) {}

protected:

	/// \brief returns the ID of the standard property that will receive the computed colors.
	virtual int outputColorPropertyId() const override { return SurfaceMeshFaces::ColorProperty; }
};

/**
 * \brief Delegate function for the ColorCodingModifier that operates on surface mesh regions.
 */
class OVITO_MESHMOD_EXPORT SurfaceMeshRegionsColorCodingModifierDelegate : public ColorCodingModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ColorCodingModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using ColorCodingModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// Indicates which class of data objects the modifier delegate is able to operate on.
		virtual const DataObject::OOMetaClass& getApplicableObjectClass() const override { return SurfaceMeshRegions::OOClass(); }

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("surface_regions"); }
	};

	OVITO_CLASS_META(SurfaceMeshRegionsColorCodingModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Mesh Regions");

public:

	/// Constructor.
	Q_INVOKABLE SurfaceMeshRegionsColorCodingModifierDelegate(ObjectCreationParams params) : ColorCodingModifierDelegate(params) {}

protected:

	/// \brief returns the ID of the standard property that will receive the computed colors.
	virtual int outputColorPropertyId() const override { return SurfaceMeshRegions::ColorProperty; }
};

}	// End of namespace
