////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 Alexander Stukowski
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
#include <ovito/core/oo/OvitoObject.h>
#include <ovito/core/dataset/DataSet.h>
#include "ObjectSaveStream.h"

namespace Ovito {

/******************************************************************************
* The destructor closes the stream.
******************************************************************************/
ObjectSaveStream::~ObjectSaveStream()
{
	try {
		ObjectSaveStream::close();
	}
	catch(Exception& ex) {
		if(!ex.context()) ex.setContext(_dataset);
		ex.reportError();
	}
}

/******************************************************************************
* Saves an object with runtime type information to the stream.
******************************************************************************/
void ObjectSaveStream::saveObject(const OvitoObject* object, bool excludeRecomputableData)
{
	if(object == nullptr) {
		*this << (quint32)0;
	}
	else {
		// Instead of saving the object's data, we only assign a unique instance ID to the object here
		// and write that ID to the stream. The object itself will get saved later when the stream
		// is being closed.
		OVITO_CHECK_OBJECT_POINTER(object);
		OVITO_ASSERT(_objects.size() == _objectMap.size());
		quint32& id = _objectMap[object];
		if(id == 0) {
			_objects.push_back({object, excludeRecomputableData});
			id = (quint32)_objects.size();

			if(object->getOOClass() == DataSet::OOClass())
				_dataset = const_cast<DataSet*>(static_object_cast<DataSet>(object));

			OVITO_ASSERT(_dataset == nullptr || !object->getOOClass().isDerivedFrom(RefTarget::OOClass()) || static_object_cast<RefTarget>(object)->dataset() == _dataset);
		}
		else {
			OVITO_ASSERT(_objects[id-1].object == object);
			if(!excludeRecomputableData) {
				_objects[id-1].excludeRecomputableData = false;
			}
		}
		*this << id;
	}
}

/******************************************************************************
* Closes the stream.
******************************************************************************/
void ObjectSaveStream::close()
{
	if(!isOpen())
		return;

	try {
		// Byte offsets of object instances.
		std::vector<qint64> objectOffsets;

		// Serialize the data of each object.
		// Note: Not using range-based for-loop here, because additional objects may be appended to the end of the list
		// as we save objects which are already in the list.
		beginChunk(0x100);
		for(size_t i = 0; i < _objects.size(); i++) { // NOLINT(modernize-loop-convert)
			OVITO_CHECK_OBJECT_POINTER(_objects[i].object);
			objectOffsets.push_back(filePosition());
			_objects[i].object->saveToStream(*this, _objects[i].excludeRecomputableData);
		}
		endChunk();

		// Save the class of each object instance.
		qint64 classTableStart = filePosition();
		std::map<OvitoClassPtr, quint32> classes;
		beginChunk(0x200);
		for(const auto& record : _objects) {
			OvitoClassPtr clazz = &record.object->getOOClass();
			if(classes.find(clazz) == classes.end()) {
				classes.insert(std::make_pair(clazz, (quint32)classes.size()));
				// Write the basic runtime type information (name and plugin ID) of the class to the stream.
				beginChunk(0x201);
				OvitoClass::serializeRTTI(*this, clazz);
				endChunk();
				// Let the metaclass save additional information like for example the list of property fields defined
				// for RefMaker-derived classes.
				beginChunk(0x202);
				clazz->saveClassInfo(*this);
				endChunk();
			}
		}
		endChunk();

		// Save object table.
		qint64 objectTableStart = filePosition();
		beginChunk(0x300);
		auto offsetIterator = objectOffsets.cbegin();
		for(const auto& record : _objects) {
			*this << classes[&record.object->getOOClass()];
			*this << *offsetIterator++;
		}
		endChunk();

		// Write index of tables.
		*this << classTableStart << (quint32)classes.size();
		*this << objectTableStart << (quint32)_objects.size();
	}
	catch(...) {
		SaveStream::close();
		throw;
	}
	SaveStream::close();
}

}	// End of namespace
