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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/OORef.h>
#include <ovito/core/oo/OvitoClass.h>

namespace Ovito {

#ifdef OVITO_DEBUG
	/// Checks whether a pointer to an OvitoObject is valid.
	#define OVITO_CHECK_OBJECT_POINTER(object) { OVITO_CHECK_POINTER(static_cast<const OvitoObject*>(object)); OVITO_ASSERT_MSG(static_cast<const OvitoObject*>(object)->__isObjectAlive(), "OVITO_CHECK_OBJECT_POINTER", "OvitoObject pointer is invalid. Object has been deleted."); }
#else
	/// Do nothing for release builds.
	#define OVITO_CHECK_OBJECT_POINTER(object)
#endif

/**
 * \brief Universal base class for most objects in OVITO.
 *
 * The OvitoObject class offers a reference counting mechanism to manage the
 * the lifetime of object instances. User code should make use of the OORef
 * smart-pointer class, which automatically increments and decrements the reference counter
 * of an OvitoObject when it holds a pointer to it.
 *
 * When the reference counter of an OvitoObject reaches zero, the virtual aboutToBeDeleted()
 * function is called to notify the object that is about to be deleted from memory.
 * After this function returns, the object instance destroys itself.
 *
 * The OvitoObject class provides serialization and deserialization functions, which allow
 * the object to be saved to disk and restored at a later time. Subclasses that want
 * to support serialization of their data should override the virtual functions
 * saveToStream() and loadFromStream().
 */
class OVITO_CORE_EXPORT OvitoObject : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString className READ className CONSTANT)
	Q_PROPERTY(QString pluginId READ pluginId CONSTANT)

public:

	using ovito_class = OvitoObject;
	using OOMetaClass = OvitoClass;

	/// Returns the type descriptor that every OvitoObject-derived class has.
	static const OOMetaClass& OOClass();

	/// \brief The default constructor. Sets the reference count to zero.
	OvitoObject() = default;

#ifdef OVITO_DEBUG
	/// \brief The destructor.
	virtual ~OvitoObject();
#endif

	/// Returns true if this object is currently being loaded from an ObjectLoadStream.
	bool isBeingLoaded() const;

	/// Returns true if this object is about to be deleted, i.e. if the reference count has reached zero
	/// and aboutToBeDeleted() is being invoked.
	bool isAboutToBeDeleted() const {
		return objectReferenceCount() >= INVALID_REFERENCE_COUNT;
	}

	/// \brief Returns the current value of the object's reference counter.
	/// \return The reference count for this object, i.e. the number of references
	///         pointing to this object.
	const QAtomicInt& objectReferenceCount() const noexcept { return _referenceCount; }

#ifdef OVITO_DEBUG
	/// \brief Returns whether this object has not been deleted yet.
	///
	/// This hidden function is used by the OVITO_CHECK_OBJECT_POINTER macro in debug builds.
	bool __isObjectAlive() const { return _magicAliveCode == 0x87ABCDEF; }
#endif

	/// Returns the class descriptor for this object.
	/// This default implementation is overriden by subclasses to return their type descriptor instead.
	virtual const OvitoClass& getOOClass() const { return OOClass(); }

	/// Returns the class descriptor for this object.
	const OvitoClass& getOOMetaClass() const { return OOClass(); }

