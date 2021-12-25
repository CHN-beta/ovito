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


#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>

#include <muparser/muParser.h>

namespace Ovito::StdObj {

/**
 * \brief Helper class that evaluates one or more math expressions for every data element.
 */
class OVITO_STDOBJ_EXPORT PropertyExpressionEvaluator
{
	Q_DECLARE_TR_FUNCTIONS(PropertyExpressionEvaluator);

public:

	/// Constructor.
	PropertyExpressionEvaluator() = default;

	/// Destructor.
	virtual ~PropertyExpressionEvaluator() {}

	/// Specifies the expressions to be evaluated for each element and creates the input variables.
	virtual void initialize(const QStringList& expressions, const PipelineFlowState& state, const ConstDataObjectPath& containerPath, int animationFrame = 0);

	/// Initializes the parser object and evaluates the expressions for every element.
	void evaluate(const std::function<void(size_t,size_t,double)>& callback, const std::function<bool(size_t)>& filter = std::function<bool(size_t)>());

	/// Returns the maximum number of threads used to evaluate the expression (or 0 if all processor cores are used).
	size_t maxThreadCount() const { return _maxThreadCount; }

	/// Sets The maximum number of threads used to evaluate the expression (or 0 if all processor cores should be used).
	void setMaxThreadCount(size_t count) { _maxThreadCount = count; }

	/// Returns the number of input data element.
	size_t elementCount() const { return _elementCount; }

	/// Returns the list of expressions.
	const std::vector<mu::string_type>& expressions() const { return _expressions; }

	/// Returns the list of available input variables.
	QStringList inputVariableNames() const;

	/// Returns a human-readable text listing the input variables.
	virtual QString inputVariableTable() const;

	/// Returns the stored simulation cell information.
	const SimulationCellObject* simCell() const { return _simCell; }

	/// Sets the name of the variable that provides the index of the current element.
	void setIndexVarName(QString name) { _indexVarName = std::move(name); }

	/// Returns the name of the variable that provides the index of the current element.
	const QString& indexVarName() const { return _indexVarName; }

	/// Returns whether a variable is being referenced in one of the expressions.
	bool isVariableUsed(const mu::char_type* varName);

	/// Returns whether the expression depends on animation time.
	bool isTimeDependent() { return isVariableUsed(_T("Frame")); }

