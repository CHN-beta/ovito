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

#include <ovito/core/Core.h>
#include <ovito/core/oo/PropertyFieldDescriptor.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/DataObject.h>
#include "PropertyField.h"

namespace Ovito {

/******************************************************************************
* Generates a notification event to inform the dependents of the field's owner
* that it has changed.
******************************************************************************/
void PropertyFieldBase::generateTargetChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor, ReferenceEvent::Type eventType)
{
	// Make sure we are not trying to generate a change message for objects that are not RefTargets.
	OVITO_ASSERT_MSG(!descriptor.shouldGenerateChangeEvent() || descriptor.definingClass()->isDerivedFrom(RefTarget::OOClass()),
			"PropertyFieldBase::generateTargetChangedEvent()",
			qPrintable(QString("Flag PROPERTY_FIELD_NO_CHANGE_MESSAGE has not been set for property field '%1' of class '%2' even though '%2' is not derived from RefTarget.")
					.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(descriptor.definingClass()->isDerivedFrom(DataObject::OOClass())) {
		// Change events are only sent by a DataObject if the object
		// is not shared by multiple owners and if we are in the main thread.
		if(QThread::currentThread() != owner->thread())
			return;
		if(!static_object_cast<DataObject>(owner)->isSafeToModify())
			return;
	}

	// Send notification message to dependents of owner object.
	if(eventType != ReferenceEvent::TargetChanged) {
		OVITO_ASSERT(owner->isRefTarget());
		static_object_cast<RefTarget>(owner)->notifyDependents(eventType);
	}
	else if(descriptor.shouldGenerateChangeEvent()) {
		OVITO_ASSERT(owner->isRefTarget());
		static_object_cast<RefTarget>(owner)->notifyTargetChanged(&descriptor);
	}
}

/******************************************************************************
* Generates a notification event to inform the dependents of the field's owner
* that it has changed.
******************************************************************************/
void PropertyFieldBase::generatePropertyChangedEvent(RefMaker* owner, const PropertyFieldDescriptor& descriptor)
{
	owner->propertyChanged(descriptor);
}

/******************************************************************************
* Indicates whether undo records should be created.
******************************************************************************/
bool PropertyFieldBase::isUndoRecordingActive(RefMaker* owner, const PropertyFieldDescriptor& descriptor)
{
	if(descriptor.automaticUndo() && owner->dataset()) {
		// Undo recording is only performed in the main thread.
		if(QThread::currentThread() != owner->thread())
			return false;
		return owner->dataset()->undoStack().isRecording();
	}
	return false;
}

/******************************************************************************
* Puts a record on the undo stack.
******************************************************************************/
void PropertyFieldBase::pushUndoRecord(RefMaker* owner, std::unique_ptr<UndoableOperation>&& operation)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "PropertyFieldBase::pushUndoRecord()", "This function may only be called from the main thread.");
	owner->dataset()->undoStack().push(std::move(operation));
}

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyFieldBase::PropertyFieldOperation::PropertyFieldOperation(RefMaker* owner, const PropertyFieldDescriptor& descriptor) :
	_owner(owner != owner->dataset() ? owner : nullptr), _descriptor(descriptor)
{
}

/******************************************************************************
* Access to the object whose property was changed.
******************************************************************************/
RefMaker* PropertyFieldBase::PropertyFieldOperation::owner() const
{
	return static_cast<RefMaker*>(_owner.get());
}

#ifdef OVITO_DEBUG
/******************************************************************************
* Destructor.
******************************************************************************/
template<typename T> SingleReferenceFieldBase<T>::~SingleReferenceFieldBase()
{
	if(_target)
		qDebug() << "Reference field value:" << get();
	OVITO_ASSERT_MSG(!_target, "~ReferenceField()", "Owner object of reference field has not been deleted correctly. The reference field was not empty when the class destructor was called.");
}
#endif

