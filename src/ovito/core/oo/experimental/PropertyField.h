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


#include <ovito/core/Core.h>
#include <ovito/core/oo/OvitoObject.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/oo/PropertyFieldDescriptor.h>
#include <ovito/core/oo/ReferenceEvent.h>

#include <boost/type_traits/has_equal_to.hpp>

namespace Ovito {

namespace detail {

/**
 * \brief Base class for property fields of RefMaker derived classes.
 */
class OVITO_CORE_EXPORT PropertyFieldBase
{
protected:

	/// Generates a notification event to inform the dependents of the field's owner that it has changed.
	static void generateTargetChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor, ReferenceEvent::Type eventType = ReferenceEvent::TargetChanged);

	/// Generates a notification event to inform the dependents of the field's owner that it has changed.
	static void generatePropertyChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor) const;

	/// Indicates whether undo records should be created.
	static bool isUndoRecordingActive(RefMaker* owner, const PropertyFieldDescriptor& descriptor) const;

	/// Puts a record on the undo stack.
	static void pushUndoRecord(RefMaker* owner, std::unique_ptr<UndoableOperation>&& operation);

protected:

	/// This abstract undo record class keeps a strong reference object whose property has been changed.
	/// This is needed to keep the owner object alive as long as this undo record is on the undo stack.
	class OVITO_CORE_EXPORT PropertyFieldOperation : public UndoableOperation
	{
	public:
	
		/// Constructor.
		PropertyFieldOperation(RefMaker* owner, const PropertyFieldDescriptor& descriptor);
		
		/// Access to the object whose property was changed.
		RefMaker* owner() const;
		
		/// Access to the descriptor of the reference field whose value has changed.
		const PropertyFieldDescriptor& descriptor() const { return _descriptor; }
	
	private:

		/// The object whose property has been changed.
		/// This is only used if the owner is not the DataSet, because that would create a circular reference.
		OORef<OvitoObject> _owner;

		/// The descriptor of the reference field that has changed.
		const PropertyFieldDescriptor& _descriptor;
	};
};

/**
 * \brief Property field that stores a simple value that may not be serializable.
 */
template<typename T>
class RuntimePropertyField : public PropertyFieldBase
{
public:

	/// The type of value stored in this property field.
	using element_type = T;

	// Pick the QVariant data type used to wrap the property's C++ data type.
	// If the property's C++ type is an enum, then use 'int'.
	// If the property's C++ type is 'Color', then use 'QColor'.
	// Otherwise just use the property's C++ type directly.
	using qvariant_type = std::conditional_t<std::is_enum<T>::value, int, std::conditional_t<std::is_same<T, Color>::value, QColor, T>>;

	// Make sure that for any enum type the QVariant data type 'int' is used.
	static_assert(!std::is_enum<T>::value || std::is_same<qvariant_type, int>::value, "QVariant data type must be 'int' for enum property types.");
	// Make sure that for a color property the QVariant data type 'QColor' is used.
	static_assert(!std::is_same<T, Color>::value || std::is_same<qvariant_type, QColor>::value, "QVariant data type must be 'QColor' for color property types.");

	/// Forwarding constructor that initializes the property field's value.
	template<class... Args>
	explicit RuntimePropertyField(Args&&... args) : _value(std::forward<Args>(args)...) {}

