///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleProperty.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/stdmod/modifiers/ComputePropertyModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Function for the ComputePropertyModifier that operates on particles.
 */
class OVITO_PARTICLES_EXPORT ParticlesComputePropertyModifierDelegate : public ComputePropertyModifierDelegate
{
	/// Give the modifier delegate its own metaclass.
	class OOMetaClass : public ComputePropertyModifierDelegate::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using ComputePropertyModifierDelegate::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override {
			return input.findObject<ParticleProperty>() != nullptr;
		}

		/// The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const override { return QStringLiteral("particles"); }
	};	

	Q_OBJECT
	OVITO_CLASS_META(ParticlesComputePropertyModifierDelegate, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// Constructor.
	Q_INVOKABLE ParticlesComputePropertyModifierDelegate(DataSet* dataset);

	/// \brief Returns the class of properties this delegate computes.
	virtual const PropertyClass& propertyClass() const override { return ParticleProperty::OOClass(); }

	/// \brief Sets the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setNeighborExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= neighborExpressions().size())
			throwException("Property component index is out of range.");
		QStringList copy = _neighborExpressions;
		copy[index] = expression;
		setNeighborExpressions(std::move(copy));
	}

	/// \brief Returns the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be returned.
	/// \return The math formula.
	/// \undoable
	const QString& neighborExpression(int index = 0) const {
		if(index < 0 || index >= neighborExpressions().size())
			throwException("Property component index is out of range.");
		return neighborExpressions()[index];
	}

	/// \brief Sets the number of vector components of the property to compute.
	/// \param componentCount The number of vector components.
	/// \undoable
	virtual void setComponentCount(int componentCount) override;

	/// Creates a computation engine that will compute the property values.
	virtual std::shared_ptr<ComputePropertyModifierDelegate::PropertyComputeEngine> createEngine(
				TimePoint time, 
				const PipelineFlowState& input,
				PropertyPtr outputProperty, 
				ConstPropertyPtr selectionProperty,
				QStringList expressions, 
				bool initializeOutputProperty) override;

private:

	/// Asynchronous compute engine that does the actual work in a separate thread.
	class ComputeEngine : public ComputePropertyModifierDelegate::PropertyComputeEngine
	{
	public:

		/// Constructor.
		ComputeEngine(
				const TimeInterval& validityInterval, 
				TimePoint time,
				PropertyPtr outputProperty, 
				ConstPropertyPtr selectionProperty,
				QStringList expressions, 
				int frameNumber, 
				const PipelineFlowState& input, 
				ConstPropertyPtr positions, 
				QStringList neighborExpressions,
				FloatType cutoff);
				
		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_neighborExpressions.clear();
			_neighborEvaluator.reset();
			PropertyComputeEngine::cleanup();
		}

		/// Returns the list of available input variables for the expressions managed by the delegate.
		virtual QStringList delegateInputVariableNames() const override;

		/// Determines whether any of the math expressions is explicitly time-dependent.
		virtual bool isTimeDependent() override;

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Indicates whether contributions from particle neighbors are taken into account.
		bool neighborMode() const { return _cutoff != 0; }

		/// Injects the computed results into the data pipeline.
		virtual PipelineFlowState emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

	private:

		const FloatType _cutoff;
		QStringList _neighborExpressions;
		ConstPropertyPtr _positions;
		std::unique_ptr<ParticleExpressionEvaluator> _neighborEvaluator;
		ParticleOrderingFingerprint _inputFingerprint;
	};

	/// Controls whether the contributions from neighbor terms are included in the computation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, neighborModeEnabled, setNeighborModeEnabled);

	/// The math expressions for calculating the neighbor-terms of the property function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, neighborExpressions, setNeighborExpressions);

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether multi-line input fields are shown in the UI for the expressions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useMultilineFields, setUseMultilineFields);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
