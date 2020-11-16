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
#include <ovito/core/dataset/data/DataObjectReference.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "SimulationCellObject.h"
#include "SimulationCellVis.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(SimulationCellObject);
DEFINE_PROPERTY_FIELD(SimulationCellObject, cellMatrix);
DEFINE_PROPERTY_FIELD(SimulationCellObject, pbcX);
DEFINE_PROPERTY_FIELD(SimulationCellObject, pbcY);
DEFINE_PROPERTY_FIELD(SimulationCellObject, pbcZ);
DEFINE_PROPERTY_FIELD(SimulationCellObject, is2D);
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, cellMatrix, "Cell matrix");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, pbcX, "Periodic boundary conditions (X)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, pbcY, "Periodic boundary conditions (Y)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, pbcZ, "Periodic boundary conditions (Z)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, is2D, "2D");
SET_PROPERTY_FIELD_UNITS(SimulationCellObject, cellMatrix, WorldParameterUnit);

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void SimulationCellObject::initializeObject(Application::ExecutionContext executionContext)
{
	// Attach a visualization element for rendering the simulation box.
	if(!visElement())
		setVisElement(OORef<SimulationCellVis>::create(dataset(), executionContext));

	DataObject::initializeObject(executionContext);
}

/******************************************************************************
* Computes the inverse of the cell matrix.
******************************************************************************/
void SimulationCellObject::computeInverseMatrix() const
{
	if(!is2D()) {
		cellMatrix().inverse(_reciprocalSimulationCell);
	}
	else {
		_reciprocalSimulationCell.setIdentity();
		FloatType det = cellMatrix()(0,0) * cellMatrix()(1,1) - cellMatrix()(0,1) * cellMatrix()(1,0);
		bool isValid = (std::abs(det) > FLOATTYPE_EPSILON);
		if(isValid) {
			_reciprocalSimulationCell(0,0) = cellMatrix()(1,1) / det;
			_reciprocalSimulationCell(1,0) = -cellMatrix()(1,0) / det;
			_reciprocalSimulationCell(0,1) = -cellMatrix()(0,1) / det;
			_reciprocalSimulationCell(1,1) = cellMatrix()(0,0) / det;
			_reciprocalSimulationCell.translation().x() = -(_reciprocalSimulationCell(0,0) * cellMatrix().translation().x() + _reciprocalSimulationCell(0,1) * cellMatrix().translation().y());
			_reciprocalSimulationCell.translation().y() = -(_reciprocalSimulationCell(1,0) * cellMatrix().translation().x() + _reciprocalSimulationCell(1,1) * cellMatrix().translation().y());
		}
	}
	_isReciprocalMatrixValid = true;
}

/******************************************************************************
* Is called when the value of a non-animatable field of this object changes.
******************************************************************************/
void SimulationCellObject::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(cellMatrix) || field == PROPERTY_FIELD(is2D)) {
		invalidateReciprocalCellMatrix();
	}
	DataObject::propertyChanged(field);
}

/******************************************************************************
* Creates an editable proxy object for this DataObject and synchronizes its parameters.
******************************************************************************/
void SimulationCellObject::updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const
{
	OVITO_ASSERT(this == dataPath.back());

	if(SimulationCellObject* proxy = static_object_cast<SimulationCellObject>(editableProxy())) {
		// Synchronize the actual data object with the editable proxy object.
		if(pbcFlags() != proxy->pbcFlags() || is2D() != proxy->is2D()) {
			// Make this data object mutable first.
			SimulationCellObject* self = static_object_cast<SimulationCellObject>(state.makeMutableInplace(dataPath));
			
			self->setPbcFlags(proxy->pbcFlags());
			self->setIs2D(proxy->is2D());
		}
	}
	else {
		// Create and initialize a new proxy.
		OORef<SimulationCellObject> newProxy = OORef<SimulationCellObject>::create(dataset(), Application::ExecutionContext::Scripting);
		newProxy->setPbcFlags(pbcFlags());
		newProxy->setIs2D(is2D());
		newProxy->setCellMatrix(cellMatrix());

		// Make this data object mutable and attach the proxy object to it.
		state.makeMutableInplace(dataPath)->setEditableProxy(std::move(newProxy));
	}

	DataObject::updateEditableProxies(state, dataPath);
}

/******************************************************************************
* Puts the simulation cell into a writable state.
* In the writable state, the Python binding layer will allow write access to
* the cell's internal data.
******************************************************************************/
void SimulationCellObject::makeWritableFromPython()
{
	if(!isSafeToModify())
		throwException(tr("Modifying the cell is not allowed, because the SimulationCell object is currently shared by more than one data collection. "
							"Please explicitly request a mutable version of the SimulationCell object by using the '_' notation."));
	_isWritableFromPython++;
}

}	// End of namespace
}	// End of namespace