	/// Changes the value of the property field. Handles undo and sends a notification message.
	template<typename U>
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, U&& newValue) {
		OVITO_CHECK_OBJECT_POINTER(owner);
		if(isEqualToCurrentValue(get(), newValue)) return;
		if(isUndoRecordingActive(owner, descriptor))
			pushUndoRecord(owner, std::make_unique<PropertyChangeOperation>(owner, *this, descriptor));
		mutableValue() = std::forward<U>(newValue);
		valueChangedInternal(owner, descriptor);
	}

	/// Sets the value of this property field to the value stored in the given QVariant.
	/// This method handles undo and sends a notification message.
	template<typename U = qvariant_type>
	std::enable_if_t<QMetaTypeId2<U>::Defined>
	setQVariant(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const QVariant& newValue) {
		OVITO_ASSERT_MSG(newValue.canConvert<qvariant_type>(), "RuntimePropertyField::setQVariant()", qPrintable(QString("The assigned QVariant value of type %1 cannot be converted to the data type %2 of the property field.").arg(newValue.typeName()).arg(QMetaType::typeName(qMetaTypeId<qvariant_type>()))));
		set(owner, descriptor, newValue.value<qvariant_type>());
	}

	/// Changes the value of the property. Handles undo and sends a notification message.
	template<typename U = qvariant_type>
	std::enable_if_t<!QMetaTypeId2<U>::Defined>
	setQVariant(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const QVariant& newValue) {
		OVITO_ASSERT_MSG(false, "RuntimePropertyField::setQVariant()", "The data type of the property field does not support conversion to/from QVariant.");
	}

	/// Returns the internal value stored in this property field as a QVariant.
	template<typename U = qvariant_type>
	std::enable_if_t<QMetaTypeId2<U>::Defined, QVariant>
	getQVariant() const {
		return QVariant::fromValue<qvariant_type>(static_cast<qvariant_type>(this->get()));
	}

	/// Returns the internal value stored in this property field as a QVariant.
	template<typename U = qvariant_type>
	std::enable_if_t<!QMetaTypeId2<T>::Defined, QVariant>
	getQVariant() const {
		OVITO_ASSERT_MSG(false, "RuntimePropertyField::getQVariant()", "The data type of the property field does not support conversion to/from QVariant.");
		return {};
	}

	/// Reads the current value of this property field.
	inline const T& get() const { return _value; }

	/// Returns a reference to the internal value storage of this property field, which allows to manipulate the
	/// value from the outside. WARNING: Do not use this function unless you know what you are doing!
	inline T& mutableValue() { return _value; }

	/// Conversion operator that reads out the current value of this property field.
	inline operator const T&() const { return get(); }

private:

	/// Internal helper function that generates notification events.
	static void valueChangedInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor) {
		generatePropertyChangedEvent(owner, descriptor);
		generateTargetChangedEvent(owner, descriptor);
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}

	/// Helper function that tests if the new value is equal to the current value of the property field.
	template<typename U = T>
	static inline std::enable_if_t<boost::has_equal_to<const U&>::value, bool>
	isEqualToCurrentValue(const U& oldValue, const U& newValue) { return oldValue == newValue; }

	/// Helper function that tests if the new value is equal to the current value of the property field.
	template<typename U = T>
	static inline std::enable_if_t<!boost::has_equal_to<const U&>::value, bool>
	isEqualToCurrentValue(const U& oldValue, const U& newValue) { return false; }

	/// This undo class records a change to the property value.
	class PropertyChangeOperation : public PropertyFieldOperation
	{
	public:

		/// Constructor.
		/// Makes a copy of the current property value.
		PropertyChangeOperation(RefMaker* owner, RuntimePropertyField& field, const PropertyFieldDescriptor& descriptor) :
			PropertyFieldOperation(owner, descriptor), _field(field), _oldValue(field.get()) {}

		/// Restores the old property value.
		virtual void undo() override {
			// Swap old value and current property value.
			using std::swap; // using ADL here
			swap(_field.mutableValue(), _oldValue);
			valueChangedInternal(owner(), descriptor());
		}

	private:

		/// The property field that has been changed.
		RuntimePropertyField& _field;
		/// The old value of the property.
		T _oldValue;
	};

	/// The internal storage of the property value.
	T _value;
};

/**
 * \brief Property field storing a simple value that is serializable.
 */
template<typename property_data_type>
class PropertyField : public RuntimePropertyField<property_data_type>
{
public:
	using property_type = property_data_type;

	/// Constructor.
	using RuntimePropertyField<property_data_type>::RuntimePropertyField;

