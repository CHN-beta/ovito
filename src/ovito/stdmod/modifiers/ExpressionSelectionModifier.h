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

#pragma once


#include <ovito/stdmod/StdMod.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyExpressionEvaluator.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Base class for ExpressionSelectionModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT ExpressionSelectionModifierDelegate : public ModifierDelegate
{
	OVITO_CLASS(ExpressionSelectionModifierDelegate)
	Q_OBJECT

public:

	/// \brief Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;

	/// Returns the type of input property container that this delegate can process.
	PropertyContainerClassPtr inputContainerClass() const {
		return static_class_cast<PropertyContainer>(&getOOMetaClass().getApplicableObjectClass());
	}

	/// Returns the reference to the selected input property container for this delegate.
	PropertyContainerReference inputContainerRef() const {
		return PropertyContainerReference(inputContainerClass(), inputDataObject().dataPath(), inputDataObject().dataTitle());
	}

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;

	/// Creates and initializes the expression evaluator object.
	virtual std::unique_ptr<PropertyExpressionEvaluator> initializeExpressionEvaluator(const QStringList& expressions, const PipelineFlowState& inputState, const ConstDataObjectPath& containerPath, int animationFrame);
};

/**
 * \brief Selects elements according to a user-defined Boolean expression.
 */
class OVITO_STDMOD_EXPORT ExpressionSelectionModifier : public DelegatingModifier
{
	/// Give this modifier class its own metaclass.
	class ExpressionSelectionModifierClass : public DelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using DelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return ExpressionSelectionModifierDelegate::OOClass(); }
	};

	Q_OBJECT
	OVITO_CLASS_META(ExpressionSelectionModifier, ExpressionSelectionModifierClass)
	Q_CLASSINFO("DisplayName", "Expression selection");
	Q_CLASSINFO("Description", "Select particles or other elements using a user-defined criterion.");
	Q_CLASSINFO("ModifierCategory", "Selection");

public:

	/// Constructor.
	Q_INVOKABLE ExpressionSelectionModifier(DataSet* dataset);

	/// \brief Returns the list of available input variables.
	const QStringList& inputVariableNames() const { return _variableNames; }

	/// \brief Returns a human-readable text listing the input variables.
	const QString& inputVariableTable() const { return _variableTable; }

	/// \brief Stores the given information about the available input variables in the modifier.
	void setVariablesInfo(QStringList variableNames, QString variableTable) {
		if(variableNames != _variableNames || variableTable != _variableTable) {
			_variableNames = std::move(variableNames);
			_variableTable = std::move(variableTable);
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		}
	}

private:

	/// The user expression for selecting elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, expression, setExpression);

	/// The list of input variables during the last evaluation.
	QStringList _variableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _variableTable;
};

}	// End of namespace
}	// End of namespace
