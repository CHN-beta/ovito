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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/stdobj/properties/ElementType.h>
#include <ovito/particles/objects/ParticleType.h>
#include "BurgersVectorFamily.h"

namespace Ovito { namespace CrystalAnalysis {

/**
 * \brief Data structure representing a phase (e.g. a crystal structure) in a Microstructure.
 */
class OVITO_CRYSTALANALYSIS_EXPORT MicrostructurePhase : public ElementType
{
	Q_OBJECT
	OVITO_CLASS(MicrostructurePhase)

public:

	/// The dimensionality of the structure.
	enum Dimensionality {
		None,			///< None of the types below
		Volumetric,		///< Volumetric phase
		Planar,			///< Planar interface, grain boundary, stacking fault, etc.
		Pointlike		///< Zero-dimensional defect
	};
	Q_ENUMS(Dimensionality);

	/// The type of symmetry of the crystal lattice.
	enum CrystalSymmetryClass {
		NoSymmetry,			///< Unknown or no crystal symmetry.
		CubicSymmetry,		///< Used for cubic crystals like FCC, BCC, diamond.
		HexagonalSymmetry	///< Used for hexagonal crystals like HCP, hexagonal diamond.
	};
	Q_ENUMS(CrystalSymmetryClass);

public:

	/// Standard constructor.
	Q_INVOKABLE MicrostructurePhase(DataSet* dataset);

	/// Returns the lotitleng name of this phase.
	const QString& longName() const { return name(); }

	/// Assigns a long title to this phase.
	void setLongName(const QString& name) { setName(name); }

	/// Adds a new family to this phase's list of Burgers vector families.
	void addBurgersVectorFamily(BurgersVectorFamily* family) { _burgersVectorFamilies.push_back(this, PROPERTY_FIELD(burgersVectorFamilies), family); }

	/// Removes a family from this lattice pattern's list of Burgers vector families.
	void removeBurgersVectorFamily(int index) { _burgersVectorFamilies.remove(this, PROPERTY_FIELD(burgersVectorFamilies), index); }

	/// Returns the default Burgers vector family, which is assigned to dislocation segments that
	/// don't belong to any family.
	BurgersVectorFamily* defaultBurgersVectorFamily() const { return !burgersVectorFamilies().empty() ? burgersVectorFamilies().front() : nullptr; }

	/// Returns the display color to be used for a given Burgers vector.
	static Color getBurgersVectorColor(const QString& latticeName, const Vector3& b);

	/// Returns the display color to be used for a given Burgers vector.
	static Color getBurgersVectorColor(ParticleType::PredefinedStructureType structureType, const Vector3& b);

private:

	/// The shortened title of this phase.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(QString, shortName, setShortName, PROPERTY_FIELD_DATA_OBJECT);

	/// The dimensionality type of the phase.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Dimensionality, dimensionality, setDimensionality, PROPERTY_FIELD_DATA_OBJECT);

	/// The type of crystal symmetry of the phase if it is crystalline.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(CrystalSymmetryClass, crystalSymmetryClass, setCrystalSymmetryClass, PROPERTY_FIELD_DATA_OBJECT);

	/// List of Burgers vector families defined for the phase if it is crystalline.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(BurgersVectorFamily, burgersVectorFamilies, setBurgersVectorFamilies, PROPERTY_FIELD_DATA_OBJECT);
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::CrystalAnalysis::MicrostructurePhase::Dimensionality);
Q_DECLARE_TYPEINFO(Ovito::CrystalAnalysis::MicrostructurePhase::Dimensionality, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(Ovito::CrystalAnalysis::MicrostructurePhase::CrystalSymmetryClass);
Q_DECLARE_TYPEINFO(Ovito::CrystalAnalysis::MicrostructurePhase::CrystalSymmetryClass, Q_PRIMITIVE_TYPE);