/******************************************************************************
* Replaces the current reference target with a new target. Handles undo recording.
******************************************************************************/
template<typename T> void SingleReferenceFieldBase<T>::set(RefMaker* owner, const PropertyFieldDescriptor& descriptor, pointer newTarget)
{
	if(_target == newTarget) 
		return;	// Nothing to change.

    // Check object type
	if(newTarget && !newTarget->getOOClass().isDerivedFrom(*descriptor.targetClass())) {
		OVITO_ASSERT_MSG(false, "SingleReferenceFieldBase::set()", "Tried to create a reference to an incompatible object for this reference field.");
		owner->throwException(QString("Cannot set a reference field of type %1 to an incompatible object of type %2.").arg(descriptor.targetClass()->name(), newTarget->getOOClass().name()));
	}

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "SingleReferenceFieldBase::set()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
				.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	class SetReferenceOperation : public PropertyFieldOperation
	{
	private:

		/// The reference target that is currently not assigned to the reference field.
		/// This is stored here so that we can restore it on a call to undo().
		pointer _inactiveTarget;

		/// The reference field whose value has changed.
		SingleReferenceFieldBase& _reffield;

	public:
		
		SetReferenceOperation(RefMaker* owner, pointer oldTarget, SingleReferenceFieldBase& reffield, const PropertyFieldDescriptor& descriptor) :
			PropertyFieldOperation(owner, descriptor), _inactiveTarget(std::move(oldTarget)), _reffield(reffield) {}
		
		virtual void undo() override { 
			_reffield.swapReference(owner(), descriptor(), _inactiveTarget); 
		}

		virtual QString displayName() const override {
				return QStringLiteral("Setting reference field <%1> of %2 to point to %3")
					.arg(descriptor().identifier())
					.arg(owner()->getOOClass().name())
					.arg(_inactiveTarget ? _inactiveTarget->getOOClass().name() : "<null>");
		}
	};

	if(isUndoRecordingActive(owner, descriptor)) {
		auto op = std::make_unique<SetReferenceOperation>(owner, std::move(newTarget), *this, descriptor);
		op->redo();
		pushUndoRecord(owner, std::move(op));
	}
	else {
		swapReference(owner, descriptor, newTarget);
	}
}