protected:

	/// \brief Saves the internal data of this object to an output stream.
	/// \param stream The destination stream.
	/// \param excludeRecomputableData Controls whether the object should not store data that can be recomputed at runtime.
	///
	/// Subclasses can override this method to write their data fields
	/// to a file. The derived class \b must always call the base implementation saveToStream() first
	/// before it writes its own data to the stream.
	///
	/// The default implementation of this method does nothing.
	/// \sa loadFromStream()
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const {}

	/// \brief Loads the data of this class from an input stream.
	/// \param stream The source stream.
	/// \throw Exception when a parsing error has occurred.
	///
	/// Subclasses can override this method to read their saved data
	/// from the input stream. The derived class \b must always call the base implementation of loadFromStream() first
	/// before reading its own data from the stream.
	///
	/// The default implementation of this method does nothing.
	///
	/// \note The OvitoObject is not in a fully initialized state when the loadFromStream() method is called.
	///       In particular the developer cannot assume that all other objects stored in the data stream and
	///       referenced by this object have already been restored at the time loadFromStream() is invoked.
	///       The loadFromStreamComplete() method will be called after all objects stored in a file have been completely
	///       loaded and and their data has been restored. If you have to perform some post-deserialization
	///       tasks that require other referenced objects to be in place and fully loaded, then this should
	///       be done by overriding loadFromStreamComplete().
	///
	/// \sa saveToStream()
	virtual void loadFromStream(ObjectLoadStream& stream) {}

	/// \brief This method is called once for this object after they have been
	///        completely loaded from a stream.
	///
	/// It is safe to access sub-objects when overriding this this method.
	/// The default implementation of this method does nothing.
	virtual void loadFromStreamComplete(ObjectLoadStream& stream) {}

	/// This method is called after the reference counter of this object has reached zero
	/// and before the object is being finally deleted. You should not call this method from user
	/// code and typically it is not necessary to override this method.
	virtual void aboutToBeDeleted() { OVITO_CHECK_OBJECT_POINTER(this); }

private:

	/// The current number of references to this object that keep it alive.
	mutable QAtomicInt _referenceCount{0};

	/// This is the special value the reference count of the object is set to while it is being deleted:
	constexpr static auto INVALID_REFERENCE_COUNT = std::numeric_limits<int>::max() / 2;

	/// \brief Increments the reference count by one.
	void incrementReferenceCount() const noexcept {
		OVITO_CHECK_OBJECT_POINTER(this);
		_referenceCount.ref();
	}

	/// \brief Decrements the reference count by one.
	///
	/// When the reference count becomes zero, then the object deletes itself automatically.
	void decrementReferenceCount() const noexcept {
		OVITO_CHECK_OBJECT_POINTER(this);
		if(!_referenceCount.deref()) {
			const_cast<OvitoObject*>(this)->deleteObjectInternal();
		}
	}

	/// Internal method that calls this object's aboutToBeDeleted() routine and the deletes the object.
	/// It is automatically called when the object's reference counter reaches zero.
	Q_INVOKABLE void deleteObjectInternal() noexcept;

	/// Returns the name of the plugin class this object is an instance of. 
	/// This method is an implementation detail required by the Q_PROPERTY macro above.
	const QString& className() const { return getOOClass().name(); }

	/// Returns the idenitifier of the plugin module this object belongs to. 
	/// This method is an implementation detail required by the Q_PROPERTY macro above.
	QString pluginId() const { return QString::fromLatin1(getOOClass().pluginId()); }

#ifdef OVITO_DEBUG
	/// This field is initialized with a special value by the class constructor to indicate that
	/// the object is still alive and has not been deleted. When the object is deleted, the
	/// destructor sets the field to a different value to indicate that the object is no longer alive.
	quint32 _magicAliveCode = 0x87ABCDEF;
#endif

	// Give OORef smart-pointer class access to the internal reference counter.
	template<class T> friend class OORef;

	// These classes need to access the protected serialization functions.
	friend class ObjectSaveStream;
	friend class ObjectLoadStream;
};

/// \brief Dynamic cast operator for subclasses of OvitoObject.
///
/// Returns a pointer to the input object, cast to type \c T if the object is of type \c T
/// (or a subclass); otherwise returns \c nullptr.
///
/// \relates OvitoObject
template<class T, class U>
inline T* dynamic_object_cast(U* obj) noexcept {
	return qobject_cast<T*>(obj);
}

/// \brief Dynamic cast operator for subclasses of OvitoObject derived.
///
/// Returns a constant pointer to the input object, cast to type \c T if the object is of type \c T
/// (or subclass); otherwise returns \c nullptr.
///
/// \relates OvitoObject
template<class T, class U>
inline const T* dynamic_object_cast(const U* obj) noexcept {
	return qobject_cast<const T*>(obj);
}

