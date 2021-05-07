////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 OVITO GmbH, Germany
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


#include <ovito/core/Core.h>
#include <ovito/core/oo/OvitoClass.h>

namespace Ovito {

/**
 * \brief Meta-class for classes derived from RefMaker.
 */
class OVITO_CORE_EXPORT RefMakerClass : public OvitoClass
{
public:

	/// Structure with the serialized metadata for a RefMaker-derived class.
	struct SerializedClassInfo : public OvitoClass::SerializedClassInfo
	{
		/// Data structure loaded from an OVITO state file describing a single property field
		/// of a RefMaker-derived class that was serialized.
		struct PropertyFieldInfo
		{
			using CustomDeserializationFunctionPtr = void (*)(const PropertyFieldInfo& field, ObjectLoadStream& stream, RefMaker& owner);

			/// The identifier of the property field.
			QByteArray identifier;

			/// The RefMaker-derived class that owns the property field.
			const RefMakerClass* definingClass;

			/// The stored flags of the property field (see PropertyFieldFlag).
			int flags;

			/// Indicates whether this is a reference field or a property field.
			bool isReferenceField;

			/// If this is a reference field, this is its RefTarget-derived class.
			OvitoClassPtr targetClass;

			/// The property field of the defining class that matches the
			/// stored field. Can be NULL if the property field no longer exists.
			const PropertyFieldDescriptor* field;

			/// An optional pointer to a custom function that takes are of the deserialization
			/// of this property field.
			CustomDeserializationFunctionPtr customDeserializationFunction;
		};

		/// The list of property fields defined for the class when it was serialized.
		std::vector<PropertyFieldInfo> propertyFields;
	};

public:

	/// Constructor.
	RefMakerClass(const QString& name, OvitoClassPtr superClass, const char* pluginId, const QMetaObject* qtClassInfo) :
		OvitoClass(name, superClass, pluginId, qtClassInfo) {}

	/// Returns the list of property fields of the class, including those of all parent classes.
	const std::vector<const PropertyFieldDescriptor*>& propertyFields() const { return _propertyFields; }

	/// If this is the descriptor of a RefMaker-derived class then this method will return
	/// the reference field with the given identifier that has been defined in the RefMaker-derived
	/// class or one of its super classes. If no such field is defined, then NULL is returned.
	const PropertyFieldDescriptor* findPropertyField(const char* identifier, bool searchSuperClasses = false) const;

	/// This method is called by the ObjectSaveStream class when saving one or more object instances of
	/// a class belonging to this metaclass.
	virtual void saveClassInfo(SaveStream& stream) const override;

	/// This method is called by the ObjectLoadStream class when loading one or more object instances
	/// of a class belonging to this metaclass.
	virtual void loadClassInfo(LoadStream& stream, OvitoClass::SerializedClassInfo* classInfo) const override;

	/// \brief Creates a new instance of the SerializedClassInfo structure.
	virtual std::unique_ptr<OvitoClass::SerializedClassInfo> createClassInfoStructure() const override {
		return std::make_unique<RefMakerClass::SerializedClassInfo>();
	}

	/// Lets the object class provide a custom function that takes are of the deserialization of a serialized property field. 
	virtual SerializedClassInfo::PropertyFieldInfo::CustomDeserializationFunctionPtr overrideFieldDeserialization(const SerializedClassInfo::PropertyFieldInfo& field) const { return nullptr; }

protected:

	/// Is called by the system after construction of the meta-class instance.
	virtual void initialize() override;

private:

	/// Lists all property fields of the class, including those of all parent classes.
	std::vector<const PropertyFieldDescriptor*> _propertyFields;

	/// The linked list of property fields.
	const PropertyFieldDescriptor* _firstPropertyField = nullptr;

	friend class PropertyFieldDescriptor;
};

}	// End of namespace
