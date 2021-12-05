////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/crystalanalysis/data/DislocationNetwork.h>
#include <ovito/crystalanalysis/objects/MicrostructurePhase.h>
#include <ovito/stdobj/simcell/PeriodicDomainDataObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>

namespace Ovito::CrystalAnalysis {

/**
 * \brief Stores a collection of dislocation segments.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationNetworkObject : public PeriodicDomainDataObject
{
	OVITO_CLASS(DislocationNetworkObject)
	Q_CLASSINFO("DisplayName", "Dislocations");

public:

	/// \brief Constructor.
	Q_INVOKABLE DislocationNetworkObject(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ObjectInitializationHints hints) override;		

	/// Returns the data encapsulated by this object after making sure it is not shared with other owners.
	const std::shared_ptr<DislocationNetwork>& modifiableStorage();

	/// Returns the list of dislocation segments.
	const std::vector<DislocationSegment*>& segments() const { return storage()->segments(); }

	/// Returns the list of dislocation segments.
	const std::vector<DislocationSegment*>& modifiableSegments() { return modifiableStorage()->segments(); }

	/// Adds a new crystal structures to the list.
	void addCrystalStructure(const MicrostructurePhase* structure) { _crystalStructures.push_back(this, PROPERTY_FIELD(crystalStructures), structure); }

	/// Removes a crystal structure.
	void removeCrystalStructure(int index) { _crystalStructures.remove(this, PROPERTY_FIELD(crystalStructures), index); }

	/// Returns the crystal structure with the given ID, or null if no such structure exists.
	const MicrostructurePhase* structureById(int id) const {
		for(const MicrostructurePhase* stype : crystalStructures())
			if(stype->numericId() == id)
				return stype;
		return nullptr;
	}

	/// Returns the crystal structure with the given name, or null if no such structure exists.
	const MicrostructurePhase* structureByName(const QString& name) const {
		for(const MicrostructurePhase* stype : crystalStructures())
			if(stype->name() == name)
				return stype;
		return nullptr;
	}

	/// Returns whether this data object wants to be shown in the pipeline editor under the data source section.
	virtual bool showInPipelineEditor() const override { return true; }

	/// Creates an editable proxy object for this DataObject and synchronizes its parameters.
	virtual void updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const override;

private:

	/// The internal data.
	DECLARE_RUNTIME_PROPERTY_FIELD(std::shared_ptr<DislocationNetwork>, storage, setStorage);

	/// List of crystal structures.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(DataOORef<const MicrostructurePhase>, crystalStructures, setCrystalStructures);
};

}	// End of namespace
