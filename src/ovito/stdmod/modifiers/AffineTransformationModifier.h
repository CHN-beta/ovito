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


#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>

namespace Ovito::StdMod {

/**
 * \brief Base class for AffineTransformationModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT AffineTransformationModifierDelegate : public ModifierDelegate
{
	Q_OBJECT
	OVITO_CLASS(AffineTransformationModifierDelegate)

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;
};

/**
 * \brief Delegate for the AffineTransformationModifier that operates on simulation cells.
 */
class OVITO_STDMOD_EXPORT SimulationCellAffineTransformationModifierDelegate : public AffineTransformationModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public AffineTransformationModifierDelegate::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using AffineTransformationModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass which data objects in the given input data collection the modifier delegate can operate on.
		virtual QVector<DataObjectReference> getApplicableObjects(const DataCollection& input) const override;

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("cell"); }
	};

	OVITO_CLASS_META(SimulationCellAffineTransformationModifierDelegate, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Simulation cell");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE SimulationCellAffineTransformationModifierDelegate(DataSet* dataset) : AffineTransformationModifierDelegate(dataset) {}

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;
};

/**
 * \brief This modifier applies an arbitrary affine transformation to the
 *        particles, the simulation box and other entities.
 *
 * The affine transformation is specified as a 3x4 matrix.
 */
class OVITO_STDMOD_EXPORT AffineTransformationModifier : public MultiDelegatingModifier
{
public:

	/// Give this modifier class its own metaclass.
	class OOMetaClass : public MultiDelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using MultiDelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return AffineTransformationModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(AffineTransformationModifier, OOMetaClass)
	Q_CLASSINFO("DisplayName", "Affine transformation");
	Q_CLASSINFO("Description", "Apply an affine transformation to the dataset.");
	Q_CLASSINFO("ModifierCategory", "Modification");
	Q_OBJECT

public:

	/// \brief Constructor.
	Q_INVOKABLE AffineTransformationModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Returns the effective affine transformation matrix to be applied to points.
	/// It depends on the linear matrix, the translation vector, relative/target cell mode, and 
	/// whether the translation is specified in terms of reduced cell coordinates.
	/// Thus, the affine transformation may depend on the current simulation cell shape.
	AffineTransformation effectiveAffineTransformation(const PipelineFlowState& state) const;

protected:

	/// This property fields stores the transformation matrix (used in 'relative' mode).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, transformationTM, setTransformationTM);

	/// This property fields stores the simulation cell geometry (used in 'absolute' mode).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, targetCell, setTargetCell);

	/// This controls whether the transformation is applied only to the selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectionOnly, setSelectionOnly);

	/// This controls whether a relative transformation is applied to the simulation box or
	/// the absolute cell geometry has been specified.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, relativeMode, setRelativeMode);

	/// Controls whether the translation vector is specified in reduced cell coordinated.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, translationReducedCoordinates, setTranslationReducedCoordinates);
};

}	// End of namespace
