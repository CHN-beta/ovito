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


#include <plugins/stdobj/StdObj.h>
#include <core/dataset/pipeline/PipelineFlowState.h>

#include <muParser.h>
#include <boost/utility.hpp>

namespace Ovito { namespace StdObj {

/**
 * \brief Helper class that evaluates one or more math expressions for every data element.
 */
class OVITO_STDOBJ_EXPORT PropertyExpressionEvaluator
{
	Q_DECLARE_TR_FUNCTIONS(PropertyExpressionEvaluator);

public:

	/// \brief Constructor.
	PropertyExpressionEvaluator() = default;

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const PipelineFlowState& inputState, const PropertyClass& propertyClass, int animationFrame = 0);

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame = 0);

	/// Initializes the parser object and evaluates the expressions for every particle.
	void evaluate(const std::function<void(size_t,size_t,double)>& callback, const std::function<bool(size_t)>& filter = std::function<bool(size_t)>());

	/// Returns the maximum number of threads used to evaluate the expression (or 0 if all processor cores are used).
	size_t maxThreadCount() const { return _maxThreadCount; }

	/// Sets The maximum number of threads used to evaluate the expression (or 0 if all processor cores should be used).
	void setMaxThreadCount(size_t count) { _maxThreadCount = count; }

	/// Returns the number of input data element.
	size_t elementCount() const { return _elementCount; }

	/// Returns the list of expressions.
	const std::vector<std::string>& expression() const { return _expressions; }

	/// Returns the list of available input variables.
	QStringList inputVariableNames() const;

	/// Returns a human-readable text listing the input variables.
	QString inputVariableTable() const;

	/// Sets the name of the variable that provides the index of the current element.
	void setIndexVarName(std::string name) { _indexVarName = std::move(name); }

	/// Returns whether the expression results depend on animation time.
	bool isTimeDependent() const { return _isTimeDependent; }

	/// Registers a new input variable whose value is recomputed for each data element.
	template<typename Function>
	void registerComputedVariable(const QString& variableName, Function&& function, QString description = QString()) {
		ExpressionVariable v;
		v.type = DERIVED_PROPERTY;
		v.name = variableName.toStdString();
		v.function = std::forward<Function>(function);
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is uniform.
	void registerGlobalParameter(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = GLOBAL_PARAMETER;
		v.name = variableName.toStdString();
		v.value = value;
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is constant.
	void registerConstant(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = CONSTANT;
		v.name = variableName.toStdString();
		v.value = value;
		v.description = description;
		addVariable(std::move(v));
	}

protected:

	enum ExpressionVariableType {
		FLOAT_PROPERTY,
		INT_PROPERTY,
		INT64_PROPERTY,
		DERIVED_PROPERTY,
		ELEMENT_INDEX,
		GLOBAL_PARAMETER,
		CONSTANT
	};

	/// Data structure representing an input variable.
	struct ExpressionVariable {
		/// The variable's value for the current data element.
		double value;
		/// Pointer into the property storage.
		const char* dataPointer;
		/// Data array stride in the property storage.
		size_t stride;
		/// The type of variable.
		ExpressionVariableType type;
		/// The name of the variable.
		std::string name;
		/// Human-readable description.
		QString description;
		/// A function that computes the variable's value for each data element.
		std::function<double(size_t)> function;
		/// Reference the original property that contains the data.
		ConstPropertyPtr property;
	};

public:

	/// One instance of this class is created per thread.
	class Worker : boost::noncopyable {
	public:

		/// Initializes the worker instance.
		Worker(PropertyExpressionEvaluator& evaluator);

		/// Evaluates the expression for a specific data element and a specific vector component.
		double evaluate(size_t elementIndex, size_t component);

		/// Returns the storage address of a variable value.
		double* variableAddress(const char* varName) {
			for(ExpressionVariable& var : _inputVariables)
				if(var.name == varName)
					return &var.value;
			OVITO_ASSERT(false);
			return nullptr;
		}

		// Returns whether the given variable is being referenced in one of the expressions.
		bool isVariableUsed(const char* varName) const {
			for(const ExpressionVariable* var : _activeVariables)
				if(var->name == varName) return true;
			return false;
		}

	private:

		/// The worker routine.
		void run(size_t startIndex, size_t endIndex, std::function<void(size_t,size_t,double)> callback, std::function<bool(size_t)> filter);

		/// List of parser objects used by this thread.
		std::vector<mu::Parser> _parsers;

		/// List of input variables used by the parsers of this thread.
		QVector<ExpressionVariable> _inputVariables;

		/// List of input variables which are actually used in the expression.
		std::vector<ExpressionVariable*> _activeVariables;

		/// The index of the last data element for which the expressions were evaluated.
		size_t _lastElementIndex = std::numeric_limits<size_t>::max();

		/// Error message reported by one of the parser objects (remains empty on success).
		QString _errorMsg;

		friend class PropertyExpressionEvaluator;
	};

protected:

	/// Initializes the list of input variables from the given input state.
	virtual void createInputVariables(const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame);

	/// Registers an input variable if the name does not exist yet.
	void addVariable(ExpressionVariable&& v);

	/// The list of expression that should be evaluated for each data element.
	std::vector<std::string> _expressions;

	/// The list of input variables.
	QVector<ExpressionVariable> _inputVariables;

	/// Indicates that the expression produces time-dependent results.
	std::atomic<bool> _isTimeDependent{false};

	/// The number of input data elements.
	size_t _elementCount = 0;

	/// List of characters allowed in variable names.
	static QByteArray _validVariableNameChars;

	/// The maximum number of threads used to evaluate the expression.
	size_t _maxThreadCount = 0;

	/// The name of the variable that provides the index of the current element.
	std::string _indexVarName; 
};

}	// End of namespace
}	// End of namespace
