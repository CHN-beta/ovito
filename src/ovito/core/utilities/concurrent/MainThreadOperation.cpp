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

#include <ovito/core/Core.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/app/UserInterface.h>
#include <ovito/core/utilities/concurrent/MainThreadOperation.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/utilities/concurrent/detail/TaskCallback.h>

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
MainThreadOperation::MainThreadOperation(TaskPtr p, UserInterface& userInterface, bool visibleInUserInterface) noexcept :
	Promise<>(std::move(p)), _userInterface(userInterface), _parentTask(Task::currentTask())
{
	OVITO_ASSERT(isValid());
	OVITO_ASSERT(isStarted());
	OVITO_ASSERT(task()->isProgressingTask());

	// Usage of MainThreadOperation is only permitted in the main thread.
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "MainThreadOperation", "MainThreadOperation may only be created in the main thread.");

	// Make this task the active one.
	Task::setCurrentTask(task().get());

	// Register the container MainThreadOperation with the TaskManager to display its progress in the UI.
	if(visibleInUserInterface)
		userInterface.taskManager().registerTask(task());
}

/******************************************************************************
* Puts the promise into the 'finished' state and detaches it from the underlying task object.
******************************************************************************/
void MainThreadOperation::reset() 
{
	if(TaskPtr task = std::move(_task)) {
		// This task is no longer the active one.
		OVITO_ASSERT(Task::currentTask() == task.get());
		Task::setCurrentTask(_parentTask);
		OVITO_ASSERT(task->isStarted());
		task->setFinished();
	}
}

/******************************************************************************
* Temporarily yield control back to the event loop to process UI events.
******************************************************************************/
void MainThreadOperation::processUIEvents() const
{
	OVITO_ASSERT(isValid());
	OVITO_ASSERT(Task::currentTask() == task().get());

	Task::setCurrentTask(nullptr);
	if(userInterface().processEvents()) {
		cancel();
	}

	OVITO_ASSERT(Task::currentTask() == nullptr);
	Task::setCurrentTask(task().get());
}

/******************************************************************************
* Creates a separate MainThreadOperation that represents a sub-task of the running operation.
* If the parent task gets canceled, the sub-task is canceled as well, and vice versa.
******************************************************************************/
MainThreadOperation MainThreadOperation::createSubTask(bool visibleInUserInterface) 
{
	OVITO_ASSERT(isValid());
	OVITO_ASSERT(task().get() == Task::currentTask());

	class MainThreadSubTask : public ProgressingTask, public detail::TaskCallback<MainThreadSubTask>
	{
	public:
		MainThreadSubTask(const TaskPtr& parentTask) noexcept : ProgressingTask(Task::Started) {
			// Register a callback function to get notified when the parent task gets canceled.
			registerCallback(parentTask.get(), true);

			// When this sub-task gets canceled, we cancel the parent task too.
			this->registerContinuation([](Task& thisTask) noexcept {
				MainThreadSubTask& self = static_cast<MainThreadSubTask&>(thisTask);
				if(self.isCanceled() && self.callbackTask() && !self.callbackTask()->isCanceled())
					self.callbackTask()->cancel();
			});
		}

		/// Callback function, which is invoked whenever the state of the parent task changes.
		bool taskStateChangedCallback(int state) noexcept {
			if(state & Canceled)
				this->cancel();
			// When the parent task finishes, we should detach our callback function immediately,
			// because a task object may have callbacks registered at the end of its lifetime.
			if(state & Finished) {
				OVITO_ASSERT(isFinished());
				return false; // Returning false indicates that the callback wishes to be unregistered.
			}
			return true;
		}
	};

	return MainThreadOperation(std::make_shared<MainThreadSubTask>(task()), userInterface(), visibleInUserInterface);
}

/******************************************************************************
* Puts the task object back into the started state. 
******************************************************************************/
void MainThreadOperation::restart()
{
	// Usage of MainThreadOperation is only permitted in the main thread.
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "MainThreadOperation::restart()", "MainThreadOperation may only be created in the main thread.");

	task()->restart();
}

}	// End of namespace
