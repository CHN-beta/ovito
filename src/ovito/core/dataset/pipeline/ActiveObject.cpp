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

#include <ovito/core/Core.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include "ActiveObject.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ActiveObject);
DEFINE_PROPERTY_FIELD(ActiveObject, isEnabled);
DEFINE_PROPERTY_FIELD(ActiveObject, title);
DEFINE_PROPERTY_FIELD(ActiveObject, status);
SET_PROPERTY_FIELD_LABEL(ActiveObject, isEnabled, "Enabled");
SET_PROPERTY_FIELD_LABEL(ActiveObject, title, "Name");
SET_PROPERTY_FIELD_LABEL(ActiveObject, status, "Status");
SET_PROPERTY_FIELD_CHANGE_EVENT(ActiveObject, isEnabled, ReferenceEvent::TargetEnabledOrDisabled);
SET_PROPERTY_FIELD_CHANGE_EVENT(ActiveObject, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
ActiveObject::ActiveObject(DataSet* dataset) : RefTarget(dataset), _isEnabled(true)
{
}

/******************************************************************************
* Is called when the value of a non-animatable property field of this RefMaker has changed.
******************************************************************************/
void ActiveObject::propertyChanged(const PropertyFieldDescriptor& field)
{
    // If the object is disabled, clear its status.
	if(field == PROPERTY_FIELD(isEnabled) && !isEnabled()) {
		setStatus(PipelineStatus::Success);
	}

    // Whenever the object's status changes, update UI with some delay.
	if(field == PROPERTY_FIELD(status)) {
		if(!_statusTimer.isActive()) {
			_statusTimer.start(50, Qt::CoarseTimer, this);
		}
	}

    RefTarget::propertyChanged(field);
}

/******************************************************************************
* Increments the internal task counter and notifies the UI that this 
* object is currently active.
******************************************************************************/
void ActiveObject::incrementNumberOfActiveTasks()
{
	if(_numberOfActiveTasks++ == 0) {
		OVITO_ASSERT(_isInActivateState == false);
		// Indicate activity status in the UI with a 100 ms delay to prevent excessive updates in case of short-running tasks.
		_activityTimer.start(100, Qt::CoarseTimer, this);
	}
}

/******************************************************************************
* Decrements the internal task counter and, if the counter has reached zero, 
* notifies the UI that this object is no longer active.
******************************************************************************/
void ActiveObject::decrementNumberOfActiveTasks()
{
	OVITO_ASSERT(_numberOfActiveTasks > 0);
	if(--_numberOfActiveTasks == 0) {
		_activityTimer.stop();
		if(_isInActivateState) {
			_isInActivateState = false;
			notifyDependents(ReferenceEvent::ObjectStatusChanged);
		}
	}
}

/******************************************************************************
* Registers the given future as an active task associated with this object.
******************************************************************************/
void ActiveObject::registerActiveTask(const TaskPtr& task)
{
	if(!task->isFinished() && Application::instance()->guiMode()) {
		incrementNumberOfActiveTasks();
		// Reset the pending status after the Future is fulfilled.
		task->finally(executor(), false, std::bind(&ActiveObject::decrementNumberOfActiveTasks, this));
	}
}

/******************************************************************************
* Handles timer events for this object.
******************************************************************************/
void ActiveObject::timerEvent(QTimerEvent* event)
{
	if(event->timerId() == _activityTimer.timerId()) {
		OVITO_ASSERT(_activityTimer.isActive());
		OVITO_ASSERT(_numberOfActiveTasks > 0);
		OVITO_ASSERT(_isInActivateState == false);
		_activityTimer.stop();
		_isInActivateState = true;
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
	else if(event->timerId() == _statusTimer.timerId()) {
		OVITO_ASSERT(_statusTimer.isActive());
		_statusTimer.stop();
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
	RefTarget::timerEvent(event);
}

}	// End of namespace
