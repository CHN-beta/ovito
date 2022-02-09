////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
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
#include <ovito/core/utilities/concurrent/ExecutionContext.h>
#include "Promise.h"

namespace Ovito {

/**
 * A promise-like object that is used during long-running program operations that are performed synchronously by the program's main thread.
 * 
 * The operation is automatically put into the 'finished' state by the class' destructor.
 */
class OVITO_CORE_EXPORT MainThreadOperation : public Promise<>
{
public:

	/// Creates a promise that represents an asynchronous operation running in the main thread.
	static MainThreadOperation create(UserInterface& userInterface, bool visibleInUserInterface = false) {
		return MainThreadOperation(std::make_shared<ProgressingTask>(Task::Started), userInterface, visibleInUserInterface);
	}

	/// No copy constructor.
	MainThreadOperation(const MainThreadOperation& other) = delete;

	/// Move constructor. 
	MainThreadOperation(MainThreadOperation&& other) noexcept = default;

	/// Destructor.
	~MainThreadOperation() { reset(); }

	/// A promise is not copy assignable.
	MainThreadOperation& operator=(const MainThreadOperation& other) = delete;

	/// Returns the abstract user interface this operation is being performed in. 
	UserInterface& userInterface() const { return _userInterface; }

	/// Puts the promise into the 'finished' state and detaches it from the underlying task object.
	void reset();

	/// Returns true if this operation is the current task in the current thread.
	bool isCurrent() const { return task().get() == Task::currentTask(); }

	/// Returns the shared task, casting it to the ProgressingTask subclass.
	ProgressingTask& progressingTask() const { 
		OVITO_ASSERT(isValid());
		OVITO_ASSERT(task()->isProgressingTask());
		return static_cast<ProgressingTask&>(*task());
	}

	/// Override this method from the Promise class to keep the UI responsive during long-running tasks.
    bool setProgressValue(qlonglong progressValue) const {
		// Temporarily yield control back to the event loop to process UI events.
		processUIEvents();
		return Promise<>::setProgressValue(progressValue);
	}

	/// Override this method from the Promise class to keep the UI responsive during long-running tasks.
    bool incrementProgressValue(qlonglong increment = 1) const { 
		// Temporarily yield control back to the event loop to process UI events.
		processUIEvents();
		return Promise<>::incrementProgressValue(increment); 
	}

	/// Override this method from the Promise class to keep the UI responsive during long-running tasks.
    void setProgressText(const QString& progressText) const { 
		// Temporarily yield control back to the event loop to process UI events.
		processUIEvents();
		Promise<>::setProgressText(progressText); 
	}

	/// Creates a separate MainThreadOperation that represents a sub-task of the running operation.
	/// If the parent task gets canceled, the sub-task is canceled as well, and vice versa.
	MainThreadOperation createSubTask(bool visibleInUserInterface);

protected:

	/// Constructor.
	explicit MainThreadOperation(TaskPtr p, UserInterface& userInterface, bool visibleInUserInterface) noexcept;

	/// Temporarily yield control back to the event loop to process UI events.
	void processUIEvents() const;

	/// The abstract user interface this operation is being performed in. 
	UserInterface& _userInterface;

	/// The task that was active when this operation was started.
	Task* _parentTask = nullptr;
};

/**
 * A helper class that mimics a MainThreadOperation based on an existing asynchronous task.
 * In contrast to MainThreadOperation, it does not automatically set the task to the 'finished'
 * state in its destructor.
 */
class OVITO_CORE_EXPORT MainThreadTaskWrapper : public MainThreadOperation
{
public:

	/// Constructor.
	MainThreadTaskWrapper(TaskPtr task, UserInterface& userInterface) : 
		MainThreadOperation(std::move(task), userInterface, false) {}

	/// Destructor.
	~MainThreadTaskWrapper() { 
		if(_task) {
			// This task is no longer the active one.
			OVITO_ASSERT(Task::currentTask() == _task.get());
			Task::setCurrentTask(_parentTask);

			// Discard task reference to prevent the base class destructor from setting
			// the task to the 'finished' state automatically.
			_task.reset(); 
		}
	}
};

}	// End of namespace