	/// Saves the property's value to a stream.
	inline void saveToStream(SaveStream& stream) const {
		stream << this->get();
	}

	/// Loads the property's value from a stream.
	inline void loadFromStream(LoadStream& stream) {
		stream >> this->mutableValue();
	}
};

// Template specialization for serialization of property fields holding a size_t value.
template<>
inline void PropertyField<size_t>::saveToStream(SaveStream& stream) const {
	stream.writeSizeT(this->get());
}

// Template specialization for deserialization of property fields holding a size_t value.
template<>
inline void PropertyField<size_t>::loadFromStream(LoadStream& stream) {
	stream.readSizeT(this->mutableValue());
}

/**
 * \brief Manages a pointer to a RefTarget derived class held by a RefMaker derived class.
 */
class OVITO_CORE_EXPORT SingleReferenceFieldBase : public PropertyFieldBase
{
public:

	/// Constructor.
	SingleReferenceFieldBase() = default;

	/// Returns the RefTarget pointer.
	inline operator RefTarget*() const {
		return _pointer;
	}

	/// Returns the RefTarget pointer.
	inline RefTarget* getInternal() const {
		return _pointer;
	}

protected:

	/// Replaces the current reference target with a new target. Handles undo recording.
	void setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, RefTarget const*& inactiveTarget, bool generateNotificationEvents = true);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<const RefTarget>& inactiveTarget, bool generateNotificationEvents = true);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, DataOORef<const DataObject>& inactiveTarget, bool generateNotificationEvents = true);

	/// The actual pointer to the reference target.
	RefTarget* _pointer = nullptr;

	friend class RefMaker;
	friend class RefTarget;

private:

	template<typename ReferenceType>
	class SetReferenceOperation : public PropertyFieldOperation
	{
	private:

		/// The reference target that is currently not assigned to the reference field.
		/// This is stored here so that we can restore it on a call to undo().
		ReferenceType _inactiveTarget;

		/// The reference field whose value has changed.
		SingleReferenceFieldBase& _reffield;

	public:
		
		SetReferenceOperation(RefMaker* owner, ReferenceType oldTarget, SingleReferenceFieldBase& reffield, const PropertyFieldDescriptor& descriptor) :
			PropertyFieldOperation(owner, descriptor), _inactiveTarget(std::move(oldTarget)), _reffield(reffield) {}
		
		virtual void undo() override { 
			_reffield.swapReference(owner(), descriptor(), _inactiveTarget); 
		}

		virtual QString displayName() const override {
				return QStringLiteral("Setting reference field <%1> of %2 to object %3")
					.arg(descriptor().identifier())
					.arg(owner()->getOOClass().name())
					.arg(_inactiveTarget ? _inactiveTarget->getOOClass().name() : "<null>");
		}
	};
};

/**
 * \brief Templated version of the SingleReferenceFieldBase class.
 */
template<typename RefTargetType>
class ReferenceField : public SingleReferenceFieldBase
{
public:
	using target_type = RefTargetType;

#ifdef OVITO_DEBUG
	/// Destructor that releases all referenced objects.
	~ReferenceField() {
		if(_pointer)
			qDebug() << "Reference field value:" << _pointer;
		OVITO_ASSERT_MSG(!_pointer, "~ReferenceField()", "Owner object of reference field has not been deleted correctly. The reference field was not empty when the class destructor was called.");
	}
#endif

	/// Read access to the RefTarget derived pointer.
	operator RefTargetType*() const { return reinterpret_cast<RefTargetType*>(_pointer); }

