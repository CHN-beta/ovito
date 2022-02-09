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
#include "Task.h"
#include "Future.h"
#include "AsynchronousTask.h"
#include "detail/TaskCallback.h"

#include <QWaitCondition>

namespace Ovito {

#ifdef OVITO_DEBUG
std::atomic_size_t Task::_globalTaskCounter{0};
#endif

// The task that is currently the active one in the current thread.
thread_local Task* Task::_currentTask = nullptr;

#ifdef OVITO_DEBUG
/*******************************************************x***********************
* Destructor.
******************************************************************************/
Task::~Task()
{
	// No-op destructor in release builds.

	// Check if the mutex is currently locked.
	// This should never be the case while destroying the promise state.
	OVITO_ASSERT(_mutex.tryLock());
	_mutex.unlock();

	// At the end of their lifetime, tasks must always end up in the finished state.
	OVITO_ASSERT(isFinished());

	// All registered callbacks should have been unregistered by now.
	OVITO_ASSERT(_callbacks == nullptr);

	// In debug builds we keep track of how many task objects exist to check whether they all get destroyed correctly 
	// at program termination. 
	_globalTaskCounter.fetch_sub(1);

	OVITO_ASSERT(currentTask() != this);
}
#endif

/******************************************************************************
* Switches the task into the 'started' state.
******************************************************************************/
bool Task::setStarted() noexcept
{
	const MutexLocker locker(&_mutex);
	return startLocked();
}

/******************************************************************************
* Puts this taskinto the 'started' state (without locking access to the object).
******************************************************************************/
bool Task::startLocked() noexcept
{
	// Check if already started.
	auto state = _state.load(std::memory_order_relaxed);
    if(state & Started)
        return false;

    OVITO_ASSERT(!(state & Finished));
    _state.fetch_or(Started, std::memory_order_relaxed);

	// Inform the registered task watchers.
	callCallbacks(Started);

	return true;
}

/******************************************************************************
* Switches the task into the 'finished' state.
******************************************************************************/
void Task::setFinished() noexcept
{
	MutexLocker locker(&taskMutex());
	if(!isFinished())
		finishLocked(locker);
}

/******************************************************************************
* Puts this task into the 'finished' state (without newly locking the task).
******************************************************************************/
void Task::finishLocked(MutexLocker& locker) noexcept
{
	OVITO_ASSERT(!isFinished());
    OVITO_ASSERT(isStarted());

	// Put this task into the 'finished' state.
	int state = _state.fetch_or(Finished, std::memory_order_relaxed);

	// Make sure that the result has been set (if not in canceled or error state).
	OVITO_ASSERT_MSG(_exceptionStore || isCanceled() || _hasResultsStored.load() || !_resultsStorage,
		"Task::finishLocked()",
		qPrintable(QStringLiteral("Result has not been set for the task. Please check program code setting the task to finished. Task's last progress text: %1")
			.arg(isProgressingTask() ? static_cast<ProgressingTask*>(this)->progressText() : QStringLiteral("<non-progress task>"))));

	// Inform the registered callbacks.
	callCallbacks(Finished);

	// Note: Move the functions into a new local list first so that we can unlock the mutex.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) 
	decltype(_continuations) continuations = std::move(_continuations);
	OVITO_ASSERT(_continuations.empty());
#else
	decltype(_continuations) continuations;
	for(auto& c : _continuations)
		continuations.append(std::move(c));
	_continuations.clear();
#endif
	locker.unlock();

	// Run all continuation functions.
	for(auto& cont : continuations)
		std::move(cont)(*this);
}

/******************************************************************************
* Requests cancellation of the task.
******************************************************************************/
void Task::cancel() noexcept
{
	MutexLocker locker(&taskMutex());
	cancelAndFinishLocked(locker);
}

/******************************************************************************
* Puts this task into the 'canceled' and 'finished' states (without newly locking the task).
******************************************************************************/
void Task::cancelAndFinishLocked(MutexLocker& locker) noexcept
{
	// Put this task into the 'finished' state.
	auto state = _state.fetch_or(Finished, std::memory_order_relaxed);

	// Do nothing if task was already in the 'finished' state.
	if(state & Finished)
		return;

	// Put the task into the 'canceled' state as well.
	state = _state.fetch_or(Canceled, std::memory_order_relaxed);

	// Inform the registered callbacks.
	callCallbacks(!(state & Canceled) ? (Canceled | Finished) : Finished);

	// Note: Move the functions into a new local list first so that we can unlock the mutex.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	decltype(_continuations) continuations = std::move(_continuations);
	OVITO_ASSERT(_continuations.empty());
#else
	decltype(_continuations) continuations;
	for(auto& c : _continuations)
		continuations.append(std::move(c));
	_continuations.clear();
#endif
	locker.unlock();

	// Run the continuation functions.
	for(auto& cont : continuations)
		std::move(cont)(*this);
}

/******************************************************************************
* Puts this task into the 'exception' state to signal that an error has occurred.
******************************************************************************/
void Task::exceptionLocked(std::exception_ptr&& ex) noexcept
{
	OVITO_ASSERT(ex != std::exception_ptr());

	// Make sure the task isn't already canceled or finished.
    OVITO_ASSERT(!(_state.load(std::memory_order_relaxed) & (Canceled | Finished)));

	_exceptionStore = std::move(ex); // NOLINT
}

/******************************************************************************
* Adds a callback to this task's list, which will get notified during state changes.
******************************************************************************/
void Task::addCallback(detail::TaskCallbackBase* cb, bool replayStateChanges) noexcept
{
	OVITO_ASSERT(cb != nullptr);
	
    const MutexLocker locker(&_mutex);

	// Insert into linked list of callbacks.
	cb->_nextInList = _callbacks;
	_callbacks = cb;

	// Replay past state changes to the new callback if requested.
	if(replayStateChanges) {
		if(!cb->callStateChanged(_state.load(std::memory_order_relaxed))) {
			// The callback requested to be removed from the list.
			_callbacks = cb->_nextInList;
		}
	}
}

/******************************************************************************
* Invokes the registered callback functions.
******************************************************************************/
void Task::callCallbacks(int state)
{
	detail::TaskCallbackBase** preceding = &_callbacks;
	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList) {
		if(!cb->callStateChanged(state)) {
			// The callback requested to be removed from the list.
			*preceding = cb->_nextInList;
		}
		else preceding = &cb->_nextInList;
	}
}