/******************************************************************************
* Replaces the target stored in the reference field.
******************************************************************************/
template<typename T> void SingleReferenceFieldBase<T>::swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, pointer& inactiveTarget)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(!descriptor.isVector());
	OVITO_ASSERT((descriptor.isWeakReference() == std::is_same<pointer, RefTarget*>::value));

	// Check for cyclic references.
	if(inactiveTarget && owner->isReferencedBy(inactiveTarget))
		throw CyclicReferenceError();

	// Move the old pointer value into a local temporary.
	pointer oldTarget = std::exchange(_target, nullptr);
	OVITO_ASSERT(!_target);

	// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
	if(oldTarget && !owner->hasReferenceTo(oldTarget)) {
		bool success = QObject::disconnect(to_address(oldTarget), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
		OVITO_ASSERT(success);
	}

	// Exchange pointer values.
	_target = std::move(inactiveTarget);
	inactiveTarget = std::move(oldTarget);
	
	// Create a Qt signal/slot connection to the newly referenced object.
	if(_target)
		QObject::connect(to_address(_target), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

	// Inform owner object about the changed reference value.
	owner->referenceReplaced(descriptor, 
		const_cast<RefTarget*>(static_cast<const RefTarget*>(to_address(inactiveTarget))), 
		const_cast<RefTarget*>(static_cast<const RefTarget*>(to_address(_target))), 
		-1);

	// Emit object-changed signal.
	generateTargetChangedEvent(owner, descriptor);

	// Emit additional signal if SET_PROPERTY_FIELD_CHANGE_EVENT macro was used for this property field.
	if(descriptor.extraChangeEventType() != 0)
		generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
}

// Instantiate base class template for the fancy pointer base types needed.
template class SingleReferenceFieldBase<RefTarget*>;
template class SingleReferenceFieldBase<OORef<RefTarget>>;
template class SingleReferenceFieldBase<DataOORef<const DataObject>>;

/******************************************************************************
* Removes a target from the list reference field.
******************************************************************************/
void VectorReferenceFieldBase::removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, RefTarget const*& deadStorage, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(descriptor.isWeakReference());

	OVITO_ASSERT(index >= 0 && index < pointers.size());
	deadStorage = pointers.at(index);

	// Remove reference.
	pointers.remove(index);

	// Release old reference target if there are no more references to it.
	if(deadStorage) {
		// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
		if(!owner->hasReferenceTo(deadStorage)) {
			bool success = QObject::disconnect(deadStorage, &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
			OVITO_ASSERT(success);
		}
	}

	if(generateNotificationEvents) {
		try {
			// Inform derived classes.
			owner->referenceRemoved(descriptor, const_cast<RefTarget*>(deadStorage), index);

			// Send auto change message.
			generateTargetChangedEvent(owner, descriptor);

			// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
			if(descriptor.extraChangeEventType() != 0)
				generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
		}
		catch(...) {
			qDebug() << "Caught exception in VectorReferenceFieldBase::removeReference(). RefMaker is" << owner << ". RefTarget is" << deadStorage;
			if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
				throw;
		}
	}
}

/******************************************************************************
* Removes a target from the list reference field.
******************************************************************************/
void VectorReferenceFieldBase::removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, OORef<const RefTarget>& deadStorage, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(!descriptor.isWeakReference());

	OVITO_ASSERT(index >= 0 && index < pointers.size());
	deadStorage = pointers.at(index);

	// Remove reference.
	pointers.remove(index);

	// Release old reference target if there are no more references to it.
	if(deadStorage) {

		// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
		if(!owner->hasReferenceTo(deadStorage)) {
			bool success = QObject::disconnect(deadStorage.get(), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
			OVITO_ASSERT(success);
		}

		OVITO_ASSERT(deadStorage->objectReferenceCount() >= 2);
		deadStorage->decrementReferenceCount();
	}

	if(generateNotificationEvents) {
		try {
			// Inform derived classes.
			owner->referenceRemoved(descriptor, const_cast<RefTarget*>(deadStorage.get()), index);

			// Send auto change message.
			generateTargetChangedEvent(owner, descriptor);

			// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
			if(descriptor.extraChangeEventType() != 0)
				generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
		}
		catch(...) {
			qDebug() << "Caught exception in VectorReferenceFieldBase::removeReference(). RefMaker is" << owner << ". RefTarget is" << deadStorage.get();
			if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
				throw;
		}
	}
}

/******************************************************************************
* Removes a target from the list reference field.
******************************************************************************/
void VectorReferenceFieldBase::removeReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int index, DataOORef<const DataObject>& deadStorage, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(!descriptor.isWeakReference());
	OVITO_ASSERT(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()));

	OVITO_ASSERT(index >= 0 && index < pointers.size());
	deadStorage = static_object_cast<DataObject>(pointers.at(index));

	// Remove reference.
	pointers.remove(index);

	// Release old reference target if there are no more references to it.
	if(deadStorage) {

		// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
		if(!owner->hasReferenceTo(deadStorage.get())) {
			bool success = QObject::disconnect(deadStorage.get(), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
			OVITO_ASSERT(success);
		}

		deadStorage->decrementDataReferenceCount();
		deadStorage->decrementReferenceCount();
	}

	if(generateNotificationEvents) {
		try {
			// Inform derived classes.
			owner->referenceRemoved(descriptor, const_cast<DataObject*>(deadStorage.get()), index);

			// Send auto change message.
			generateTargetChangedEvent(owner, descriptor);

			// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
			if(descriptor.extraChangeEventType() != 0)
				generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
		}
		catch(...) {
			qDebug() << "Caught exception in VectorReferenceFieldBase::removeReference(). RefMaker is" << owner << ". RefTarget is" << deadStorage.get();
			if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
				throw;
		}
	}
}

