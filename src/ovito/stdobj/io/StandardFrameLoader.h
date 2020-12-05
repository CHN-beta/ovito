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

#pragma once


#include <ovito/stdobj/StdObj.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>

namespace Ovito { namespace StdObj {

/**
 * \brief Base class for file parsers that load property objects and/or simulation cell definitions.
 */
class OVITO_STDOBJ_EXPORT StandardFrameLoader : public FileSourceImporter::FrameLoader
{
public:

	/// Constructor.
	using FileSourceImporter::FrameLoader::FrameLoader;

	/// Returns the simulation cell object, newly creating it first if necessary.
	SimulationCellObject* simulationCell();

	/// Returns true if the file reader has already loaded a simulation cell definition.
	bool hasSimulationCell() const { return _simulationCell != nullptr; }

	/// Registers a new numeric element type with the given ID and an optional name string.
	const ElementType* addNumericType(PropertyObject* typedProperty, int id, const QString& name, const OvitoClass& elementTypeClass);

	/// Registers a new named element type and automatically gives it a unique numeric ID.
	const ElementType* addNamedType(PropertyObject* typedProperty, const QString& name, const OvitoClass& elementTypeClass) {
		if(const ElementType* existingType = typedProperty->elementType(name))
			return existingType;
		return addNumericType(typedProperty, typedProperty->generateUniqueElementTypeId(), name, elementTypeClass);
	}

	/// Registers a new named element type and automatically gives it a unique numeric ID.
	const ElementType* addNamedType(PropertyObject* typedProperty, const QLatin1String& name, const OvitoClass& elementTypeClass) {
		if(const ElementType* existingType = typedProperty->elementType(name))
			return existingType;
		return addNumericType(typedProperty, typedProperty->generateUniqueElementTypeId(), name, elementTypeClass);
	}

protected:

	/// Finalizes the particle data loaded by a sub-class.
	virtual void loadFile() override;

private:

	/// The simulation cell object.
	SimulationCellObject* _simulationCell = nullptr;

	/// The simulation cell object if it was newly created by the importer.
	SimulationCellObject* _simulationCellNewlyCreated = nullptr;
};

}	// End of namespace
}	// End of namespace
