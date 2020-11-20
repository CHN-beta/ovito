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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include "DislocationNetworkObject.h"
#include "DislocationVis.h"

namespace Ovito { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(DislocationNetworkObject);
DEFINE_PROPERTY_FIELD(DislocationNetworkObject, storage);
DEFINE_VECTOR_REFERENCE_FIELD(DislocationNetworkObject, crystalStructures);
SET_PROPERTY_FIELD_LABEL(DislocationNetworkObject, crystalStructures, "Crystal structures");

/// Holds a shared, empty instance of the DislocationNetwork class,
/// which is used in places where a default storage is needed.
/// This singleton instance is never modified.
static const std::shared_ptr<DislocationNetwork> defaultStorage = std::make_shared<DislocationNetwork>(std::make_shared<ClusterGraph>());

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationNetworkObject::DislocationNetworkObject(DataSet* dataset) : PeriodicDomainDataObject(dataset), _storage(defaultStorage)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void DislocationNetworkObject::initializeObject(ExecutionContext executionContext)
{
	// Attach a visualization element for rendering the dislocation lines.
	if(!visElement())
		setVisElement(OORef<DislocationVis>::create(dataset(), executionContext));

	// Create the "unidentified" structure.
	DataOORef<MicrostructurePhase> defaultStructure = DataOORef<MicrostructurePhase>::create(dataset(), executionContext);
	defaultStructure->setName(tr("Unidentified structure"));
	defaultStructure->setColor(Color(1,1,1));
	defaultStructure->addBurgersVectorFamily(DataOORef<BurgersVectorFamily>::create(dataset(), executionContext));
	addCrystalStructure(std::move(defaultStructure));

	PeriodicDomainDataObject::initializeObject(executionContext);
}

/******************************************************************************
* Returns the data encapsulated by this object after making sure it is not
* shared with other owners.
******************************************************************************/
const std::shared_ptr<DislocationNetwork>& DislocationNetworkObject::modifiableStorage()
{
	// Copy data storage on write if there is more than one reference to the storage.
	OVITO_ASSERT(storage());
	OVITO_ASSERT(storage().use_count() >= 1);
	if(storage().use_count() > 1)
		_storage.mutableValue() = std::make_shared<DislocationNetwork>(*storage());
	OVITO_ASSERT(storage().use_count() == 1);
	return storage();
}

}	// End of namespace
}	// End of namespace
