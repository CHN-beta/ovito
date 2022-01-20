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

constexpr static int MaxProgressEmitsPerSecond = 20;

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
* Sets the current maximum value for progress reporting.
* The current progress value is reset to zero.
******************************************************************************/
void Task::setProgressMaximum(qlonglong maximum)
{
    const QMutexLocker locker(&_mutex);

    _progressMaximum = maximum;
	_progressValue = 0;

    updateTotalProgress();

	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
		cb->callProgressChanged(_totalProgressValue, _totalProgressMaximum);
}

/******************************************************************************
* Sets the current progress value of the task.
******************************************************************************/
bool Task::setProgressValue(qlonglong value)
{
    const QMutexLocker locker(&_mutex);

	auto state = _state.loadRelaxed();
    if(state & (Canceled | Finished) || value == _progressValue)
        return !(state & Canceled);

    _progressValue = value;
    updateTotalProgress();

    if(!_progressTime.isValid() || _totalProgressValue >= _totalProgressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();

		for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
			cb->callProgressChanged(_totalProgressValue, _totalProgressMaximum);
    }

    return !(state & Canceled);
}

/******************************************************************************
* Increments the progress value of the task.
******************************************************************************/
bool Task::incrementProgressValue(qlonglong increment)
{
    const QMutexLocker locker(&_mutex);

	auto state = _state.loadRelaxed();
    if(state & (Canceled | Finished))
        return !(state & Canceled);

    _progressValue += increment;
    updateTotalProgress();

    if(!_progressTime.isValid() || _totalProgressValue >= _totalProgressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();

		for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
			cb->callProgressChanged(_totalProgressValue, _totalProgressMaximum);
    }

    return !(state & Canceled);
}

/******************************************************************************
* Sets the current progress value of the task, generating update events only occasionally.
******************************************************************************/
bool Task::setProgressValueIntermittent(qlonglong progressValue, int updateEvery)
{
	if(_intermittentUpdateCounter >= updateEvery) {
		_intermittentUpdateCounter = 0;
		return setProgressValue(progressValue);
	}
	else {
		_intermittentUpdateCounter++;
		return !isCanceled();
	}
}

/******************************************************************************
* Changes the description of this task to be displayed in the GUI.
******************************************************************************/
void Task::setProgressText(const QString& progressText)
{
    const QMutexLocker locker(&_mutex);

    if(auto state = _state.loadRelaxed(); state & (Canceled | Finished))
        return;

    _progressText = progressText;

	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
		cb->callTextChanged();
}

/******************************************************************************
* Recomputes the total progress made so far based on the progress of the current sub-task.
******************************************************************************/
void Task::updateTotalProgress()
{
	if(_subTaskProgressStack.empty()) {
		_totalProgressMaximum = _progressMaximum;
		_totalProgressValue = _progressValue;
	}
	else {
		double percentage;
		if(_progressMaximum > 0)
			percentage = (double)_progressValue / _progressMaximum;
		else
			percentage = 0;
		for(auto level = _subTaskProgressStack.crbegin(); level != _subTaskProgressStack.crend(); ++level) {
			OVITO_ASSERT(level->first >= 0 && level->first <= level->second.size());
			int weightSum1 = std::accumulate(level->second.cbegin(), level->second.cbegin() + level->first, 0);
			int weightSum2 = std::accumulate(level->second.cbegin() + level->first, level->second.cend(), 0);
			percentage = ((double)weightSum1 + percentage * (level->first < level->second.size() ? level->second[level->first] : 0)) / (weightSum1 + weightSum2);
		}
		_totalProgressMaximum = 1000;
		_totalProgressValue = (qlonglong)(percentage * 1000.0);
	}
}

/******************************************************************************
* Starts a sequence of sub-steps in the progress range of this task.
******************************************************************************/
void Task::beginProgressSubStepsWithWeights(std::vector<int> weights)
{
    OVITO_ASSERT(std::accumulate(weights.cbegin(), weights.cend(), 0) > 0);

    _subTaskProgressStack.emplace_back(0, std::move(weights));
    _progressMaximum = 0;
    _progressValue = 0;
}

/******************************************************************************
* Completes the current sub-step in the sequence started with beginProgressSubSteps() 
* or beginProgressSubStepsWithWeights() and moves to the next one.
******************************************************************************/
void Task::nextProgressSubStep()
{
    const QMutexLocker locker(&_mutex);

    if(auto state = _state.loadRelaxed(); state & (Canceled | Finished))
        return;

	OVITO_ASSERT(!_subTaskProgressStack.empty());
	OVITO_ASSERT(_subTaskProgressStack.back().first < _subTaskProgressStack.back().second.size());
	_subTaskProgressStack.back().first++;

    _progressMaximum = 0;
    _progressValue = 0;
    updateTotalProgress();

	for(detail::TaskCallbackBase* cb = _callbacks; cb != nullptr; cb = cb->_nextInList)
		cb->callProgressChanged(_totalProgressValue, _totalProgressMaximum);
}

/******************************************************************************
* Completes a sub-step sequence started with beginProgressSubSteps() or 
* beginProgressSubStepsWithWeights().
******************************************************************************/
void Task::endProgressSubSteps()
{
	OVITO_ASSERT(!_subTaskProgressStack.empty());
	_subTaskProgressStack.pop_back();
    _progressMaximum = 0;
    _progressValue = 0;
}

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
		qPrintable(QStringLiteral("Result has not been set for the task. Please check program code setting the task to finished. Task's last progress text: %1").arg(progressText())));

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

/******************************************************************************
* Puts this task into the 'canceled' and 'finished' states (without newly locking the task).
******************************************************************************/
void Task::cancelAndFinishLocked(QMutexLocker<QMutex>& locker) noexcept
{
}

}	// End of namespace
