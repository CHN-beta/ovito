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
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/simcell/SimulationCellVis.h>
#include "StandardFrameLoader.h"

namespace Ovito { namespace StdObj {

/******************************************************************************
* Returns the simulation cell object, newly creating it first if necessary.
******************************************************************************/
SimulationCellObject* StandardFrameLoader::simulationCell()
{
	if(!_simulationCell) {
		_simulationCell = state().getMutableObject<SimulationCellObject>();
		if(!_simulationCell) {
			_simulationCell = state().createObject<SimulationCellObject>(dataSource(), executionContext(), AffineTransformation::Zero(), true, true, true, false);
			_isSimulationCellNewlyCreated = true;
		}
	}
	return _simulationCell;
}

/******************************************************************************
* Registers a new numeric element type with the given ID and an optional name string.
******************************************************************************/
const ElementType* StandardFrameLoader::addNumericType(const PropertyContainerClass& containerClass, PropertyObject* typedProperty, int id, const QString& name, OvitoClassPtr elementTypeClass)
{
	if(const ElementType* existingType = typedProperty->elementType(id))
		return existingType;

	// If the caller did not specify an element type class, let the PropertyConatiner class 
	// determine the right element type class for the given property.
	if(elementTypeClass == nullptr) {
		elementTypeClass = containerClass.typedPropertyElementClass(typedProperty->type());
		if(elementTypeClass == nullptr)
			elementTypeClass = &ElementType::OOClass();
	}
	OVITO_ASSERT(elementTypeClass->isDerivedFrom(ElementType::OOClass()));

	DataOORef<ElementType> elementType = static_object_cast<ElementType>(elementTypeClass->createInstance(dataset(), executionContext()));
	elementType->setNumericId(id);
	elementType->setName(name);
	elementType->initializeObject(executionContext());
	elementType->initializeType(PropertyReference(&containerClass, typedProperty), executionContext());

	// Log in type name assigned by the file reader as default value for the element type.
	// This is needed for the Python code generator to detect manual changes subsequently made by the user.
	elementType->freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(ElementType::name)});

	// Add the new element type to the type list managed by the property.
	return typedProperty->addElementType(std::move(elementType));
}

/******************************************************************************
* Finalizes the data loaded by a sub-class.
******************************************************************************/
void StandardFrameLoader::loadFile()
{
	// Only initialize the vis element once, when it was newly created.
	if(isSimulationCellNewlyCreated()) {
		// Set up the vis element for the simulation cell.
		if(SimulationCellVis* cellVis = dynamic_object_cast<SimulationCellVis>(simulationCell()->visElement())) {
			// Choose an appropriate line width that depends on the cell's size.
			FloatType cellDiameter = (
					simulationCell()->cellMatrix().column(0) +
					simulationCell()->cellMatrix().column(1) +
					simulationCell()->cellMatrix().column(2)).length();
			cellVis->setCellLineWidth(std::max(cellDiameter * FloatType(1.4e-3), FloatType(1e-8)));
			// Take a snapshot of the object's parameter values, which serves as reference to detect future changes made by the user.
			cellVis->freezeInitialParameterValues({SHADOW_PROPERTY_FIELD(SimulationCellVis::cellLineWidth)});
		}
	}

	// Log in 2d/3d flag and PBC flags set by the file reader as default values for the simulation cell.
	// This is needed for the Python code generator to detect manual changes subsequently made by the user.
	if(_simulationCell) {
		_simulationCell->freezeInitialParameterValues({
			SHADOW_PROPERTY_FIELD(SimulationCellObject::pbcX), 
			SHADOW_PROPERTY_FIELD(SimulationCellObject::pbcY), 
			SHADOW_PROPERTY_FIELD(SimulationCellObject::pbcZ), 
			SHADOW_PROPERTY_FIELD(SimulationCellObject::is2D)});
	}
}

}	// End of namespace
}	// End of namespace
