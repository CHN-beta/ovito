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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include "MicrostructurePhase.h"

namespace Ovito::CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(MicrostructurePhase);
DEFINE_PROPERTY_FIELD(MicrostructurePhase, shortName);
DEFINE_PROPERTY_FIELD(MicrostructurePhase, dimensionality);
DEFINE_PROPERTY_FIELD(MicrostructurePhase, crystalSymmetryClass);
DEFINE_VECTOR_REFERENCE_FIELD(MicrostructurePhase, burgersVectorFamilies);
DEFINE_SHADOW_PROPERTY_FIELD(MicrostructurePhase, shortName);
DEFINE_SHADOW_PROPERTY_FIELD(MicrostructurePhase, dimensionality);
DEFINE_SHADOW_PROPERTY_FIELD(MicrostructurePhase, crystalSymmetryClass);
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, shortName, "Short name");
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, dimensionality, "Dimensionality");
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, crystalSymmetryClass, "Symmetry class");
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, burgersVectorFamilies, "Burgers vector families");

/******************************************************************************
* Constructs the MicrostructurePhase object.
******************************************************************************/
MicrostructurePhase::MicrostructurePhase(DataSet* dataset) : ElementType(dataset),
	_dimensionality(Dimensionality::None),
	_crystalSymmetryClass(CrystalSymmetryClass::NoSymmetry)
{
}

/******************************************************************************
* Returns the display color to be used for a given Burgers vector.
******************************************************************************/
Color MicrostructurePhase::getBurgersVectorColor(const QString& latticeName, const Vector3& b)
{
	if(latticeName == ParticleType::getPredefinedStructureTypeName(ParticleType::PredefinedStructureType::BCC)) {
		return getBurgersVectorColor(ParticleType::PredefinedStructureType::BCC, b);
	}
	else if(latticeName == ParticleType::getPredefinedStructureTypeName(ParticleType::PredefinedStructureType::FCC)) {
		return getBurgersVectorColor(ParticleType::PredefinedStructureType::FCC, b);
	}
	return getBurgersVectorColor(ParticleType::PredefinedStructureType::OTHER, b);
}

/******************************************************************************
* Returns the display color to be used for a given Burgers vector.
******************************************************************************/
Color MicrostructurePhase::getBurgersVectorColor(ParticleType::PredefinedStructureType structureType, const Vector3& b)
{
	if(structureType == ParticleType::PredefinedStructureType::BCC) {
		static const Color predefinedLineColors[] = {
				Color(0.4f,1.0f,0.4f),
				Color(1.0f,0.2f,0.2f),
				Color(0.4f,0.4f,1.0f),
				Color(0.9f,0.5f,0.0f),
				Color(1.0f,1.0f,0.0f),
				Color(1.0f,0.4f,1.0f),
				Color(0.7f,0.0f,1.0f)
		};
		static const Vector3 burgersVectors[] = {
				{ FloatType(0.5), FloatType(0.5), FloatType(0.5) },
				{ FloatType(-0.5), FloatType(0.5), FloatType(0.5) },
				{ FloatType(0.5), FloatType(-0.5), FloatType(0.5) },
				{ FloatType(0.5), FloatType(0.5), FloatType(-0.5) },
				{ FloatType(1.0), FloatType(0.0), FloatType(0.0) },
				{ FloatType(0.0), FloatType(1.0), FloatType(0.0) },
				{ FloatType(0.0), FloatType(0.0), FloatType(1.0) }
		};
		OVITO_STATIC_ASSERT(sizeof(burgersVectors)/sizeof(burgersVectors[0]) == sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(burgersVectors)/sizeof(burgersVectors[0]); i++) {
			if(b.equals(burgersVectors[i], FloatType(1e-6)) || b.equals(-burgersVectors[i], FloatType(1e-6)))
				return predefinedLineColors[i];
		}
	}
	else if(structureType == ParticleType::PredefinedStructureType::FCC) {
		static const Color predefinedLineColors[] = {
				Color(230.0/255.0, 25.0/255.0, 75.0/255.0),
				Color(245.0/255.0, 130.0/255.0, 48.0/255.0),
				Color(255.0/255.0, 225.0/255.0, 25.0/255.0),
				Color(210.0/255.0, 245.0/255.0, 60.0/255.0),
				Color(60.0/255.0, 180.0/255.0, 75.0/255.0),
				Color(70.0/255.0, 240.0/255.0, 240.0/255.0),
				Color(0.0/255.0, 130.0/255.0, 200.0/255.0),
				Color(145.0/255.0, 30.0/255.0, 180.0/255.0),
				Color(240.0/255.0, 50.0/255.0, 230.0/255.0),
				Color(0.0/255.0, 128.0/255.0, 128.0/255.0),
				Color(170.0/255.0, 110.0/255.0, 40.0/255.0),
				Color(128.0/255.0, 128.0/255.0, 0.0/255.0),

				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
		};
		static const Vector3 burgersVectors[] = {
				{ FloatType(1.0/6.0), FloatType(-2.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-2.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), FloatType(2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), FloatType(-2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), FloatType(2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), FloatType(-2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(2.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(2.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(-1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(-1.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(1.0/6.0), FloatType(1.0/6.0) },

				{ 0, FloatType(1.0/6.0), FloatType(1.0/6.0) },
				{ 0, FloatType(1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), 0, FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), 0, FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), 0 },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), 0 },
		};
		OVITO_STATIC_ASSERT(sizeof(burgersVectors)/sizeof(burgersVectors[0]) == sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(burgersVectors)/sizeof(burgersVectors[0]); i++) {
			if(b.equals(burgersVectors[i], FloatType(1e-6)) || b.equals(-burgersVectors[i], FloatType(1e-6)))
				return predefinedLineColors[i];
		}
	}
	return Color(0.9f, 0.9f, 0.9f);
}

/******************************************************************************
* Creates an editable proxy object for this DataObject and synchronizes its parameters.
******************************************************************************/
void MicrostructurePhase::updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const
{
	ElementType::updateEditableProxies(state, dataPath);

	// Note: 'this' may no longer exist at this point, because the sub-class implementation of the method may
	// have already replaced it with a mutable copy.
	const MicrostructurePhase* self = static_object_cast<MicrostructurePhase>(dataPath.back());

	if(MicrostructurePhase* proxy = static_object_cast<MicrostructurePhase>(self->editableProxy())) {
		// Adopt the proxy objects for the Burgers vector families, which have already been created by
		// the recursive method.
		OVITO_ASSERT(proxy->burgersVectorFamilies().size() == self->burgersVectorFamilies().size());
		for(int i = 0; i < self->burgersVectorFamilies().size(); i++) {
			OVITO_ASSERT(proxy->isSafeToModify());
			const BurgersVectorFamily* family = self->burgersVectorFamilies()[i];
			OVITO_ASSERT(family->editableProxy());
			proxy->_burgersVectorFamilies.set(proxy, PROPERTY_FIELD(burgersVectorFamilies), i, static_object_cast<BurgersVectorFamily>(family->editableProxy()));
		}
	}
}

}	// End of namespace
