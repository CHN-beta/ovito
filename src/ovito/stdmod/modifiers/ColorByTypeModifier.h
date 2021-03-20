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
#include <ovito/stdobj/properties/GenericPropertyModifier.h>
#include <ovito/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdMod {

/**
 * \brief Assigns colors to data elements based on a typed property.
 */
class OVITO_STDMOD_EXPORT ColorByTypeModifier : public GenericPropertyModifier
{
	Q_OBJECT
	OVITO_CLASS(ColorByTypeModifier)

#ifndef OVITO_BUILD_BASIC
	Q_CLASSINFO("DisplayName", "Color by type");
#else
	Q_CLASSINFO("DisplayName", "Color by type (Pro)");
#endif
	Q_CLASSINFO("Description", "Color data elements according to a typed property.");
	Q_CLASSINFO("ModifierCategory", "Coloring");

public:

	/// Constructor.
	Q_INVOKABLE ColorByTypeModifier(DataSet* dataset);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

private:

	/// The input type property that is used as data source for the selection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, sourceProperty, setSourceProperty);

	/// Controls whether the modifier assigns a color only to selected elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, colorOnlySelected, setColorOnlySelected);

	/// Controls whether the input selection is preserved or not. If true, the current selection is cleared by the modifier to reveal the assigned colors in the interactive viewports of OVITO.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, clearSelection, setClearSelection);
};

}	// End of namespace
}	// End of namespace