	/// Registers a new input variable whose value is recomputed for each data element.
	void registerComputedVariable(const QString& variableName, std::function<double(size_t)> function, QString description = QString(), int variableClass = 0) {
		ExpressionVariable v;
		v.type = DERIVED_PROPERTY;
		v.name = convertQString(variableName);
		v.function = std::move(function);
		v.description = std::move(description);
		v.variableClass = variableClass;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is uniform.
	void registerGlobalParameter(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = GLOBAL_PARAMETER;
		v.name = convertQString(variableName);
		v.value = value;
		v.description = std::move(description);
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is constant.
	void registerConstant(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = CONSTANT;
		v.name = convertQString(variableName);
		v.value = value;
		v.description = std::move(description);
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is reflects the current element index.
	void registerIndexVariable(const QString& variableName, int variableClass, QString description = QString()) {
		ExpressionVariable v;
		v.type = ELEMENT_INDEX;
		v.name = convertQString(variableName);
		v.variableClass = variableClass;
		v.description = std::move(description);
		addVariable(std::move(v));
	}

	/// Registers a list of expression variables that refer to input properties.
	void registerPropertyVariables(const std::vector<ConstPropertyPtr>& inputProperties, int variableClass, const mu::char_type* namePrefix = nullptr);

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
	struct OVITO_STDOBJ_EXPORT ExpressionVariable {
		/// Indicates whether this variable has been successfully registered with the muParser.
		bool isRegistered = false;
		/// Indicates whether this variable is referenced by at least one of the expressions.
		bool isReferenced = false;
		/// The variable's value for the current data element.
		double value;
		/// Pointer into the property storage.
		const uint8_t* dataPointer;
		/// Data array stride in the property storage.
		size_t stride;
		/// The type of variable.
		ExpressionVariableType type;
		/// The original name of the variable.
		mu::string_type name;
		/// The name of the variable as registered with the muparser.
		mu::string_type mangledName;
		/// Human-readable description.
		QString description;
		/// A function that computes the variable's value for each data element.
		std::function<double(size_t)> function;
		/// Array with the property values.
		ConstPropertyAccessAndRef<void,true> propertyArray;
		/// Indicates whether this variable is a caller-defined element variable.
		int variableClass = 0;

		/// Retrieves the value of the variable and stores it in the memory location passed to muparser.
		void updateValue(size_t elementIndex);
	};

public:

	/// One instance of this class is created per thread.
	class OVITO_STDOBJ_EXPORT Worker {
	public:

		/// Initializes the worker instance.
		Worker(PropertyExpressionEvaluator& evaluator);

		// Make class noncopyable.
		Worker(const Worker&) = delete;
		Worker& operator=(const Worker&) = delete;

		/// Evaluates the expression for a specific data element and a specific vector component.
		double evaluate(size_t elementIndex, size_t component);

		/// Returns the storage address of a variable value.
		double* variableAddress(const mu::char_type* varName) {
			for(ExpressionVariable& var : _variables) {
				if(var.name == varName)
					return &var.value;
			}
			OVITO_ASSERT(false);
			return nullptr;
		}

		// Returns whether the given variable is being referenced in one of the expressions.
		bool isVariableUsed(const mu::char_type* varName) const {
			for(const ExpressionVariable& var : _variables)
				if(var.name == varName && var.isReferenced) return true;
			return false;
		}

		/// Updates the stored value of variables that depends on the current element index.
		void updateVariables(int variableClass, size_t elementIndex) {
			for(ExpressionVariable& v : _variables) {
				if(v.variableClass == variableClass)
					v.updateValue(elementIndex);
			}
		}

	private:

		/// The worker routine.
		void run(size_t startIndex, size_t endIndex, std::function<void(size_t,size_t,double)> callback, std::function<bool(size_t)> filter);

		// Paremt of this worker object.
		PropertyExpressionEvaluator& _evaluator;

		/// List of parser objects used by this thread.
		std::vector<mu::Parser> _parsers;

		/// List of input variables used by the parsers of this thread.
		std::vector<ExpressionVariable> _variables;

		/// The index of the last data element for which the expressions were evaluated.
		size_t _lastElementIndex = std::numeric_limits<size_t>::max();

		/// Error message reported by one of the parser objects (remains empty on success).
		QString _errorMsg;

		friend class PropertyExpressionEvaluator;
	};

protected:

	/// Initializes the list of input variables from the given input state.
	virtual void createInputVariables(const std::vector<ConstPropertyPtr>& inputProperties, const SimulationCellObject* simCell, const QVariantMap& attributes, int animationFrame);

	/// Updates the stored value of variables that depends on the current element index.
	virtual void updateVariables(Worker& worker, size_t elementIndex) {
		worker.updateVariables(0, elementIndex);
	}

	/// Helper function for converting a muParser string to a Qt string.
	static QString convertMuString(const mu::string_type& str) {
#if defined(_UNICODE)
		return QString::fromStdWString(str);
#else
		return QString::fromStdString(str);
#endif	
	}

	/// Helper function for converting a Qt string to a muParser string.
	static mu::string_type convertQString(const QString& str) {
#if defined(_UNICODE)
		return str.toStdWString();
#else
		return str.toStdString();
#endif	
	}

	/// Registers an input variable if the name does not exist yet.
	size_t addVariable(ExpressionVariable v);

	/// The list of expression that should be evaluated for each data element.
	std::vector<mu::string_type> _expressions;

	/// The list of input variables that can be referenced in the expressions.
	std::vector<ExpressionVariable> _variables;

	/// Indicates whether the list of referenced variables has been determined.
	bool _referencedVariablesKnown = false;

	/// The number of input data elements.
	size_t _elementCount = 0;

	/// List of characters allowed in variable names.
	static mu::string_type _validVariableNameChars;

	/// The maximum number of threads used to evaluate the expression.
	size_t _maxThreadCount = 0;

	/// The name of the variable that provides the index of the current element.
	QString _indexVarName;

	/// Human-readable name describing the data elements, e.g. "particles".
	QString _elementDescriptionName;

	/// The simulation cell information.
	DataOORef<const SimulationCellObject> _simCell;
};

}	// End of namespace
