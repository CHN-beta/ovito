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


#include <core/Core.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)


/**
 * \brief Base class for modifier delegates used by the AsynchronousDelegatingModifier class.
 */
class OVITO_CORE_EXPORT AsynchronousModifierDelegate : public RefTarget
{
public:

	/// Give asynchronous modifier delegates their own metaclass.
	class AsynchronousModifierDelegateClass : public RefTarget::OOMetaClass 
	{
	public:

		/// Inherit constructor from base class.
		using RefTarget::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier delegate can operate on the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const { 
			OVITO_ASSERT_MSG(false, "AsynchronousModifierDelegate::OOMetaClass::isApplicableTo()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the isApplicableTo() method.").arg(name()))); 
			return false;
		}

		/// \brief The name by which Python scripts can refer to this modifier delegate.
		virtual QString pythonDataName() const {
			OVITO_ASSERT_MSG(false, "AsynchronousModifierDelegate::OOMetaClass::pythonDataName()",
				qPrintable(QStringLiteral("Metaclass of modifier delegate class %1 does not override the pythonDataName() method.").arg(name()))); 
			return {};
		}
	};

	OVITO_CLASS_META(AsynchronousModifierDelegate, AsynchronousModifierDelegateClass)
	Q_OBJECT
	
protected:

	/// \brief Constructor.
	using RefTarget::RefTarget;

public:

	/// \brief Returns the modifier to which this delegate belongs.
	AsynchronousDelegatingModifier* modifier() const;
};

/**
 * \brief Base class for modifiers that delegate work to a ModifierDelegate object.
 */
class OVITO_CORE_EXPORT AsynchronousDelegatingModifier : public AsynchronousModifier
{
public:

	/// Give this modifier class its own metaclass.
	class DelegatingModifierClass : public ModifierClass 
	{
	public:

		/// Inherit constructor from base class.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const PipelineFlowState& input) const override;

		/// Return the metaclass of delegates for this modifier type. 
		virtual const AsynchronousModifierDelegate::OOMetaClass& delegateMetaclass() const { 
			OVITO_ASSERT_MSG(false, "AsynchronousDelegatingModifier::OOMetaClass::delegateMetaclass()",
				qPrintable(QStringLiteral("Delegating modifier class %1 does not define a corresponding delegate metaclass. "
				"You must override the delegateMetaclass() method in the modifier's metaclass.").arg(name()))); 
			return AsynchronousModifierDelegate::OOClass();
		}
	};
		
	OVITO_CLASS_META(AsynchronousDelegatingModifier, DelegatingModifierClass)
	Q_OBJECT
	
public:

	/// Constructor.
	AsynchronousDelegatingModifier(DataSet* dataset);

protected:

	/// Creates a default delegate for this modifier.
	/// This should be called from the modifier's constructor.
	void createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName);

protected:

	/// The modifier's delegate.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(AsynchronousModifierDelegate, delegate, setDelegate, PROPERTY_FIELD_ALWAYS_CLONE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
