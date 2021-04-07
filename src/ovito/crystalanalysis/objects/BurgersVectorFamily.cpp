////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 OVITO GmbH, Germany
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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/data/ClusterVector.h>
#include "BurgersVectorFamily.h"
#include "MicrostructurePhase.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(BurgersVectorFamily);
DEFINE_PROPERTY_FIELD(BurgersVectorFamily, burgersVector);
SET_PROPERTY_FIELD_LABEL(BurgersVectorFamily, burgersVector, "Burgers vector");

/******************************************************************************
* Constructs a new BurgersVectorFamily.
******************************************************************************/
BurgersVectorFamily::BurgersVectorFamily(DataSet* dataset, int id, const QString& name, const Vector3& burgersVector, const Color& color)
	: ElementType(dataset), _burgersVector(burgersVector)
{
	setNumericId(id);
	setName(name);
	setColor(color);
}

/******************************************************************************
* Checks if the given Burgers vector is a member of this family.
******************************************************************************/
bool BurgersVectorFamily::isMember(const Vector3& v, const MicrostructurePhase* latticeStructure) const
{
	if(burgersVector() == Vector3::Zero())
		return false;

	if(latticeStructure->crystalSymmetryClass() == MicrostructurePhase::CrystalSymmetryClass::CubicSymmetry) {

		// Bring prototype vector into canonical form.
		Vector3 sc1(std::fabs(burgersVector().x()), std::fabs(burgersVector().y()), std::fabs(burgersVector().z()));
		std::sort(sc1.data(), sc1.data() + 3);

		// Bring candidate vector into canonical form.
		Vector3 sc2(std::fabs(v.x()), std::fabs(v.y()), std::fabs(v.z()));
		std::sort(sc2.data(), sc2.data() + 3);

		return sc2.equals(sc1, CA_LATTICE_VECTOR_EPSILON);
	}
	else if(latticeStructure->crystalSymmetryClass() == MicrostructurePhase::CrystalSymmetryClass::HexagonalSymmetry) {

		// Bring prototype vector into canonical form.
		Vector3 sc1a(std::fabs(burgersVector().x()), std::fabs(burgersVector().y()), std::fabs(burgersVector().z()));
		Vector3 sc1b(
				std::fabs(0.5f*burgersVector().x()+sqrt(3.0f)/2*burgersVector().y()),
				std::fabs(0.5f*burgersVector().y()-sqrt(3.0f)/2*burgersVector().x()),
				std::fabs(burgersVector().z()));

		// Bring candidate vector into canonical form.
		Vector3 sc2(std::fabs(v.x()), std::fabs(v.y()), std::fabs(v.z()));

		return sc2.equals(sc1a, CA_LATTICE_VECTOR_EPSILON) || sc2.equals(sc1b, CA_LATTICE_VECTOR_EPSILON);
	}
	return false;
}

}	// End of namespace
}	// End of namespace
