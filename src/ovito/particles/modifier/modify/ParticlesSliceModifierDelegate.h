////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 OVITO GmbH, Germany
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
#include <ovito/stdmod/modifiers/SliceModifier.h>

namespace Ovito::Particles {

using namespace Ovito::StdMod;

/**
 * \brief Function for the SliceModifier that operates on particles.
 */
class ParticlesSliceModifierDelegate : public SliceModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class ParticlesSliceModifierDelegateClass : public SliceModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using SliceModifierDelegate::OOMetaClass::OOMetaClass;

		/// Indicates which data objects in the given input data collection the modifier delegate is able to operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};

	OVITO_CLASS_META(ParticlesSliceModifierDelegate, ParticlesSliceModifierDelegateClass)

	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesSliceModifierDelegate(ObjectCreationParams params) : SliceModifierDelegate(params) {}

	/// Applies a slice operation to a data object.
	virtual PipelineStatus apply(const ModifierEvaluationRequest& request, PipelineFlowState& state, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

}	// End of namespace