/******************************************************************************
* Adds the target to the list reference field.
******************************************************************************/
int VectorReferenceFieldBase::addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, RefTarget const*&& target, int index)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(descriptor.isWeakReference());

	// Check for cyclic references.
	if(target && owner->isReferencedBy(target))
		throw CyclicReferenceError();

	// Add new reference to list field.
	if(index == -1) {
		index = pointers.size();
		pointers.push_back(const_cast<RefTarget*>(target));
	}
	else {
		OVITO_ASSERT(index >= 0 && index <= pointers.size());
		pointers.insert(index, const_cast<RefTarget*>(target));
	}
	if(target) {
		OVITO_CHECK_POINTER(target);

		// Create a Qt signal/slot connection.
		QObject::connect(target, &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));
	}

	try {
		// Inform derived classes.
		owner->referenceInserted(descriptor, const_cast<RefTarget*>(target), index);

		// Send auto change message.
		generateTargetChangedEvent(owner, descriptor);

		// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
	catch(...) {
		qDebug() << "Caught exception in VectorReferenceFieldBase::addReference(). RefMaker is" << owner << ". RefTarget is" << target;
		if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
			throw;
	}
	target = nullptr;

	return index;
}

/******************************************************************************
* Adds the target to the list reference field.
******************************************************************************/
int VectorReferenceFieldBase::addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<const RefTarget>&& target, int index)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(!descriptor.isWeakReference());

	// Check for cyclic references.
	if(target && owner->isReferencedBy(target.get()))
		throw CyclicReferenceError();

	// Add new reference to list field.
	if(index == -1) {
		index = pointers.size();
		pointers.push_back(const_cast<RefTarget*>(target.get()));
	}
	else {
		OVITO_ASSERT(index >= 0 && index <= pointers.size());
		pointers.insert(index, const_cast<RefTarget*>(target.get()));
	}
	if(target) {
		// Create a Qt signal/slot connection.
		QObject::connect(target.get(), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

		target->incrementReferenceCount();
	}

	try {
		// Inform derived classes.
		owner->referenceInserted(descriptor, const_cast<RefTarget*>(target.get()), index);

		// Send auto change message.
		generateTargetChangedEvent(owner, descriptor);

		// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
	catch(...) {
		qDebug() << "Caught exception in VectorReferenceFieldBase::addReference(). RefMaker is" << owner << ". RefTarget is" << target.get();
		if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
			throw;
	}
	target.reset();

	return index;
}

/******************************************************************************
* Adds the target to the list reference field.
******************************************************************************/
int VectorReferenceFieldBase::addReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, DataOORef<const DataObject>&& target, int index)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(!descriptor.isWeakReference());
	OVITO_ASSERT(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()));

	// Check for cyclic references.
	if(target && owner->isReferencedBy(target.get()))
		throw CyclicReferenceError();

	// Add new reference to list field.
	if(index == -1) {
		index = pointers.size();
		pointers.push_back(const_cast<DataObject*>(target.get()));
	}
	else {
		OVITO_ASSERT(index >= 0 && index <= pointers.size());
		pointers.insert(index, const_cast<DataObject*>(target.get()));
	}
	if(target) {
		// Create a Qt signal/slot connection.
		QObject::connect(target.get(), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

		target->incrementReferenceCount();
		static_object_cast<DataObject>(target.get())->incrementDataReferenceCount();
	}

	try {
		// Inform derived classes.
		owner->referenceInserted(descriptor, const_cast<DataObject*>(target.get()), index);

		// Send auto change message.
		generateTargetChangedEvent(owner, descriptor);

		// An additional message can be requested by the user using the SET_PROPERTY_FIELD_CHANGE_EVENT macro.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
	catch(...) {
		qDebug() << "Caught exception in VectorReferenceFieldBase::addReference(). RefMaker is" << owner << ". RefTarget is" << target.get();
		if(!owner->isRefTarget() || !owner->dataset() || !owner->dataset()->undoStack().isUndoingOrRedoing())
			throw;
	}
	target.reset();

	return index;
}

/******************************************************************************
* Adds a reference target to the internal list.
* Creates an undo record so the insertion can be undone at a later time.
******************************************************************************/
int VectorReferenceFieldBase::insertInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, const RefTarget* newTarget, int index)
{
    // Check object type
	if(newTarget && !newTarget->getOOClass().isDerivedFrom(*descriptor.targetClass())) {
		OVITO_ASSERT_MSG(false, "VectorReferenceFieldBase::insertInternal", "Cannot add incompatible object to this vector reference field.");
		owner->throwException(QString("Cannot add an object to a reference field of type %1 that has the incompatible type %2.").arg(descriptor.targetClass()->name(), newTarget->getOOClass().name()));
	}

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "VectorReferenceFieldBase::insertInternal()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
					.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	// Weak references to data objects are not supported.
	OVITO_ASSERT_MSG(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()) == false || descriptor.isWeakReference() == false, "VectorReferenceFieldBase::insertInternal()",
			qPrintable(QString("PROPERTY_FIELD_WEAK_REF flag cannot be used with a DataObject target type in reference field '%1' of class '%2'.")
				.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(isUndoRecordingActive(owner, descriptor)) {
		if(descriptor.isWeakReference()) {
			OVITO_ASSERT(!descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()));
			auto op = std::make_unique<InsertReferenceOperation<const RefTarget*>>(owner, newTarget, *this, index, descriptor);
			op->redo();
			int index = op->insertionIndex();
			pushUndoRecord(owner, std::move(op));
			return index;
		}
		else if(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass())) {
			auto op = std::make_unique<InsertReferenceOperation<DataOORef<const DataObject>>>(owner, static_object_cast<DataObject>(newTarget), *this, index, descriptor);
			op->redo();
			int index = op->insertionIndex();
			pushUndoRecord(owner, std::move(op));
			return index;
		}
		else {
			auto op = std::make_unique<InsertReferenceOperation<OORef<const RefTarget>>>(owner, newTarget, *this, index, descriptor);
			op->redo();
			int index = op->insertionIndex();
			pushUndoRecord(owner, std::move(op));
			return index;
		}
	}
	else {
		if(descriptor.isWeakReference()) {
			const RefTarget* newTargetRef = newTarget;
			return addReference(owner, descriptor, std::move(newTargetRef), index);
		}
		else if(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass())) {
			DataOORef<const DataObject> newTargetRef = static_object_cast<DataObject>(newTarget);
			return addReference(owner, descriptor, std::move(newTargetRef), index);
		}
		else {
			OORef<const RefTarget> newTargetRef = newTarget;
			return addReference(owner, descriptor, std::move(newTargetRef), index);
		}
	}
}

