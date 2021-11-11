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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/pipeline/PipelineStatus.h>

namespace Ovito {

/**
 * \brief Abstract base class for objects perform long-running computations and which 
 *        can be enabled or disabled.
 */
class OVITO_CORE_EXPORT ActiveObject : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ActiveObject)

#ifdef OVITO_QML_GUI
	Q_PROPERTY(PipelineStatus status READ status NOTIFY objectStatusChanged)
#endif

protected:

	/// \brief Constructor.
	ActiveObject(DataSet* dataset);

public:

	/// \brief Returns the title of this object.
	virtual QString objectTitle() const override {
		if(title().isEmpty()) 
			return RefTarget::objectTitle();
		else
			return title();
	}

	/// \brief Changes the title of this object.
	/// \undoable
	void setObjectTitle(const QString& title) { 
		setTitle(title); 
	}

	/// \brief Returns true if at least one computation task associated with this object is currently active. 
	bool isObjectActive() const { return _isInActivateState; }

Q_SIGNALS:

#ifdef OVITO_QML_GUI
	/// This signal is emitted whenever the status of this object changes.
	/// The signal is used in the QML GUI to update the status display.
	void objectStatusChanged();
#endif

protected:

	/// Is called when the value of a non-animatable property field of this RefMaker has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor* field) override;

#ifdef OVITO_QML_GUI
	/// Sends an event to all dependents of this RefTarget.
	virtual void notifyDependentsImpl(const ReferenceEvent& event) override;
#endif

	/// Increments the internal task counter and notifies the UI that this object is currently active.
	void incrementNumberOfActiveTasks();

	/// Decrements the internal task counter and, if the counter has reached zero, notifies the 
	/// UI that this object is no longer active.
	void decrementNumberOfActiveTasks();

	/// Registers the given asynchronous task as an active task associated with this object.
	void registerActiveTask(const TaskPtr& task);

	/// Registers the given future as an active task associated with this object.
	void registerActiveFuture(const FutureBase& future) {
		registerActiveTask(future.task());
	}

	/// Registers the given promise as an active task associated with this object.
	void registerActivePromise(const PromiseBase& promise) {
		registerActiveTask(promise.task());
	}

	/// Handles timer events for this object.
	virtual void timerEvent(QTimerEvent* event) override;

private:

	/// Controls whether the object is currently enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isEnabled, setEnabled);
	DECLARE_SHADOW_PROPERTY_FIELD(isEnabled);

	/// The user-defined title of this object.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);
	DECLARE_SHADOW_PROPERTY_FIELD(title);

	/// The current status of this object.
	DECLARE_RUNTIME_PROPERTY_FIELD_FLAGS(PipelineStatus, status, setStatus, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

	/// Indicates how many running tasks are currently associated with this object.
	int _numberOfActiveTasks = 0;

	/// Flag indicating wheter this object is currently displayed as active in the GUI.
	bool _isInActivateState = false;

	/// Timer used to implement delayed UI updates of the activity state.
	QBasicTimer _activityTimer;

	/// Timer used to implement delayed UI updates whenever object status changes.
	QBasicTimer _statusTimer;
};

}	// End of namespace