	/// Write access to the RefTarget pointer. Changes the value of the reference field.
	/// The old reference target will be released and the new reference target
	/// will be bound to this reference field.
	/// This operator automatically handles undo so the value change can be undone.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTargetType* newPointer) {
		SingleReferenceFieldBase::setInternal(owner, descriptor, newPointer);
	}

	/// Overloaded arrow operator; implements pointer semantics.
	/// Just use this operator as you would with a normal C++ pointer.
	RefTargetType* operator->() const {
		OVITO_ASSERT_MSG(_pointer, "ReferenceField operator->", "Tried to make a call to a NULL pointer.");
		return reinterpret_cast<RefTargetType*>(_pointer);
	}

	/// Dereference operator; implements pointer semantics.
	/// Just use this operator as you would with a normal C++ pointer.
	RefTargetType& operator*() const {
		OVITO_ASSERT_MSG(_pointer, "ReferenceField operator*", "Tried to dereference a NULL pointer.");
		return *reinterpret_cast<RefTargetType*>(_pointer);
	}

	/// Returns true if the internal pointer is non-null.
	operator bool() const { return _pointer != nullptr; }
};

/// \brief Dynamic casting function for reference fields.
///
/// Returns the given object cast to type \c T if the object is of type \c T
/// (or of a subclass); otherwise returns \c NULL.
///
/// \relates ReferenceField
template<class T, class U>
inline T* dynamic_object_cast(const ReferenceField<U>& field) {
	return dynamic_object_cast<T,U>(field.value());
}

/**
 * \brief Manages a list of references to RefTarget objects held by a RefMaker derived class.
 */
class OVITO_CORE_EXPORT VectorReferenceFieldBase : public PropertyFieldBase
{
public:

	/// Returns the stored references as a QVector.
	operator const QVector<RefTarget*>&() const { return pointers; }

	/// Returns the reference target at index position i.
	RefTarget* operator[](int i) const { return pointers[i]; }

	/// Returns the number of objects in the vector reference field.
	int size() const { return pointers.size(); }

	/// Returns true if the vector has size 0; otherwise returns false.
	bool empty() const { return pointers.empty(); }

	/// Returns true if the vector contains an occurrence of value; otherwise returns false.
	bool contains(const RefTarget* value) const { return pointers.contains(const_cast<RefTarget*>(value)); }

	/// Returns the index position of the first occurrence of value in the vector,
	/// searching forward from index position from. Returns -1 if no item matched.
	int indexOf(const RefTarget* value, int from = 0) const { return pointers.indexOf(const_cast<RefTarget*>(value), from); }

	/// Returns the stored references as a QVector.
	const QVector<RefTarget*>& targets() const { return pointers; }

	/// Clears all references and sets the vector size to zero.
	void clear(RefMaker* owner, const PropertyFieldDescriptor& descriptor);

	/// Removes the element at index position i.
	void remove(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i);

protected:

	/// The actual pointer list to the reference targets.
	QVector<RefTarget*> pointers;

	/// Replaces a reference in the vector.
	void setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTarget* object);

	/// Adds a reference target to the internal list.
	int insertInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget, int index = -1);

	/// Removes a target from the list reference field.
	void removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, RefTarget const*& deadStorage, bool generateNotificationEvents = true);

	/// Removes a target from the list reference field.
	void removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, OORef<const RefTarget>& deadStorage, bool generateNotificationEvents = true);

	/// Removes a target from the list reference field.
	void removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, DataOORef<const DataObject>& deadStorage, bool generateNotificationEvents = true);

	/// Adds the target to the list reference field.
	int addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, RefTarget const*&& target, int index);

	/// Adds the target to the list reference field.
	int addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<const RefTarget>&& target, int index);

	/// Adds the target to the list reference field.
	int addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, DataOORef<const DataObject>&& target, int index);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, RefTarget const*& inactiveTarget, int index, bool generateNotificationEvents = true);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<const RefTarget>& inactiveTarget, int index, bool generateNotificationEvents = true);

	/// Replaces the target stored in the reference field.
	void swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, DataOORef<const DataObject>& inactiveTarget, int index, bool generateNotificationEvents = true);

