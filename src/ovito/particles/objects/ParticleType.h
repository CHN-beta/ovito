////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/ParticlesVis.h>
#include <ovito/stdobj/properties/ElementType.h>
#include <ovito/mesh/tri/TriMeshObject.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the properties of a particle type, e.g. name, color, and radius.
 */
class OVITO_PARTICLES_EXPORT ParticleType : public ElementType
{
	Q_OBJECT
	OVITO_CLASS(ParticleType)

public:

	enum PredefinedParticleType {
		    H , He, Li, Be, B , C , N , O , F , Ne,
		Na, Mg, Al, Si, P , S , Cl, Ar, K , Ca, Sc,
		Ti, V , Cr, Mn, Fe, Co, Ni, Cu, Zn, Ga, Ge,
		As, Se, Br, Kr, Rb, Sr, Y , Zr, Nb, Mo, Tc,
		Ru, Rh, Pd, Ag, Cd, In, Sn, Sb, Te, I , Xe,
		Cs, Ba, La, Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb,
		Dy, Ho, Er, Tm, Yb, Lu, Hf, Ta, W , Re, Os,
		Ir, Pt, Au, Hg, Tl, Pb, Bi, Po, At, Rn, Fr,
//		Ra, Ac, Th, Pa, U , Np, Pu, Am, Cm, Bk, Cf,
//		Es, Fm, Md, No, Lr, Rf, Db, Sg, Bh, Hs, Mt,
//		Ds, Rg,

		NUMBER_OF_PREDEFINED_PARTICLE_TYPES
	};

	enum PredefinedStructureType {
		OTHER = 0,					//< Unidentified structure
		FCC,						//< Face-centered cubic
		HCP,						//< Hexagonal close-packed
		BCC,						//< Body-centered cubic
		ICO,						//< Icosahedral structure
		CUBIC_DIAMOND,				//< Cubic diamond structure
		CUBIC_DIAMOND_FIRST_NEIGH,	//< First neighbor of a cubic diamond atom
		CUBIC_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a cubic diamond atom
		HEX_DIAMOND,				//< Hexagonal diamond structure
		HEX_DIAMOND_FIRST_NEIGH,	//< First neighbor of a hexagonal diamond atom
		HEX_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a hexagonal diamond atom
		SC,							//< Simple cubic structure
		GRAPHENE,						//< Graphene structure
		HEXAGONAL_ICE,
		CUBIC_ICE,
		INTERFACIAL_ICE,
		HYDRATE,
		INTERFACIAL_HYDRATE,
		NUMBER_OF_PREDEFINED_STRUCTURE_TYPES
	};

	enum RadiusVariant {
		DisplayRadius,
		VanDerWaalsRadius
	};

public:

	/// \brief Constructs a new particle type.
	Q_INVOKABLE ParticleType(DataSet* dataset);

	/// \brief Initializes the element type's attributes to standard values.
	virtual void initializeType(const PropertyReference& property, ExecutionContext executionContext) override;

	/// Creates an editable proxy object for this DataObject and synchronizes its parameters.
	virtual void updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const override;

	//////////////////////////////////// Utility methods ////////////////////////////////

	/// Builds a map from type identifiers to particle radii.
	static std::map<int,FloatType> typeRadiusMap(const PropertyObject* typeProperty) {
		std::map<int,FloatType> m;
		for(const ElementType* type : typeProperty->elementTypes())
			if(const ParticleType* particleType = dynamic_object_cast<ParticleType>(type))
				m.insert({ type->numericId(), particleType->radius() });
		return m;
	}

	/// Builds a map from type identifiers to particle masses.
	static std::map<int,FloatType> typeMassMap(const PropertyObject* typeProperty) {
		std::map<int,FloatType> m;
		for(const ElementType* type : typeProperty->elementTypes())
			if(const ParticleType* particleType = dynamic_object_cast<ParticleType>(type))
				m.insert({ type->numericId(), particleType->mass() });
		return m;
	}

	/// Loads a user-defined display shape from a geometry file and assigns it to this particle type.
	bool loadShapeMesh(const QUrl& sourceUrl, Promise<>&& operation, ExecutionContext executionContext, const FileImporterClass* importerType = nullptr);

	//////////////////////////////////// Default parameters ////////////////////////////////

	/// Returns the name string of a predefined particle type.
	static const QString& getPredefinedParticleTypeName(PredefinedParticleType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_PARTICLE_TYPES);
		return _predefinedParticleTypes[predefType].name;
	}

	/// Returns the hard-coded color of a predefined particle type.
	static const Color& getPredefinedParticleTypeColor(PredefinedParticleType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_PARTICLE_TYPES);
		return _predefinedParticleTypes[predefType].color;
	}

	/// Returns the name string of a predefined structure type.
	static const QString& getPredefinedStructureTypeName(PredefinedStructureType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_STRUCTURE_TYPES);
		return _predefinedStructureTypes[predefType].name;
	}

	/// Returns the hard-coded color of a predefined structure type.
	static const Color& getPredefinedStructureTypeColor(PredefinedStructureType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_STRUCTURE_TYPES);
		return _predefinedStructureTypes[predefType].color;
	}

	/// Returns the default radius for a named particle type.
	static FloatType getDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, ExecutionContext executionContext, RadiusVariant radiusVariant = DisplayRadius);

	/// Changes the default radius for a named particle type.
	static void setDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, FloatType radius, RadiusVariant radiusVariant = DisplayRadius);

	/// Returns the default mass for a named particle type.
	static FloatType getDefaultParticleMass(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, ExecutionContext executionContext);

protected:

	/// Is called once for this object after it has been completely loaded from a stream.
	virtual void loadFromStreamComplete(ObjectLoadStream& stream) override;

private:

	/// The radius used for rendering particles of this type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, radius, setRadius);

	/// The van der Waals radius of this particle type, which is used for generating bonds between particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, vdwRadius, setVdwRadius);

	/// The visualization shape for particles of this type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlesVis::ParticleShape, shape, setShape);

	/// An optional user-defined shape used for rendering particles of this type.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(DataOORef<const TriMeshObject>, shapeMesh, setShapeMesh, PROPERTY_FIELD_NO_SUB_ANIM);

	/// Activates the highlighting of the polygonal edges of the user-defined shape assigned to this particle type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, highlightShapeEdges, setHighlightShapeEdges, PROPERTY_FIELD_MEMORIZE);

	/// Activates the culling of back-facing faces of the user-defined shape assigned to this particle type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, shapeBackfaceCullingEnabled, setShapeBackfaceCullingEnabled, PROPERTY_FIELD_MEMORIZE);

	/// Use the mesh colors intead of particle colors when rendering the user-defined shape.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, shapeUseMeshColor, setShapeUseMeshColor);

	/// The mass of this particle type (maybe zero if not set).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, mass, setMass);

private:

	/// Data structure that holds the name, color, and radius of a chemical atom type.
	struct PredefinedChemicalType {
		QString name;
		Color color;
		FloatType displayRadius;
		FloatType vdwRadius;
		FloatType mass;
	};

	/// Data structure that holds the name and display color of a structural particle type.
	struct PredefinedStructuralType {
		QString name;
		Color color;
	};

	/// Contains default names, colors, and radii for some predefined particle types.
	static const std::array<PredefinedChemicalType, NUMBER_OF_PREDEFINED_PARTICLE_TYPES> _predefinedParticleTypes;

	/// Contains default names, colors, and radii for the predefined structure types.
	static const std::array<PredefinedStructuralType, NUMBER_OF_PREDEFINED_STRUCTURE_TYPES> _predefinedStructureTypes;
};

}	// End of namespace
}	// End of namespace
