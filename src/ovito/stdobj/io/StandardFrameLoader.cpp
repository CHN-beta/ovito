////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
			_simulationCellNewlyCreated = _simulationCell;
		}
	}
	return _simulationCell;
}

/******************************************************************************
* Registers a new numeric element type with the given ID and an optional name string.
******************************************************************************/
const ElementType* StandardFrameLoader::addNumericType(PropertyObject* typedProperty, int id, const QString& name, const OvitoClass& elementTypeClass)
{
	if(const ElementType* existingType = typedProperty->elementType(id))
		return existingType;

	DataOORef<ElementType> elementType = static_object_cast<ElementType>(elementTypeClass.createInstance(dataset(), executionContext()));
	elementType->setNumericId(id);
	elementType->setName(name);
	elementType->initializeObject(executionContext());
	elementType->initializeType(typedProperty->type(), executionContext());

	return typedProperty->addElementType(std::move(elementType));
}

/******************************************************************************
* Finalizes the particle data loaded by a sub-class.
******************************************************************************/
void StandardFrameLoader::loadFile()
{
	if(_simulationCellNewlyCreated) {
		// Set up the vis element for the simulation cell.
		if(SimulationCellVis* cellVis = dynamic_object_cast<SimulationCellVis>(_simulationCellNewlyCreated->visElement())) {
			// Choose an appropriate line width that depends on the cell's size.
			FloatType cellDiameter = (
					_simulationCellNewlyCreated->cellMatrix().column(0) +
					_simulationCellNewlyCreated->cellMatrix().column(1) +
					_simulationCellNewlyCreated->cellMatrix().column(2)).length();
			cellVis->setDefaultCellLineWidth(std::max(cellDiameter * FloatType(1.4e-3), FloatType(1e-8)));
			cellVis->setCellLineWidth(cellVis->defaultCellLineWidth());
		}
	}
}

}	// End of namespace
}	// End of namespace