/******************************************************************************
* Removes the element at index position i.
* Creates an undo record so the removal can be undone at a later time.
******************************************************************************/
void VectorReferenceFieldBase::remove(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i)
{
	OVITO_ASSERT(i >= 0 && i < size());

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "VectorReferenceFieldBase::remove()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
					.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(isUndoRecordingActive(owner, descriptor)) {
		if(descriptor.isWeakReference()) {
			OVITO_ASSERT(!descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()));
			auto op = std::make_unique<RemoveReferenceOperation<const RefTarget*>>(owner, *this, i, descriptor);
			op->redo();
			pushUndoRecord(owner, std::move(op));
		}
		else if(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass())) {
			auto op = std::make_unique<RemoveReferenceOperation<DataOORef<const DataObject>>>(owner, *this, i, descriptor);
			op->redo();
			pushUndoRecord(owner, std::move(op));
		}
		else {
			auto op = std::make_unique<RemoveReferenceOperation<OORef<const RefTarget>>>(owner, *this, i, descriptor);
			op->redo();
			pushUndoRecord(owner, std::move(op));
		}
	}
	else {
		if(descriptor.isWeakReference()) {
			const RefTarget* deadStorage;
			removeReference(owner, descriptor, i, deadStorage);
		}
		else if(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass())) {
			DataOORef<const DataObject> deadStorage;
			removeReference(owner, descriptor, i, deadStorage);
		}
		else {
			OORef<const RefTarget> deadStorage;
			removeReference(owner, descriptor, i, deadStorage);
		}
	}
}

