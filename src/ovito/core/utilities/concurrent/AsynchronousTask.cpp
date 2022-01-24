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
#include "AsynchronousTask.h"
#include "MainThreadOperation.h"

#include <QWaitCondition>

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
AsynchronousTaskBase::AsynchronousTaskBase(State initialState, void* resultsStorage) noexcept : ProgressingTask(initialState, resultsStorage) 
{
	QRunnable::setAutoDelete(false);
}	

/******************************************************************************
* Destructor.
******************************************************************************/
AsynchronousTaskBase::~AsynchronousTaskBase() 
{
	// If task was never submitted for execution, cancel and finish it.
	if(!isFinished()) {
		cancel();
		setFinished();
	}
}

/******************************************************************************
* Submits the task for execution to a thread pool.
******************************************************************************/
void AsynchronousTaskBase::startInThreadPool(QThreadPool* pool) 
{
	OVITO_ASSERT(!this->_thisTask);
	OVITO_ASSERT(!this->isStarted());
	// Store a shared_ptr to this task to keep it alive while running.
	this->_thisTask = this->shared_from_this();
	// Inherit execution context from parent task.
	_executionContext = ExecutionContext::current();
	this->setStarted();
	pool->start(this);
}

/******************************************************************************
* Runs the task's work function immediately in the current thread.
******************************************************************************/
void AsynchronousTaskBase::startInThisThread() 
{
	OVITO_ASSERT(!this->_thisTask);
	OVITO_ASSERT(!this->isStarted());
	// Inherit execution context from parent task.
	_executionContext = ExecutionContext::current();
	this->setStarted();
	this->run();
}

/******************************************************************************
* Implementation of QRunnable.
******************************************************************************/
void AsynchronousTaskBase::run() 
{
	OVITO_ASSERT(isStarted());
	ExecutionContext::Scope execScope(_executionContext);
	try {
		perform();
	}
	catch(...) {
		captureException();
	}
	setFinished();
	_thisTask.reset(); // No need to keep the task object alive any longer.
}

/******************************************************************************
* Switches the task into the 'started' state.
******************************************************************************/
bool AsynchronousTaskBase::waitForFuture(const FutureBase& future)
{
	OVITO_ASSERT(future.isValid());

	// Is the current task running in a thread pool?
	if(_thisTask) {

		// Lock access to this task.
		QMutexLocker thisTaskLocker(&this->taskMutex());

		// No need to wait for the other task if this task is already canceled.
		if(isCanceled())
			return false;

		// You should never invoke waitForTask() on a task object that has already finished.
		OVITO_ASSERT(!isFinished());

		// Quick check if the awaited task has already finished.
		const TaskPtr& awaitedTask = future.task();
		QMutexLocker awaitedTaskLocker(&awaitedTask->taskMutex());
		if(awaitedTask->isFinished()) {
			if(awaitedTask->isCanceled()) {
				// If the awaited was canceled, cancel this task as well.
				cancelAndFinishLocked(thisTaskLocker);
				return false;
			}
			else {
				// It's ready, no need to wait.
				return true;
			}
		}

		// Create a shared pointer on the stack to make sure the awaited task doesn't get 
		// destroyed during or right after the waiting phase and before we access it
		// again below.
		TaskPtr awaitedTaskPtr(awaitedTask);

		thisTaskLocker.unlock();
		awaitedTaskLocker.unlock();

		QWaitCondition wc;
		QMutex waitMutex;
		QAtomicInt alreadyDone{0};

		// Register a callback function with the waiting task, which sets the wait condition in case the waiting task gets canceled.
		detail::FunctionTaskCallback thisTaskCallback(this, [&](int state) {
			if(state & (Task::Canceled | Task::Finished)) {
				QMutexLocker locker(&waitMutex);
				alreadyDone.storeRelaxed(true);
				wc.wakeAll();
			}
			return true;
		});

		// Register a callback function with the awaited task, which sets the wait condition when the task gets canceled or finishes.
		detail::FunctionTaskCallback awaitedfTaskCallback(awaitedTaskPtr.get(), [&](int state) {
			if(state & Task::Finished) {
				QMutexLocker locker(&waitMutex);
				alreadyDone.storeRelaxed(true);
				wc.wakeAll();
			}
			return true;
		});

		// Wait now until one of the tasks are done.
		waitMutex.lock();
		if(!alreadyDone.loadRelaxed())
			wc.wait(&waitMutex);
		waitMutex.unlock();

		thisTaskCallback.unregisterCallback();
		awaitedfTaskCallback.unregisterCallback();

		thisTaskLocker.relock();

		// Check if the waiting task has been canceled.
		if(isCanceled())
			return false;

		// Now check if the awaited task has been canceled.
		awaitedTaskLocker.relock();

		if(awaitedTaskPtr->isCanceled()) {
			// If the awaited task was canceled, cancel this task as well.
			cancelAndFinishLocked(thisTaskLocker);
			return false;
		}

		OVITO_ASSERT(awaitedTaskPtr->isFinished());
		return true;
	}
	else {
		// The current task is executing synchronously in the main thread. 
		// Perform a wait using a local QEventLoop, similar to a MainThreadOperation.
		UserInterface* nullUserInterface = nullptr;
		return MainThreadTaskWrapper(this->shared_from_this(), *nullUserInterface).waitForFuture(future);
	}
}


}	// End of namespace
