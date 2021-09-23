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

#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/rendering/RendererResourceCache.h>
#include "PropertyColorMapping.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(PropertyColorMapping);
DEFINE_REFERENCE_FIELD(PropertyColorMapping, colorGradient);
DEFINE_PROPERTY_FIELD(PropertyColorMapping, startValue);
DEFINE_PROPERTY_FIELD(PropertyColorMapping, endValue);
DEFINE_PROPERTY_FIELD(PropertyColorMapping, autoAdjustRange);
DEFINE_PROPERTY_FIELD(PropertyColorMapping, sourceProperty);
SET_PROPERTY_FIELD_LABEL(PropertyColorMapping, startValue, "Start value");
SET_PROPERTY_FIELD_LABEL(PropertyColorMapping, endValue, "End value");
SET_PROPERTY_FIELD_LABEL(PropertyColorMapping, colorGradient, "Color gradient");
SET_PROPERTY_FIELD_LABEL(PropertyColorMapping, autoAdjustRange, "Automatically adjust range");
SET_PROPERTY_FIELD_LABEL(PropertyColorMapping, sourceProperty, "Source property");

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyColorMapping::PropertyColorMapping(DataSet* dataset) : RefTarget(dataset),
	_startValue(0.0),
	_endValue(0.0),
	_autoAdjustRange(true)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void PropertyColorMapping::initializeObject(ExecutionContext executionContext)
{
	if(executionContext == ExecutionContext::Interactive) {
#ifndef OVITO_DISABLE_QSETTINGS
		// Load the default gradient type set by the user.
		QSettings settings;
		settings.beginGroup(PropertyColorMapping::OOClass().plugin()->pluginId());
		settings.beginGroup(PropertyColorMapping::OOClass().name());
		QString typeString = settings.value(PROPERTY_FIELD(colorGradient).identifier()).toString();
		if(!typeString.isEmpty()) {
			try {
				OvitoClassPtr gradientType = OvitoClass::decodeFromString(typeString);
				if(!colorGradient() || colorGradient()->getOOClass() != *gradientType) {
					OORef<ColorCodingGradient> gradient = dynamic_object_cast<ColorCodingGradient>(gradientType->createInstance(dataset(), executionContext));
					if(gradient) setColorGradient(std::move(gradient));
				}
			}
			catch(...) {}
		}
	#endif
	}

	// Select the rainbow color gradient by default.
	if(!colorGradient())
		setColorGradient(OORef<ColorCodingHSVGradient>::create(dataset(), executionContext));

	RefTarget::initializeObject(executionContext);
}

/******************************************************************************
* Creates a PseudoColorMapping that can be used for rendering of graphics primitives.
******************************************************************************/
PseudoColorMapping PropertyColorMapping::pseudoColorMapping(const PropertyObject* pseudoColorProperty, int pseudoColorPropertyComponent) const
{
	if(!autoAdjustRange()) {
		// Manual range control.
		return PseudoColorMapping(startValue(), endValue(), colorGradient());
	}
	else if(pseudoColorProperty && colorGradient()) {
		// Automatic range control. Need to determine min/max range of input property values.
		if(boost::optional<std::pair<FloatType, FloatType>> range = determineValueRange(pseudoColorProperty, pseudoColorPropertyComponent)) {
			return PseudoColorMapping(range->first, range->second, colorGradient());
		}
	}

	// Returns an invalid mapping.
	return {};
}

/******************************************************************************
* Determines the min/max range of values stored in the given property array.
******************************************************************************/
boost::optional<std::pair<FloatType, FloatType>> PropertyColorMapping::determineValueRange(const PropertyObject* pseudoColorProperty, int pseudoColorPropertyComponent) const
{
	if(!pseudoColorProperty)
		return {};
	OVITO_ASSERT(pseudoColorPropertyComponent >= 0 && pseudoColorPropertyComponent < pseudoColorProperty->componentCount());

	// Compute ranges for input properties are kept in an internal cache to speed up subsequent queries.

	// The tagged key used for caching:
	RendererResourceKey<PropertyColorMapping, ConstDataObjectRef, int> cacheKey(pseudoColorProperty, pseudoColorPropertyComponent);
	// The value type stored in the cache:
	struct CacheValue {
		FloatType minValue = std::numeric_limits<FloatType>::max();
		FloatType maxValue = std::numeric_limits<FloatType>::lowest();
	};
	// Perform cache lookup.
	auto& range = dataset()->visCache().get<CacheValue>(std::move(cacheKey));

	// Check if value returned by cache is already initialized. If not, we have to perform the range computation.
	if(range.minValue == std::numeric_limits<FloatType>::max()) {

		// Iterate over the property array to find the lowest/highest value.
		pseudoColorProperty->forEach(pseudoColorPropertyComponent, [&](size_t i, auto v) {
				if(v > range.maxValue) range.maxValue = v;
				if(v < range.minValue) range.minValue = v;
			});

		// Range may be degenerate if input property contains zero elements.
		if(range.minValue == std::numeric_limits<FloatType>::max())
			return {};

		// Clamp to finite range.
		if(!std::isfinite(range.minValue)) range.minValue = std::numeric_limits<FloatType>::lowest();
		if(!std::isfinite(range.maxValue)) range.maxValue = std::numeric_limits<FloatType>::max();
	}

	return std::make_pair(range.minValue, range.maxValue);
}

/******************************************************************************
* Swaps the minimum and maximum values to reverse the color scale.
******************************************************************************/
void PropertyColorMapping::reverseRange()
{
	FloatType oldStartValue = startValue();
	setStartValue(endValue());
	setEndValue(oldStartValue);
}

#ifdef OVITO_QML_GUI
/******************************************************************************
* Returns the class name of the selected color gradient.
******************************************************************************/
QString PropertyColorMapping::colorGradientType() const 
{
	return colorGradient() ? colorGradient()->getOOClass().name() : QString();
}

/******************************************************************************
* Assigns a new color gradient based on its class name.
******************************************************************************/
void PropertyColorMapping::setColorGradientType(const QString& typeName, ExecutionContext executionContext) 
{
	OvitoClassPtr descriptor = PluginManager::instance().findClass(QString(), typeName);
	if(!descriptor) {
		qWarning() << "setColorGradientType: Color gradient class" << typeName << "does not exist.";
		return;
	}
	OORef<ColorCodingGradient> gradient = static_object_cast<ColorCodingGradient>(descriptor->createInstance(dataset(), executionContext));
	if(gradient) {
		setColorGradient(std::move(gradient));
#ifndef OVITO_DISABLE_QSETTINGS
		QSettings settings;
		settings.beginGroup(PropertyColorMapping::OOClass().plugin()->pluginId());
		settings.beginGroup(PropertyColorMapping::OOClass().name());
		settings.setValue(PROPERTY_FIELD(PropertyColorMapping::colorGradient).identifier(),
				QVariant::fromValue(OvitoClass::encodeAsString(descriptor)));
#endif
	}
}
#endif

}	// End of namespace
}	// End of namespace