/******************************************************************************
* Replaces a reference in the vector.
******************************************************************************/
void VectorReferenceFieldBase::setInternal(RefMaker* owner, const PropertyFieldDescriptor& descriptor, int i, const RefTarget* newTarget) 
{
	OVITO_ASSERT(i >= 0 && i < size());
	if(targets()[i] == newTarget) 
		return; // Nothing changed.

    // Check object type
	if(newTarget && !newTarget->getOOClass().isDerivedFrom(*descriptor.targetClass())) {
		OVITO_ASSERT_MSG(false, "VectorReferenceFieldBase::setInternal()", "Tried to create a reference to an incompatible object for this reference field.");
		owner->throwException(QString("Cannot set a reference field of type %1 to an incompatible object of type %2.").arg(descriptor.targetClass()->name(), newTarget->getOOClass().name()));
	}

	// Make sure automatic undo is disabled for a reference field of a class that is not derived from RefTarget.
	OVITO_ASSERT_MSG(descriptor.automaticUndo() == false || owner->isRefTarget(), "VectorReferenceFieldBase::setInternal()",
			qPrintable(QString("PROPERTY_FIELD_NO_UNDO flag has not been set for reference field '%1' of non-RefTarget derived class '%2'.")
				.arg(descriptor.identifier()).arg(descriptor.definingClass()->name())));

	if(isUndoRecordingActive(owner, descriptor)) {
		if(descriptor.isWeakReference()) {
			OVITO_ASSERT(!descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()));
			auto op = std::make_unique<ReplaceReferenceOperation<const RefTarget*>>(owner, newTarget, *this, descriptor, i);
			op->redo();
			pushUndoRecord(owner, std::move(op));
		}
		else if(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass())) {
			auto op = std::make_unique<ReplaceReferenceOperation<DataOORef<const DataObject>>>(owner, static_object_cast<DataObject>(newTarget), *this, descriptor, i);
			op->redo();
			pushUndoRecord(owner, std::move(op));
		}
		else {
			auto op = std::make_unique<ReplaceReferenceOperation<OORef<const RefTarget>>>(owner, newTarget, *this, descriptor, i);
			op->redo();
			pushUndoRecord(owner, std::move(op));
		}
	}
	else {
		if(descriptor.isWeakReference()) {
			const RefTarget* newTargetRef = newTarget;
			swapReference(owner, descriptor, newTargetRef, i);
		}
		else if(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass())) {
			DataOORef<const DataObject> newTargetRef = static_object_cast<DataObject>(newTarget);
			swapReference(owner, descriptor, newTargetRef, i);
		}
		else {
			OORef<const RefTarget> newTargetRef = newTarget;
			swapReference(owner, descriptor, newTargetRef, i);
		}
	}
	OVITO_ASSERT(targets()[i] == newTarget);
}

/******************************************************************************
* Replaces the target stored in the reference field.
******************************************************************************/
void VectorReferenceFieldBase::swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, RefTarget const*& inactiveTarget, int index, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(descriptor.isWeakReference());
	OVITO_ASSERT(index >= 0 && index < pointers.size());

	// Check for cyclic references.
	if(inactiveTarget && owner->isReferencedBy(inactiveTarget))
		throw CyclicReferenceError();

	const RefTarget* oldTarget = pointers[index];
	if(oldTarget) {
		OVITO_CHECK_OBJECT_POINTER(oldTarget);

		// Reset internal pointer now so that we can use hasReferenceTo() below to check whether
		// any other references to the old target object exist.
		pointers[index] = nullptr;

		// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
		if(!owner->hasReferenceTo(oldTarget)) {
			bool success = QObject::disconnect(oldTarget, &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
			OVITO_ASSERT(success);
		}
	}

	// Swap pointer values.
	pointers[index] = const_cast<RefTarget*>(inactiveTarget);
	
	if(pointers[index]) {
		OVITO_CHECK_OBJECT_POINTER(pointers[index]);

		// Create a Qt signal/slot connection.
		QObject::connect(pointers[index], &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));
	}
	inactiveTarget = std::move(oldTarget);

	if(generateNotificationEvents) {

		// Inform owner object.
		owner->referenceReplaced(descriptor, const_cast<RefTarget*>(inactiveTarget), pointers[index], index);

		// Emit object-changed signal.
		generateTargetChangedEvent(owner, descriptor);

		// Emit additional signal if SET_PROPERTY_FIELD_CHANGE_EVENT macro was used for this property field.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
}

