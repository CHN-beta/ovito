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

#include <ovito/core/Core.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/DataSet.h>
#include "AsynchronousDelegatingModifier.h"

namespace Ovito {

DEFINE_REFERENCE_FIELD(AsynchronousDelegatingModifier, delegate);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AsynchronousDelegatingModifier::AsynchronousDelegatingModifier(DataSet* dataset) : AsynchronousModifier(dataset)
{
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval AsynchronousDelegatingModifier::validityInterval(const ModifierEvaluationRequest& request) const
{
	TimeInterval iv = AsynchronousModifier::validityInterval(request);

	if(delegate() && delegate()->isEnabled())
		iv.intersect(delegate()->validityInterval(request));

	return iv;
}

/******************************************************************************
* Creates a default delegate for this modifier.
******************************************************************************/
void AsynchronousDelegatingModifier::createDefaultModifierDelegate(const OvitoClass& delegateType, const QString& defaultDelegateTypeName, ObjectInitializationHints initializationHints)
{
	OVITO_ASSERT(delegateType.isDerivedFrom(ModifierDelegate::OOClass()));

	// Find the delegate type that corresponds to the given name string.
	for(OvitoClassPtr clazz : PluginManager::instance().listClasses(delegateType)) {
		if(clazz->name() == defaultDelegateTypeName) {
			OORef<ModifierDelegate> delegate = static_object_cast<ModifierDelegate>(clazz->createInstance(dataset(), initializationHints));
			setDelegate(std::move(delegate));
			break;
		}
	}
	OVITO_ASSERT_MSG(delegate(), "AsynchronousDelegatingModifier::createDefaultModifierDelegate", qPrintable(QStringLiteral("There is no delegate class named '%1' inheriting from %2.").arg(defaultDelegateTypeName).arg(delegateType.name())));
}

/******************************************************************************
* Asks the metaclass whether the modifier can be applied to the given input data.
******************************************************************************/
bool AsynchronousDelegatingModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	if(!AsynchronousModifier::OOMetaClass::isApplicableTo(input)) return false;

	// Check if there is any modifier delegate that could handle the input data.
	for(const ModifierDelegate::OOMetaClass* clazz : PluginManager::instance().metaclassMembers<ModifierDelegate>(delegateMetaclass())) {
		if(clazz->getApplicableObjects(input).empty() == false)
			return true;
	}

	return false;
}

}	// End of namespace
