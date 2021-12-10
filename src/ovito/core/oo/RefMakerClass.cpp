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

#include <ovito/core/Core.h>
#include <ovito/core/oo/PropertyFieldDescriptor.h>
#include <ovito/core/oo/RefMaker.h>
#include <ovito/core/oo/OvitoObject.h>
#include <ovito/core/dataset/DataSet.h>
#include "RefMakerClass.h"

namespace Ovito {

/******************************************************************************
* Is called by the system after construction of the meta-class instance.
******************************************************************************/
void RefMakerClass::initialize()
{
	OvitoClass::initialize();
//	if(name() == "DataObject")
//		qDebug() << "RefMakerClass::initialize():" << name() << (void*)this;

	// Collect all property fields of the class hierarchy in one array.
	for(const RefMakerClass* clazz = this; clazz != &RefMaker::OOClass(); clazz = static_cast<const RefMakerClass*>(clazz->superClass())) {
		for(const PropertyFieldDescriptor* field = clazz->_firstPropertyField; field != nullptr; field = field->next()) {
			_propertyFields.push_back(field);
		}
	}
}

/******************************************************************************
* Searches for a property field defined in this class or one of its super classes.
******************************************************************************/
const PropertyFieldDescriptor* RefMakerClass::findPropertyField(const char* identifier, bool searchSuperClasses) const
{
	if(!searchSuperClasses) {
		for(const PropertyFieldDescriptor* field = _firstPropertyField; field; field = field->next())
			if(qstrcmp(field->identifier(), identifier) == 0) return field;
	}
	else {
		for(const PropertyFieldDescriptor* field : _propertyFields) {
			if(qstrcmp(field->identifier(), identifier) == 0) return field;
		}
	}
	return nullptr;
}

/******************************************************************************
* This method is called by the ObjectSaveStream class when saving one or more
* object instances of a class belonging to this metaclass.
******************************************************************************/
void RefMakerClass::saveClassInfo(SaveStream& stream) const
{
	OvitoClass::saveClassInfo(stream);

	// Serialize the list of property fields registered for this RefMaker-derived class.
	for(const PropertyFieldDescriptor* field : propertyFields()) {
		stream.beginChunk(0x01);
		stream << QByteArray::fromRawData(field->identifier(), qstrlen(field->identifier()));
		OvitoClass::serializeRTTI(stream, field->definingClass());
		stream << field->flags();
		stream << field->isReferenceField();
		if(field->isReferenceField()) {
			OvitoClass::serializeRTTI(stream, field->targetClass());
		}
		stream.endChunk();
	}

	// Property list terminator:
	stream.beginChunk(0x0);
	stream.endChunk();
}

/******************************************************************************
* This method is called by the ObjectLoadStream class when loading one or more object instances
* of a class belonging to this metaclass.
******************************************************************************/
void RefMakerClass::loadClassInfo(LoadStream& stream, OvitoClass::SerializedClassInfo* classInfo) const
{
	OvitoClass::loadClassInfo(stream, classInfo);

	for(;;) {
		quint32 chunkId = stream.openChunk();
		if(chunkId == 0x0) {
			stream.closeChunk();
			break;	// End of list
		}
		if(chunkId != 0x1)
			throw Exception(RefMaker::tr("File format is invalid. Failed to load property fields of class %1.").arg(classInfo->clazz->name()));

		SerializedClassInfo::PropertyFieldInfo fieldInfo;

		// Read serialized property field definition from input stream.
		stream >> fieldInfo.identifier;
		OvitoClassPtr definingClass = OvitoClass::deserializeRTTI(stream);
		OVITO_ASSERT(definingClass->isDerivedFrom(RefMaker::OOClass()));
		fieldInfo.definingClass = static_cast<const RefMakerClass*>(definingClass);
		stream >> fieldInfo.flags;
		stream >> fieldInfo.isReferenceField;
		fieldInfo.targetClass = fieldInfo.isReferenceField ? OvitoClass::deserializeRTTI(stream) : nullptr;
		stream.closeChunk();

		// Give object class the chance to override deserialization behavior for this property field.
		fieldInfo.customDeserializationFunction = overrideFieldDeserialization(fieldInfo);
		if(!fieldInfo.customDeserializationFunction) {

			// Verify consistency of serialized and runtime class hierarchy.
			if(!classInfo->clazz->isDerivedFrom(*fieldInfo.definingClass)) {
				qDebug() << "WARNING:" << classInfo->clazz->name() << "is not derived from" << fieldInfo.definingClass->name();
				throw Exception(RefMaker::tr("The class hierarchy stored in the file differs from the class hierarchy of the program."));
			}

			// Verify consistency  of serialized and runtime property field definition.
			fieldInfo.field = fieldInfo.definingClass->findPropertyField(fieldInfo.identifier.constData(), true);
			if(fieldInfo.field) {
				if(fieldInfo.field->isReferenceField() != fieldInfo.isReferenceField ||
						fieldInfo.field->isVector() != ((fieldInfo.flags & PROPERTY_FIELD_VECTOR) != 0) ||
						(fieldInfo.isReferenceField && !fieldInfo.targetClass->isDerivedFrom(*fieldInfo.field->targetClass())))
					throw Exception(RefMaker::tr("The type of stored property field '%1' in class %2 has changed.").arg(fieldInfo.identifier, fieldInfo.definingClass->name()));
			}
		}

		// Add property field to list of fields that will be deserialized for each instance of the object class. 
		static_cast<RefMakerClass::SerializedClassInfo*>(classInfo)->propertyFields.push_back(std::move(fieldInfo));
	}
}

}	// End of namespace
