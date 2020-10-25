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
#include "RefTarget.h"

namespace Ovito {

/**
 * \brief A helper class that can be used to monitor notification events
 *        generated by a RefTarget object without the need to derive a new class from RefMaker.
 *
 * This class is designed to be used on the stack or as a member of another
 * class that is not derived from RefMaker but still wants to receive notification events
 * from a RefTarget.
 */
class OVITO_CORE_EXPORT RefTargetListenerBase : public RefMaker
{
	Q_OBJECT
	OVITO_CLASS(RefTargetListenerBase)

public:

	/// \brief The default constructor.
	RefTargetListenerBase() = default;

	/// \brief Destructor.
	virtual ~RefTargetListenerBase() {
		clearAllReferences();
	}

Q_SIGNALS:

	/// \brief This Qt signal is emitted by the listener each time it receives a notification
	///        event from the current target.
	/// \param event The notification event.
	void notificationEvent(const ReferenceEvent& event);

protected:

	/// This method is called after the reference counter of this object has reached zero
	/// and before the object is being deleted.
	virtual void aboutToBeDeleted() override {
		OVITO_ASSERT_MSG(false, "RefTargetListenerBase::aboutToBeDeleted()", "Invalid use of this class. A RefTargetListener should not be used with OORef smart-pointers.");
	}

	/// \brief Is called when the RefTarget referenced by this listener has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The RefTarget which is being monitored by this listener.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(RefTarget, target, setTarget, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_WEAK_REF);
};

/**
 * \brief A helper class that can be used to monitor notification events
 *        generated by a RefTarget object without the need to derive a new class from RefMaker.
 *
 * This is the templated version of RefTargetListenerBase class.
 */
template<typename T>
class RefTargetListener : public RefTargetListenerBase
{
public:

	/// \brief Returns the current target this listener is listening to.
	/// \return The current target object or \c NULL.
	T* target() const { return static_object_cast<T>(RefTargetListenerBase::target()); }

	/// \brief Sets the current target this listener should listen to.
	/// \param newTarget The new target or \c NULL.
	void setTarget(T* newTarget) { RefTargetListenerBase::setTarget(newTarget); }
};

/**
 * \brief A helper class that can be used to monitor notification events
 *        generated by multiple RefTarget objects without the need to derive a new class from RefMaker.
 *
 * This class is designed to be used on the stack or as a member of another
 * class that is not derived from RefMaker but still wants to receive notification events
 * from several RefTarget objects.
 */
class OVITO_CORE_EXPORT VectorRefTargetListenerBase : public RefMaker
{
	OVITO_CLASS(VectorRefTargetListenerBase)
	Q_OBJECT

public:

	/// \brief The default constructor.
	VectorRefTargetListenerBase() = default;

	/// \brief Destructor.
	virtual ~VectorRefTargetListenerBase() {
		clearAllReferences();
	}

	/// \brief Clears the list of targets.
	void clear() { _targets.clear(this, PROPERTY_FIELD(targets));  }

	/// \brief Adds a new object to the list of targets this listener should listen to.
	void push_back(RefTarget* target) { OVITO_CHECK_OBJECT_POINTER(target); _targets.push_back(this, PROPERTY_FIELD(targets), target); }

	/// \brief Inserts a new object into the list of targets this listener should listen to.
	void insert(int index, RefTarget* target) { OVITO_CHECK_OBJECT_POINTER(target); _targets.insert(this, PROPERTY_FIELD(targets), index, target); }

	/// \brief Removes an object from the list of targets this listener should listen to.
	void remove(RefTarget* target) {
		OVITO_CHECK_OBJECT_POINTER(target);
		int index = _targets.indexOf(target);
		if(index >= 0)
			_targets.remove(this, PROPERTY_FIELD(targets), index);
	}

	/// \brief Removes an object from the list of targets this listener should listen to.
	void remove(int index) {
		_targets.remove(this, PROPERTY_FIELD(targets), index);
	}

Q_SIGNALS:

	/// \brief This Qt signal is emitted by the listener each time it receives a notification
	///        event from a target.
	/// \param source The object that sent the notification event.
	/// \param event The notification event.
	void notificationEvent(RefTarget* source, const ReferenceEvent& event);

protected:

	/// This method is called after the reference counter of this object has reached zero
	/// and before the object is being deleted.
	virtual void aboutToBeDeleted() override {
		OVITO_ASSERT_MSG(false, "VectorRefTargetListenerBase::aboutToBeDeleted()", "Invalid use of this class. A VectorRefTargetListener should not be used with OORef smart-pointers.");
	}

	/// \brief Is called when a RefTarget referenced by this listener has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The list of RefTargets which are being monitored by this listener.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD_FLAGS(RefTarget, targets, setTargets, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE | PROPERTY_FIELD_WEAK_REF);
};

/**
 * \brief A helper class that can be used to monitor notification events
 *        generated by multiple RefTarget objects without the need to derive a new class from RefMaker.
 *
 * This is the templated version of VectorRefTargetListenerBase.
 */
template<typename T>
class VectorRefTargetListener : public VectorRefTargetListenerBase
{
public:

	/// \brief Returns the list of targets this listener is listening to.
	/// \return The current list of target objects.
	/// \sa setTargets()
	const QVector<T*>& targets() const { return reinterpret_cast<const QVector<T*>&>(VectorRefTargetListenerBase::targets()); }

	/// \brief Sets the list of targets this listener should listen to.
	/// \param newTargets The new list of targets.
	/// \sa targets()
	void setTargets(const QVector<T*>& newTargets) { VectorRefTargetListenerBase::setTargets(reinterpret_cast<const QVector<RefTarget*>&>(newTargets)); }

	/// \brief Adds a new object to the list of targets this listener should listen to.
	void push_back(T* target) { VectorRefTargetListenerBase::push_back(target); }

	/// \brief Inserts a new object into the list of targets this listener should listen to.
	void insert(int index, T* target) { VectorRefTargetListenerBase::insert(index, target); }
};

}	// End of namespace