private:

	template<typename ReferenceType>
	class InsertReferenceOperation : public PropertyFieldOperation
	{
	public:
    	InsertReferenceOperation(RefMaker* owner, ReferenceType target, VectorReferenceFieldBase& reffield, int index, const PropertyFieldDescriptor& descriptor) :
			PropertyFieldOperation(owner, descriptor), _target(std::move(target)), _reffield(reffield), _index(index) {}

		virtual void undo() override {
			OVITO_ASSERT(!_target);
			_reffield.removeReference(owner(), descriptor(), _index, _target);
		}

		virtual void redo() override {
			_index = _reffield.addReference(owner(), descriptor(), std::move(_target), _index);
			OVITO_ASSERT(!_target);
		}

		int insertionIndex() const { return _index; }

		virtual QString displayName() const override {
			return QStringLiteral("Insert ref to %1 into vector field <%2> of %3")
				.arg(_target ? _target->getOOClass().name() : "<null>")
				.arg(descriptor().identifier())
				.arg(owner()->getOOClass().name());
		}


	private:
		/// The target that has been added into the vector reference field.
	    ReferenceType _target;
		/// The vector reference field to which the reference has been added.
		VectorReferenceFieldBase& _reffield;
		/// The position at which the target has been inserted into the vector reference field.
		int _index;
	};

	template<typename ReferenceType>
	class RemoveReferenceOperation : public PropertyFieldOperation
	{
	public:
    	RemoveReferenceOperation(RefMaker* owner, VectorReferenceFieldBase& reffield, int index, const PropertyFieldDescriptor& descriptor) :
			PropertyFieldOperation(owner, descriptor), _reffield(reffield), _index(index) {}

		virtual void undo() override {
			_index = _reffield.addReference(owner(), descriptor(), std::move(_target), _index);
			OVITO_ASSERT(!_target);
		}

		virtual void redo() override {
			OVITO_ASSERT(!_target);
			_reffield.removeReference(owner(), descriptor(), _index, _target);
		}

		virtual QString displayName() const override {
			return QStringLiteral("Remove ref to %1 from vector field <%2> of %3")
				.arg(_target ? _target->getOOClass().name() : "<null>")
				.arg(descriptor().identifier())
				.arg(owner()->getOOClass().name());
		}

	private:
		/// The target that has been removed from the vector reference field.
	    ReferenceType _target;
		/// The vector reference field from which the reference has been removed.
		VectorReferenceFieldBase& _reffield;
		/// The position at which the target has been removed from the vector reference field.
		int _index;
	};

	template<typename ReferenceType>
	class ReplaceReferenceOperation : public PropertyFieldOperation
	{
	private:

		/// The reference target that is currently not assigned to the reference field.
		/// This is stored here so that we can restore it on a call to undo().
		ReferenceType _inactiveTarget;

		/// The reference field whose value has changed.
		VectorReferenceFieldBase& _reffield;

		/// The list position at which the target has been replaced.
		int _index;

	public:
		
		ReplaceReferenceOperation(RefMaker* owner, ReferenceType oldTarget, VectorReferenceFieldBase& reffield, const PropertyFieldDescriptor& descriptor, int index) :
			PropertyFieldOperation(owner, descriptor), _inactiveTarget(std::move(oldTarget)), _reffield(reffield), _index(index) {}
		
		virtual void undo() override { 
			_reffield.swapReference(owner(), descriptor(), _inactiveTarget, _index); 
		}

		virtual QString displayName() const override {
			return QStringLiteral("Replacing ref with %1 in vector field <%2> of %3")
				.arg(_inactiveTarget ? _inactiveTarget->getOOClass().name() : "<null>")
				.arg(descriptor().identifier())
				.arg(owner()->getOOClass().name());
		}
	};

	friend class RefMaker;
	friend class RefTarget;
};

/**
 * \brief Templated version of the VectorReferenceFieldBase class.
 */
template<typename RefTargetType>
class VectorReferenceField : public VectorReferenceFieldBase
{
public:
	using target_type = RefTargetType;