/// \brief Static cast operator for OvitoObject derived classes.
///
/// Returns a pointer to the object, cast to target type \c T.
/// Performs a runtime check in debug builds to make sure the input object
/// is really an instance of the target class.
///
/// \relates OvitoObject
template<class T, class U>
inline T* static_object_cast(U* obj) noexcept {
	OVITO_ASSERT_MSG(!obj || obj->getOOClass().isDerivedFrom(T::OOClass()), "static_object_cast",
		qPrintable(QString("Runtime type check failed. The source object %1 is not an instance of the target class %2.").arg(obj->getOOClass().name()).arg(T::OOClass().name())));
	return static_cast<T*>(obj);
}

/// \brief Static cast operator for OvitoObject derived object.
///
/// Returns a const pointer to the object, cast to target type \c T.
/// Performs a runtime check in debug builds to make sure the input object
/// is really an instance of the target class.
///
/// \relates OvitoObject
template<class T, class U>
inline const T* static_object_cast(const U* obj) noexcept {
	OVITO_ASSERT_MSG(!obj || obj->getOOClass().isDerivedFrom(T::OOClass()), "static_object_cast",
		qPrintable(QString("Runtime type check failed. The source object %1 is not an instance of the target class %2.").arg(obj->getOOClass().name()).arg(T::OOClass().name())));
	return static_cast<const T*>(obj);
}

/// \brief Turns a pointer to a const object into a pointer to a non-const object.
template<class T>
T* const_pointer_cast(const T* p) noexcept {
    return const_cast<T*>(p);
}

/// \brief Dynamic cast operator for fancy pointers to OVITO objects.
///
/// Returns a fancy pointer to the input object, cast to type \c T if the object is of type \c T
/// (or a subclass); otherwise returns \c nullptr.
///
/// \relates OORef, DataOORef
template<class T, class U, template<typename> class Pointer>
inline Pointer<T> dynamic_object_cast(const Pointer<U>& obj) noexcept {
	return dynamic_pointer_cast<T, U>(obj);
}

/// \brief Dynamic cast operator for fancy pointers to OVITO objects.
///
/// Returns a fancy pointer to the input object, cast to type \c T if the object is of type \c T
/// (or a subclass); otherwise returns \c nullptr.
///
/// \relates OORef, DataOORef
template<class T, class U, template<typename> class Pointer>
inline Pointer<T> dynamic_object_cast(Pointer<U>&& obj) noexcept {
	return dynamic_pointer_cast<T, U>(std::move(obj));
}

/// \brief Static cast operator for fancy pointers to OVITO objects.
///
/// Returns the given object cast to type \c T.
/// Performs a runtime check of the object type in debug build.
///
/// \relates OORef, DataOORef
template<class T, class U, template<typename> class Pointer>
inline Pointer<T> static_object_cast(const Pointer<U>& obj) noexcept {
	OVITO_ASSERT_MSG(!obj || obj->getOOClass().isDerivedFrom(T::OOClass()), "static_object_cast",
		qPrintable(QString("Runtime type check failed. The source object %1 is not an instance of the target class %2.").arg(obj->getOOClass().name()).arg(T::OOClass().name())));
	return static_pointer_cast<T, U>(obj);
}

/// \brief Static cast operator for fancy pointers to OVITO objects.
///
/// Returns the given object cast to type \c T.
/// Performs a runtime check of the object type in debug build.
///
/// \relates OORef, DataOORef
template<class T, class U, template<typename> class Pointer>
inline Pointer<T> static_object_cast(Pointer<U>&& obj) noexcept {
	OVITO_ASSERT_MSG(!obj || obj->getOOClass().isDerivedFrom(T::OOClass()), "static_object_cast",
		qPrintable(QString("Runtime type check failed. The source object %1 is not an instance of the target class %2.").arg(obj->getOOClass().name()).arg(T::OOClass().name())));
	return static_pointer_cast<T, U>(std::move(obj));
}

}	// End of namespace

Q_DECLARE_SMART_POINTER_METATYPE(Ovito::OORef);

#include <ovito/core/utilities/io/ObjectSaveStream.h>
#include <ovito/core/utilities/io/ObjectLoadStream.h>
