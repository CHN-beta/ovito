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
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/stdobj/properties/PropertyReference.h>

namespace Ovito { namespace StdObj {

/**
 * \brief A data object type that consists of a set of data columns, which are typically used to generate 2d data plots.
 */
class OVITO_STDOBJ_EXPORT DataTable : public PropertyContainer
{
	/// Define a new property metaclass for data table property containers.
	class OVITO_STDOBJ_EXPORT OOMetaClass : public PropertyContainerClass
	{
	public:

		/// Inherit constructor from base class.
		using PropertyContainerClass::PropertyContainerClass;

		/// Creates a storage object for standard data table properties.
		virtual PropertyPtr createStandardPropertyInternal(DataSet* dataset, size_t elementCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath = {}) const override;

	protected:

		/// Is called by the system after construction of the meta-class instance.
		virtual void initialize() override;
	};

	Q_OBJECT
	OVITO_CLASS_META(DataTable, OOMetaClass);
	Q_CLASSINFO("DisplayName", "Data table");

public:

	/// \brief The list of standard data table properties.
	enum Type {
		UserProperty = PropertyObject::GenericUserProperty,	//< This is reserved for user-defined properties.
		XProperty = PropertyObject::FirstSpecificProperty,
		YProperty
	};

	enum PlotMode {
		None,
		Line,
		Histogram,
		BarChart,
		Scatter
	};
	Q_ENUMS(PlotMode);

	/// Constructor.
	Q_INVOKABLE DataTable(DataSet* dataset, PlotMode plotMode = Line, const QString& title = QString(), ConstPropertyPtr y = {}, ConstPropertyPtr x = {});

	/// Returns the property object containing the y-coordinates of the data points.
	const PropertyObject* getY() const { return getProperty(Type::YProperty); }

	/// Returns the property object containing the x-coordinates of the data points (may be NULL).
	const PropertyObject* getX() const { return getProperty(Type::XProperty); }

	/// Returns the data array containing the x-coordinates of the data points.
	/// If no explicit x-coordinate data is available, the array is dynamically generated
	/// from the x-axis interval set for this data table.
	ConstPropertyPtr getXValues() const;

private:

	/// The lower bound of the x-interval of the histogram if data points have no explicit x-coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, intervalStart, setIntervalStart, PROPERTY_FIELD_DATA_OBJECT);

	/// The upper bound of the x-interval of the histogram if data points have no explicit x-coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, intervalEnd, setIntervalEnd, PROPERTY_FIELD_DATA_OBJECT);

	/// The label of the x-axis (optional).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(QString, axisLabelX, setAxisLabelX, PROPERTY_FIELD_DATA_OBJECT);

	/// The label of the y-axis (optional).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(QString, axisLabelY, setAxisLabelY, PROPERTY_FIELD_DATA_OBJECT);

	/// The plotting mode for this data table.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(PlotMode, plotMode, setPlotMode, PROPERTY_FIELD_DATA_OBJECT);
};

/**
 * Encapsulates a reference to a data table property.
 */
using DataTablePropertyReference = TypedPropertyReference<DataTable>;

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::StdObj::DataTablePropertyReference);
Q_DECLARE_METATYPE(Ovito::StdObj::DataTable::PlotMode);
Q_DECLARE_TYPEINFO(Ovito::StdObj::DataTable::PlotMode, Q_PRIMITIVE_TYPE);