	typedef QVector<RefTargetType*> RefTargetVector;
	typedef QVector<const RefTargetType*> ConstRefTargetVector;
	typedef typename RefTargetVector::ConstIterator ConstIterator;
	typedef typename RefTargetVector::const_iterator const_iterator;
	typedef typename RefTargetVector::const_pointer const_pointer;
	typedef typename RefTargetVector::const_reference const_reference;
	typedef typename RefTargetVector::difference_type difference_type;
	typedef typename RefTargetVector::size_type size_type;
	typedef typename RefTargetVector::value_type value_type;

#ifdef OVITO_DEBUG
	/// Destructor that releases all referenced objects.
	~VectorReferenceField() {
		OVITO_ASSERT_MSG(pointers.empty(), "~VectorReferenceField()", "Owner object of vector reference field has not been deleted correctly. The reference field was not empty when the class destructor was called.");
	}
#endif

	/// Returns the stored references as a QVector.
	operator const RefTargetVector&() const { return targets(); }

	/// Returns the reference target at index position i.
	RefTargetType* operator[](int i) const { return static_object_cast<RefTargetType>(pointers[i]); }

	/// Inserts a reference at the end of the vector.
	void push_back(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTargetType* object) { insertInternal(owner, descriptor, object); }

	/// Inserts a reference at index position i in the vector.
	/// If i is 0, the value is prepended to the vector.
	/// If i is size() or negative, the value is appended to the vector.
	void insert(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTargetType* object) { insertInternal(owner, descriptor, object, i); }

	/// Replaces a reference in the vector.
	/// This method removes the reference at index i and inserts the new reference at the same index.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTargetType* object) {
		setInternal(owner, descriptor, i, object);
	}

	/// Returns an STL-style iterator pointing to the first item in the vector.
	const_iterator begin() const { return targets().begin(); }

	/// Returns an STL-style iterator pointing to the imaginary item after the last item in the vector.
	const_iterator end() const { return targets().end(); }

	/// Returns an STL-style iterator pointing to the first item in the vector.
	const_iterator constBegin() const { return begin(); }

	/// Returns a const STL-style iterator pointing to the imaginary item after the last item in the vector.
	const_iterator constEnd() const { return end(); }

	/// Returns the first reference stored in this vector reference field.
	RefTargetType* front() const { return targets().front(); }

	/// Returns the last reference stored in this vector reference field.
	RefTargetType* back() const { return targets().back(); }

	/// Finds the first object stored in this vector reference field that is of the given type.
	/// or can be cast to the given type. Returns NULL if no such object is in the list.
	template<class Clazz>
	Clazz* firstOf() const {
		for(const_iterator i = constBegin(); i != constEnd(); ++i) {
			Clazz* obj = dynamic_object_cast<Clazz>(*i);
			if(obj) return obj;
		}
		return nullptr;
	}

	/// Copies the references of another vector reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const VectorReferenceField& other) {
		for(int i = 0; i < other.size() && i < this->size(); i++)
			set(owner, descriptor, i, other[i]);
		for(int i = this->size(); i < other.size(); i++)
			push_back(owner, descriptor, other[i]);
		for(int i = this->size() - 1; i >= other.size(); i--)
			remove(owner, descriptor, i);
	}

	/// Assigns the given list of targets to this vector reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTargetVector& other) {
		for(int i = 0; i < other.size() && i < this->size(); i++)
			set(owner, descriptor, i, other[i]);
		for(int i = this->size(); i < other.size(); i++)
			push_back(owner, descriptor, other[i]);
		for(int i = this->size() - 1; i >= other.size(); i--)
			remove(owner, descriptor, i);
	}

	/// Assigns the given list of targets to this vector reference field.
	void set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const ConstRefTargetVector& other) {
		set(owner, descriptor, reinterpret_cast<const RefTargetVector&>(other));
	}

	/// Returns the stored references as a QVector.
	const RefTargetVector& targets() const { return reinterpret_cast<const RefTargetVector&>(pointers); }
};

}	// End of namespace
