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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/TimeInterval.h>

namespace Ovito {

/**
 * \brief Generic base class for events generated by a RefTarget object.
 */
class ReferenceEvent
{
public:

	/// Types of events generated by RefTarget objects.
	enum Type {

		/// This event is generated by a reference target when its internal state or parameters have changed in some way.
		TargetChanged,

		/// This notification event is generated by a reference target if has been deleted.
		/// This event is automatically handled by the RefMaker class.
		TargetDeleted,

		/// This event is generated by a RefMaker when one of its reference fields changed.
		ReferenceChanged,

		/// This event is generated by a RefMaker when a new reference has been added to one of its list reference fields.
		ReferenceAdded,

		/// This event is generated by a RefMaker when a reference has been removed from one of its list reference fields.
		ReferenceRemoved,

		/// This event is sent by an object to query whether it currently is indirectly referenced by some other object.
		CheckIsReferencedBy,

		/// This event is sent by an object to visit each of its dependents by having them call a callback function.
		VisitDependents,

		/// This event is generated by a RefTarget when its display title changed.
		TitleChanged,

		/// This event is generated by a SceneNode when its transformation controller has generated a TargetChanged event
		/// or the transformation controller has been replaced with a different controller, or if the transformation of a
		/// parent node has changed.
		TransformationChanged,

		/// This event is generated by a Modifier if it has been enabled or disabled.
		TargetEnabledOrDisabled,

		/// This event is generated by a data object or modifier when its status has changed.
		ObjectStatusChanged,

		/// This event is generated by a PipelineObject or by an PipelineSceneNode if the structure of the
		/// upstream pipeline changes.
		PipelineChanged,

		/// This event is generated by a PipelineObject or by an PipelineSceneNode if a new preliminary state
		/// has become available.
		PreliminaryStateAvailable,

		/// This event is generated by a Modifier or a DataVis element whenever the preliminary input state from the upstream
		/// pipeline(s) changes.
		PipelineInputChanged,

		/// This event is generated by the owner of a PipelineCache when the stored pipeline state in the cache has been updated.
		PipelineCacheUpdated,

		/// This event is generated by a pipeline object when the number of animation frames it provides has changed.
		AnimationFramesChanged,
	};

public:

	/// \brief Constructs the message.
	/// \param type Identifies the type of the notification message.
	/// \param sender The object that generated the event.
	explicit ReferenceEvent(Type type, RefTarget* sender) : _type(type), _sender(sender) {}

	/// \brief Returns the type of the event.
	Type type() const { return _type; }

	/// \brief Returns the RefTarget that has generated this message.
	/// \return The sender of this notification message.
	RefTarget* sender() const { return _sender; }

	/// \brief Returns a flag that indicates whether this type of event should be propagated
	///        by a receiver to its respective dependents.
	bool shouldPropagate() const {
		return type() == ReferenceEvent::TargetChanged || type() == ReferenceEvent::PreliminaryStateAvailable;
	}

private:

	/// The type of event.
	Type _type;

	/// The RefTarget object that has generated the event.
	RefTarget* _sender;
};

/**
 * \brief This type of event is generated by a RefMaker whenever the value of a property field or
 *        a reference field changes.
 */
class PropertyFieldEvent : public ReferenceEvent
{
public:

	/// Constructor.
	PropertyFieldEvent(ReferenceEvent::Type type, RefTarget* sender, const PropertyFieldDescriptor* field) :
		ReferenceEvent(type, sender), _field(field) {}

	/// \brief Returns the property/reference field that has changed (may be null).
	const PropertyFieldDescriptor* field() const { return _field; }

private:

	/// The property or reference field that has changed.
	const PropertyFieldDescriptor* _field;
};

/**
 * \brief This type of event is generated by a RefTarget whenever the its state changes.
 */
class TargetChangedEvent : public PropertyFieldEvent
{
public:

	/// Constructor.
	TargetChangedEvent(RefTarget* sender, const PropertyFieldDescriptor* field, const TimeInterval& unchangedInterval = TimeInterval::empty()) :
		PropertyFieldEvent(ReferenceEvent::TargetChanged, sender, field), _unchangedInterval(unchangedInterval) {}

	/// Returns the animation time interval over which the state of the object remained the same (can be the empty interval).
	const TimeInterval& unchangedInterval() const { return _unchangedInterval; }

private:

	/// The animation time interval over which the state of the object remained the same.
	TimeInterval _unchangedInterval;
};

/**
 * \brief This type of event is generated by a RefMaker when the pointer stored in one of its reference
 *        fields has been replaced, removed or added.
 */
class ReferenceFieldEvent : public PropertyFieldEvent
{
public:

	/// Constructor.
	ReferenceFieldEvent(ReferenceEvent::Type type, RefTarget* sender, const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int index = -1) :
		PropertyFieldEvent(type, sender, field), _oldvalue(oldTarget), _newvalue(newTarget), _index(index) {}

	/// \brief Returns the old target that was stored in the reference field.
	RefTarget* oldTarget() const { return _oldvalue; }

	/// \brief Returns the new target that is now stored in the reference field.
	RefTarget* newTarget() const { return _newvalue; }

	/// \brief The index that has been added or removed if the changed reference field is a vector field.
	/// \return The index into the VectorReferenceField where the entry has been added or removed.
	///         Returns -1 if the reference field is not a vector reference field.
	int index() const { return _index; }

private:

	RefTarget* _oldvalue;
	RefTarget* _newvalue;
	int _index;
};

/**
 * \brief This type of event is sent by an object to determine whether it is referenced by some other object.
 */
class CheckIsReferencedByEvent : public ReferenceEvent
{
public:

	/// Constructor.
	explicit CheckIsReferencedByEvent(RefTarget* sender, const RefMaker* dependent, bool onlyStrongReferences) : ReferenceEvent(ReferenceEvent::CheckIsReferencedBy, sender), _dependent(dependent), _onlyStrongReferences(onlyStrongReferences) {}

	/// Returns the potentially dependent, which we are looking for.
	const RefMaker* dependent() const { return _dependent; }

	/// Returns whether only strong references are considered.
	bool onlyStrongReferences() const { return _onlyStrongReferences; }

	/// After the query has been performed, this returns whether the dependent references the target object (directly or indicrectly).
	bool isReferenced() const { return _isReferenced; }

	/// While the query is being been performed, this stores whether the dependent references the target object (directly or indicrectly).
	void setIsReferenced() const { _isReferenced = true; }

private:

	const RefMaker* _dependent;
	bool _onlyStrongReferences;
	mutable bool _isReferenced = false;
};

/**
 * \brief This type of event is sent by an object to visits all of its dependents.
 */
class VisitDependentsEvent : public ReferenceEvent
{
public:

	/// Constructor.
	explicit VisitDependentsEvent(RefTarget* sender, fu2::unique_function<void(RefMaker*)>&& callback) : ReferenceEvent(ReferenceEvent::VisitDependents, sender), _callback(std::move(callback)) {}

	/// Invokes the visitor callback.
	void visitDependent(RefMaker* dependent) const {
		_callback(dependent);
	}

private:

	mutable fu2::unique_function<void(RefMaker*)> _callback;
};

}	// End of namespace
