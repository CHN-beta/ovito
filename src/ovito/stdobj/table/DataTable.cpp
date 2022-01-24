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
#include "DataTable.h"

namespace Ovito::StdObj {

IMPLEMENT_OVITO_CLASS(DataTable);
DEFINE_PROPERTY_FIELD(DataTable, intervalStart);
DEFINE_PROPERTY_FIELD(DataTable, intervalEnd);
DEFINE_PROPERTY_FIELD(DataTable, axisLabelX);
DEFINE_PROPERTY_FIELD(DataTable, axisLabelY);
DEFINE_PROPERTY_FIELD(DataTable, plotMode);

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void DataTable::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a DataTablePropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<DataTablePropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, DataTablePropertyReference>();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QMetaType::registerComparators<DataTablePropertyReference>();
#endif

	setPropertyClassDisplayName(tr("Data table"));
	setElementDescriptionName(QStringLiteral("points"));
	setPythonName(QStringLiteral("table"));

	const QStringList emptyList;
	registerStandardProperty(XProperty, QString(), PropertyObject::Float, emptyList);
	registerStandardProperty(YProperty, QString(), PropertyObject::Float, emptyList);
}

/******************************************************************************
* Creates a storage object for standard data table properties.
******************************************************************************/
PropertyPtr DataTable::OOMetaClass::createStandardPropertyInternal(DataSet* dataset, size_t elementCount, int type, DataBuffer::InitializationFlags flags, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;

	switch(type) {
	case XProperty:
	case YProperty:
		dataType = PropertyObject::Float;
		componentCount = 1;
		break;
	default:
		OVITO_ASSERT_MSG(false, "DataTable::createStandardProperty()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	return PropertyPtr::create(dataset, elementCount, dataType, componentCount, propertyName, flags, type, componentNames);
}

/******************************************************************************
* Constructor.
******************************************************************************/
DataTable::DataTable(DataSet* dataset, PlotMode plotMode, const QString& title, ConstPropertyPtr y, ConstPropertyPtr x) : PropertyContainer(dataset, title),
	_intervalStart(0),
	_intervalEnd(0),
	_plotMode(plotMode)
{
	OVITO_ASSERT(!x || !y || x->size() == y->size());
	if(x) {
		OVITO_ASSERT(x->type() == XProperty);
		addProperty(std::move(x));
	}
	if(y) {
		OVITO_ASSERT(y->type() == YProperty);
		addProperty(std::move(y));
	}
}

/******************************************************************************
* Returns the data array containing the x-coordinates of the data points.
* If no explicit x-coordinate data is available, the array is dynamically generated
* from the x-axis interval set for this data table.
******************************************************************************/
ConstPropertyPtr DataTable::getXValues() const
{
	if(const PropertyObject* xProperty = getX()) {
		return xProperty;
	}
	else if(const PropertyObject* yProperty = getY()) {
		if(elementCount() != 0 && (intervalStart() != 0 || intervalEnd() != 0)) {
			PropertyAccessAndRef<FloatType> xdata = OOClass().createStandardProperty(dataset(), elementCount(), XProperty);
			xdata.buffer()->setName(axisLabelX());
			FloatType binSize = (intervalEnd() - intervalStart()) / xdata.size();
			FloatType x = intervalStart() + binSize * FloatType(0.5);
			for(FloatType& v : xdata) {
				v = x;
				x += binSize;
			}
			return xdata.take();
		}
		else {
			PropertyAccessAndRef<qlonglong> xdata = OOClass().createUserProperty(dataset(), elementCount(), PropertyObject::Int64, 1, axisLabelX(), DataBuffer::NoFlags, DataTable::XProperty);
			std::iota(xdata.begin(), xdata.end(), (size_t)0);
			return xdata.take();
		}
	}
	return {};
}

}	// End of namespace
