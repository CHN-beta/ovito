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


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>

namespace Ovito::StdObj {

/**
 * \brief Base class for geometry objects that are embedded in a spatial domain that may be periodic.
 */
class OVITO_STDOBJ_EXPORT PeriodicDomainDataObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(PeriodicDomainDataObject)

public:

	/// Returns the spatial domain this geometry is embedded in after making sure it
	/// can safely be modified.
	SimulationCellObject* mutableDomain() {
		return makeMutable(domain());
	}

	/// Returns the display title of this object.
	virtual QString objectTitle() const override;

protected:

	/// \brief Constructor.
	PeriodicDomainDataObject(DataSet* dataset, const QString& title = {});

private:

	/// The spatial domain (possibly periodic) this geometry object is embedded in.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(DataOORef<const SimulationCellObject>, domain, setDomain, PROPERTY_FIELD_NO_SUB_ANIM);

	/// The planar cuts to be applied to geometry after its has been transformed into a non-periodic representation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QVector<Plane3>, cuttingPlanes, setCuttingPlanes);

	/// The assigned title of the data object, which is displayed in the user interface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);
};

}	// End of namespace