/******************************************************************************
* Removes a callback from this task's list, which will no longer get notified about state changes.
******************************************************************************/
void Task::removeCallback(detail::TaskCallbackBase* cb) noexcept
{
    const MutexLocker locker(&_mutex);

	// Remove from linked list of callbacks.
	if(_callbacks == cb) {
		_callbacks = cb->_nextInList;
	}
	else {
		for(detail::TaskCallbackBase* cb2 = _callbacks; cb2 != nullptr; cb2 = cb2->_nextInList) {
			if(cb2->_nextInList == cb) {
				cb2->_nextInList = cb->_nextInList;
				return;
			}
		}
		OVITO_ASSERT(false); // Callback was not found in linked list. Did you try to remove a callback that was never added?
	}
}

/******************************************************************************
* Blocks execution until another task finishes. 
******************************************************************************/
bool Task::waitFor(const TaskPtr& awaitedTask)
{
	OVITO_ASSERT(awaitedTask);

	// The task this function was called from.
	Task* waitingTask = currentTask();
	OVITO_ASSERT_MSG(waitingTask != nullptr, "Task::waitFor()", "No active task. This function may only be called from a task worker function.");

	// Lock access to the waiting task (this function was called from).
	QMutexLocker waitingTaskLocker(&waitingTask->taskMutex());

	// No need to wait for the other task if the waiting task was already canceled.
	if(waitingTask->isCanceled())
		return false;

	// You should never invoke waitFor() on a task object that has already finished!
	OVITO_ASSERT(!waitingTask->isFinished());

	// Quick check if the awaited task has already finished.
	QMutexLocker awaitedTaskLocker(&awaitedTask->taskMutex());
	if(awaitedTask->isFinished()) {
		if(awaitedTask->isCanceled()) {
			// If the awaited was canceled, cancel the waiting task as well.
			waitingTask->cancelAndFinishLocked(waitingTaskLocker);
			return false;
		}
		else {
			// It's ready, no need to wait.
			return true;
		}
	}

	// Create shared pointers on the stack to make sure the two task don't get 
	// destroyed during or right after the waiting phase and before we access them again below.
	TaskPtr waitingTaskPtr(waitingTask->shared_from_this());
	TaskPtr awaitedTaskPtr(awaitedTask);

	waitingTaskLocker.unlock();
	awaitedTaskLocker.unlock();

	// Is the waiting task running in a thread pool?
	if(waitingTask->isAsynchronousTask() && static_cast<AsynchronousTaskBase*>(waitingTask)->threadPool() != nullptr) {
		// Are we really in a worker thread?
		OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread());

		QWaitCondition wc;
		QMutex waitMutex;
		std::atomic_bool alreadyDone{false};

		// Register a callback function with the waiting task, which sets the wait condition in case the waiting task gets canceled.
		detail::FunctionTaskCallback waitingTaskCallback(waitingTask, [&](int state) {
			if(state & (Task::Canceled | Task::Finished)) {
				QMutexLocker locker(&waitMutex);
				alreadyDone.store(true);
				wc.wakeAll();
			}
			return true;
		});

		// Register a callback function with the awaited task, which sets the wait condition when the task gets canceled or finishes.
		detail::FunctionTaskCallback awaitedTaskCallback(awaitedTaskPtr.get(), [&](int state) {
			if(state & Task::Finished) {
				QMutexLocker locker(&waitMutex);
				alreadyDone.store(true);
				wc.wakeAll();
			}
			return true;
		});

		// TODO: Implement work-stealing mechanism to avoid dealock when running out of threads in the thread pool.

		waitMutex.lock();
		// Last minute check if the awaited task has already completed:
		if(!alreadyDone.load()) {
			// Wait now until one of the tasks are done.
			wc.wait(&waitMutex);
		}
		waitMutex.unlock();

		waitingTaskCallback.unregisterCallback();
		awaitedTaskCallback.unregisterCallback();

		waitingTaskLocker.relock();
	}
	else {
		// Otherwise we are currently in the main thread.
		// In this case, use a local event loop to keep processing application events while waiting.
		OVITO_ASSERT(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread());

		// The local event loop we are going to start.
		QEventLoop eventLoop;

		// Register a callback function with the waiting task, which makes the event loop quit in case the waiting task gets canceled.
		detail::FunctionTaskCallback waitingTaskCallback(waitingTask, [&eventLoop](int state) {
			if(state & (Task::Canceled | Task::Finished))
				QMetaObject::invokeMethod(&eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
			return true;
		});

		// Register a callback function with the awaited task, which makes the event loop quit when the task gets canceled or finishes.
		detail::FunctionTaskCallback awaitedTaskCallback(awaitedTaskPtr.get(), [&eventLoop](int state) {
			if(state & Task::Finished)
				QMetaObject::invokeMethod(&eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
			return true;
		});

#ifdef Q_OS_UNIX
		// Boolean flag which is set by the POSIX signal handler when user
		// presses Ctrl+C to interrupt the program.
		static QAtomicInt userInterrupt;
		userInterrupt.storeRelease(0);

		// Install POSIX signal handler to catch Ctrl+C key signal.
		static std::atomic<QEventLoop*> activeEventLoop;
		QEventLoop* previousEventLoop = activeEventLoop.exchange(&eventLoop, std::memory_order_release);
		auto oldSignalHandler = ::signal(SIGINT, [](int) {
			userInterrupt.storeRelease(1);
			if(QEventLoop* eventLoop = activeEventLoop.load(std::memory_order_acquire))
				QMetaObject::invokeMethod(eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
		});
#endif

		{
			// Temporarily switch back to an interactive context now, since
			// the user may be performing other actions in the user interface while the local event loop is running.
			ExecutionContext::Scope executionContextScope(ExecutionContext::Interactive);

			// Enter the local event loop.
			eventLoop.exec();
		}

		waitingTaskCallback.unregisterCallback();
		awaitedTaskCallback.unregisterCallback();

		waitingTaskLocker.relock();

#ifdef Q_OS_UNIX
		// Cancel the task if user pressed Ctrl+C.
		::signal(SIGINT, oldSignalHandler);
		activeEventLoop.store(previousEventLoop, std::memory_order_relaxed);
		if(userInterrupt.loadAcquire()) {
			waitingTask->cancelAndFinishLocked(waitingTaskLocker);
			return false;
		}
#endif
	}

	// Check if the waiting task has been canceled.
	if(waitingTask->isCanceled())
		return false;

	// Now check if the awaited task has been canceled.
	awaitedTaskLocker.relock();

	if(awaitedTaskPtr->isCanceled()) {
		// If the awaited task was canceled, cancel the waiting task as well.
		waitingTask->cancelAndFinishLocked(waitingTaskLocker);
		return false;
	}

	OVITO_ASSERT(awaitedTaskPtr->isFinished());
	return true;
}

}	// End of namespace
