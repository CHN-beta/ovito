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
#include "detail/TaskCallback.h"

namespace Ovito {

#ifdef OVITO_DEBUG
std::atomic_size_t Task::_globalTaskCounter{0};
#endif

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
}
#endif

/******************************************************************************
* Switches the task into the 'started' state.
******************************************************************************/
bool Task::setStarted() noexcept
{
	const QMutexLocker locker(&_mutex);
	return startLocked();
}

/******************************************************************************
* Puts this taskinto the 'started' state (without locking access to the object).
******************************************************************************/
bool Task::startLocked() noexcept
{
	// Check if already started.
	auto state = _state.loadRelaxed();
    if(state & Started)
        return false;

    OVITO_ASSERT(!(state & Finished));
    _state.fetchAndOrRelaxed(Started);

	// Inform the registered task watchers.
	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
		cb->callStateChanged(Started);

	return true;
}

/******************************************************************************
* Switches the task into the 'finished' state.
******************************************************************************/
void Task::setFinished() noexcept
{
	QMutexLocker locker(&taskMutex());
	if(!isFinished())
		finishLocked(locker);
}

/******************************************************************************
* Puts this task into the 'finished' state (without newly locking the task).
******************************************************************************/
void Task::finishLocked(QMutexLocker<QMutex>& locker) noexcept
{
	OVITO_ASSERT(!isFinished());
    OVITO_ASSERT(isStarted());

	// Put this task into the 'finished' state.
	int state = _state.fetchAndOrRelaxed(Finished);

	// Make sure that the result has been set (if not in canceled or error state).
	OVITO_ASSERT_MSG(_exceptionStore || isCanceled() || _hasResultsStored.load() || !_resultsStorage,
		"Task::finishLocked()",
		qPrintable(QStringLiteral("Result has not been set for the task. Please check program code setting the task to finished. Task's last progress text: %1")
			.arg(isProgressingTask() ? static_cast<ProgressingTask*>(this)->progressText() : QStringLiteral("<non-progress task>"))));

	// Inform the registered task watchers.
	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
		cb->callStateChanged(Finished);

	// Note: Move the functions into a new local list first so that we can unlock the mutex.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) 
	decltype(_continuations) continuations = std::move(_continuations);
	OVITO_ASSERT(_continuations.empty());
#else
	decltype(_continuations) continuations;
	std::move(_continuations.begin(), _continuations.end(), std::back_inserter(continuations));
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
	QMutexLocker locker(&taskMutex());
	cancelAndFinishLocked(locker);
}

/******************************************************************************
* Puts this task into the 'canceled' and 'finished' states (without newly locking the task).
******************************************************************************/
void Task::cancelAndFinishLocked(QMutexLocker<QMutex>& locker) noexcept
{
	// Put this task into the 'finished' state.
	auto state = _state.fetchAndOrRelaxed(Finished);

	// Do nothing if task was already in the 'finished' state.
	if(state & Finished)
		return;

	// Put the task into the 'canceled' state as well.
	state = _state.fetchAndOrRelaxed(Canceled);

	// Inform the registered task watchers.
	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList) {
		cb->callStateChanged(!(state & Canceled) ? (Canceled | Finished) : Finished);
	}

	// Note: Move the functions into a new local list first so that we can unlock the mutex.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	decltype(_continuations) continuations = std::move(_continuations);
	OVITO_ASSERT(_continuations.empty());
#else
	decltype(_continuations) continuations;
	std::move(_continuations.begin(), _continuations.end(), std::back_inserter(continuations));
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
    OVITO_ASSERT(!(_state.loadRelaxed() & (Canceled | Finished)));

	_exceptionStore = std::move(ex); // NOLINT
}

/******************************************************************************
* Adds a callback to this task's list, which will get notified during state changes.
******************************************************************************/
void Task::addCallback(detail::TaskCallbackBase* cb, bool replayStateChanges) noexcept
{
	OVITO_ASSERT(cb != nullptr);
	
    const QMutexLocker locker(&_mutex);

	// Insert into linked list of callbacks.
	cb->_nextInList = _callbacks;
	_callbacks = cb;

	// Replay past state changes to the new callback if requested.
	if(replayStateChanges)
		cb->callStateChanged(_state.loadRelaxed());
}

/******************************************************************************
* Removes a callback from this task's list, which will no longer get notified about state changes.
******************************************************************************/
void Task::removeCallback(detail::TaskCallbackBase* cb) noexcept
{
    const QMutexLocker locker(&_mutex);

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

}	// End of namespace