/******************************************************************************
* Replaces the target stored in the reference field.
******************************************************************************/
void VectorReferenceFieldBase::swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, OORef<const RefTarget>& inactiveTarget, int index, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(!descriptor.isWeakReference());
	OVITO_ASSERT(index >= 0 && index < pointers.size());

	// Check for cyclic references.
	if(inactiveTarget && owner->isReferencedBy(inactiveTarget))
		throw CyclicReferenceError();

	OORef<const RefTarget> oldTarget(pointers[index]);
	pointers[index] = nullptr;

	if(oldTarget) {
		// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
		if(!owner->hasReferenceTo(oldTarget)) {
			bool success = QObject::disconnect(oldTarget.get(), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
			OVITO_ASSERT(success);
		}

		oldTarget->decrementReferenceCount();
	}

	pointers[index] = const_cast<RefTarget*>(inactiveTarget.get());

	if(pointers[index]) {
		// Create a Qt signal/slot connection.
		QObject::connect(pointers[index], &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

		pointers[index]->incrementReferenceCount();
	}
	inactiveTarget = std::move(oldTarget);

	if(generateNotificationEvents) {

		// Inform owner object.
		owner->referenceReplaced(descriptor, const_cast<RefTarget*>(inactiveTarget.get()), pointers[index], index);

		// Emit object-changed signal.
		generateTargetChangedEvent(owner, descriptor);

		// Emit additional signal if SET_PROPERTY_FIELD_CHANGE_EVENT macro was used for this property field.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
}

/******************************************************************************
* Replaces the target stored in the reference field.
******************************************************************************/
void VectorReferenceFieldBase::swapReference(RefMaker* owner, const PropertyFieldDescriptor& descriptor, DataOORef<const DataObject>& inactiveTarget, int index, bool generateNotificationEvents)
{
	OVITO_CHECK_POINTER(this);
	OVITO_CHECK_OBJECT_POINTER(owner);
	OVITO_ASSERT(descriptor.isVector());
	OVITO_ASSERT(!descriptor.isWeakReference());
	OVITO_ASSERT(descriptor.targetClass()->isDerivedFrom(DataObject::OOClass()));
	OVITO_ASSERT(index >= 0 && index < pointers.size());

	// Check for cyclic references.
	if(inactiveTarget && owner->isReferencedBy(inactiveTarget))
		throw CyclicReferenceError();

	DataOORef<const DataObject> oldTarget(static_object_cast<DataObject>(pointers[index]));
	pointers[index] = nullptr;

	if(oldTarget) {
		// Disconnect the Qt signal/slot connection, but only if the dependent has no other references to the old target.
		if(!owner->hasReferenceTo(oldTarget)) {
			bool success = QObject::disconnect(oldTarget.get(), &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent);
			OVITO_ASSERT(success);
		}

		static_object_cast<DataObject>(oldTarget.get())->decrementDataReferenceCount();
		oldTarget->decrementReferenceCount();
	}

	pointers[index] = const_cast<DataObject*>(inactiveTarget.get());
	
	if(pointers[index]) {
		// Create a Qt signal/slot connection.
		QObject::connect(pointers[index], &RefTarget::objectEvent, owner, &RefMaker::receiveObjectEvent, static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

		pointers[index]->incrementReferenceCount();
		static_object_cast<DataObject>(pointers[index])->incrementDataReferenceCount();
	}
	inactiveTarget = std::move(oldTarget);

	if(generateNotificationEvents) {

		// Inform owner object.
		owner->referenceReplaced(descriptor, const_cast<DataObject*>(inactiveTarget.get()), pointers[index], index);

		// Emit object-changed signal.
		generateTargetChangedEvent(owner, descriptor);

		// Emit additional signal if SET_PROPERTY_FIELD_CHANGE_EVENT macro was used for this property field.
		if(descriptor.extraChangeEventType() != 0)
			generateTargetChangedEvent(owner, descriptor, static_cast<ReferenceEvent::Type>(descriptor.extraChangeEventType()));
	}
}

/******************************************************************************
* Clears all references and sets the vector size to zero.
******************************************************************************/
void VectorReferenceFieldBase::clear(RefMaker* owner, const PropertyFieldDescriptor& descriptor)
{
	while(!pointers.empty())
		remove(owner, descriptor, pointers.size() - 1);
}

}	// End of namespace
