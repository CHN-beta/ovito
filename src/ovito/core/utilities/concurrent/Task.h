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
#include <function2/function2.hpp>
#include "detail/FutureDetail.h"

namespace Ovito {

namespace detail { 
    class TaskReference; // Forward declaration
    class TaskCallbackBase;
    template<typename Derived> class TaskCallback;
}

/**
 * \brief The shared state of promises and futures.
 */
class OVITO_CORE_EXPORT Task : public std::enable_shared_from_this<Task>
{
public:

    /// The different states a task can be in.
    enum State {
        NoState       = 0,
        Started       = (1<<0),
        Finished      = (1<<1),
        Canceled      = (1<<2),
        IsProgressing = (1<<3) // Indicates that the task's class is derived from ProgressingTask and can report its progress
    };

    /// Constructor.
    explicit Task(State initialState = NoState, void* resultsStorage = nullptr) noexcept : _state(initialState), _resultsStorage(resultsStorage) {
#ifdef OVITO_DEBUG
        // In debug builds we keep track of how many task objects exist to check whether they all get destroyed correctly 
        // at program termination. 
        _globalTaskCounter.fetch_add(1);
#endif
    }

#ifdef OVITO_DEBUG
    /// Destructor.
    ~Task();
#endif

    /// Returns whether this shared state has been canceled by a previous call to cancel().
    bool isCanceled() const { return (_state.loadRelaxed() & Canceled); }

    /// Returns true if the promise is in the 'started' state.
    bool isStarted() const { return (_state.loadRelaxed() & Started); }

    /// Returns true if the promise is in the 'finished' state.
    bool isFinished() const { return (_state.loadRelaxed() & Finished); }

    /// Indicates whether this task's class is derived from the ProgressingTask base class.
    bool isProgressingTask() const { return (_state.loadRelaxed() & IsProgressing); }

    /// \brief Requests cancellation of the task.
    void cancel() noexcept;

    /// \brief Switches the task into the 'started' state.
    /// \return false if the task was already in the 'started' state before.
    bool setStarted() noexcept;

    /// \brief Switches the task into the 'finished' state.
    void setFinished() noexcept;

    /// \brief Switches the task into the 'exception' state to signal that an exception has occurred.
    ///
    /// This method should be called from within an exception handler. It saves a copy of the current exception
    /// being handled into the task object.
    void captureException() { setException(std::current_exception()); }

    /// \brief Switches the task into the 'exception' state to signal that an exception has occurred.
    /// \param ex The exception to store into the task object.
    void setException(const std::exception_ptr& ex) { setException(std::exception_ptr(ex)); }

    /// \brief Switches the task into the 'exception' state to signal that an exception has occurred.
    /// \param ex The exception to store into the task object.
    void setException(std::exception_ptr&& ex) {
        const QMutexLocker locker(&taskMutex());

        // Check if task is already canceled or finished.
        if(_state.loadRelaxed() & (Canceled | Finished))
            return;

        exceptionLocked(std::move(ex));
    }

    /// \brief Switches the task into the 'exception' and the 'finished' states to signal that an exception has occurred.
    ///
    /// This method should be called from within an exception handler. It saves a copy of the current exception
    /// being handled into the task object.
    void captureExceptionAndFinish() {
        QMutexLocker locker(&taskMutex());

        // Check if task is already canceled or finished.
        if(_state.loadRelaxed() & (Canceled | Finished))
            return;

        exceptionLocked(std::current_exception());
        finishLocked(locker);
    }

	/// Runs the given continuation function once this task has reached either the 'finished' or the 'canceled' state.
	/// Note that the continuation function will always be executed, even if this task was canceled or set to an error state.
    /// The callable may take one optional parameter: a reference to the Task object that completed.
    template<typename Executor, typename Function>
    void finally(Executor&& executor, Function&& f) {
        QMutexLocker locker(&taskMutex());
        addContinuation(std::forward<Executor>(executor).schedule(std::forward<Function>(f)), locker);
    }

	/// Runs the given continuation function once this task has reached either the 'finished' or the 'canceled' state.
	/// Note that the continuation function will always be executed, even if this task was canceled or set to an error state.
    /// The callable may take one optional parameter: a reference to the Task object that completed.
    template<typename Function>
    void finally(Function&& f) { 
        QMutexLocker locker(&taskMutex());
        addContinuation(std::forward<Function>(f), locker); 
    }

    /// Accessor function for the internal results storage.
    /// This overload is used for tasks with a non-empty results tuple.
    template<typename tuple_type>
    const std::enable_if_t<std::tuple_size<tuple_type>::value != 0, tuple_type>& getResults() const {
        OVITO_ASSERT(_resultsStorage != nullptr);
#ifdef OVITO_DEBUG
        OVITO_ASSERT(_hasResultsStored.load());
#endif
        return *static_cast<const tuple_type*>(_resultsStorage);
    }

    /// Accessor function for the internal results storage.
    /// This overload is used for tasks with an empty results tuple (returning void).
    template<typename tuple_type>
    std::enable_if_t<std::tuple_size<tuple_type>::value == 0, tuple_type> getResults() const {
        return {};
    }

    /// Accessor function for the internal results storage.
    template<typename tuple_type>
    tuple_type takeResults() {
        if constexpr(std::tuple_size<tuple_type>::value != 0) {
#ifdef OVITO_DEBUG
            OVITO_ASSERT(_hasResultsStored.exchange(false) == true);
#endif
            OVITO_ASSERT(_resultsStorage != nullptr);
            return std::move(*static_cast<tuple_type*>(_resultsStorage));
        }
        else {
            return {};
        }
    }

    /// \brief Re-throws the exception stored in this task state if an exception was previously set via setException().
    /// \throw The exception stored in the Task (if any).
    void throwPossibleException() {
        if(exceptionStore())
            std::rethrow_exception(exceptionStore());
    }

    /// \brief Returns the internal exception store, which contains an exception object in case the task has failed.
    const std::exception_ptr& exceptionStore() const noexcept { return _exceptionStore; }

    /// \brief Returns a copy of the internal exception store, which contains an exception object in case the task has failed.
    std::exception_ptr copyExceptionStore() const { return std::exception_ptr{exceptionStore()}; }

protected:

    /// Assigns a tuple of values to the internal results storage of the task.
    template<typename tuple_type, typename... R>
    void setResults(std::tuple<R...>&& value) {
		static_assert(std::tuple_size_v<tuple_type> == std::tuple_size_v<std::tuple<R...>>, "Must assign a compatible tuple");
#ifdef OVITO_DEBUG
        OVITO_ASSERT(_hasResultsStored.exchange(true) == false);
#endif
        if constexpr(std::tuple_size_v<tuple_type> != 0) {
            OVITO_ASSERT(_resultsStorage != nullptr);
            *static_cast<tuple_type*>(_resultsStorage) = std::move(value);
        }
    }

    /// Assigns a single value to the internal results storage of the task.
	template<typename tuple_type, typename value_type>
	void setResults(value_type&& result) {
		setResults<tuple_type>(std::forward_as_tuple(std::forward<value_type>(result)));
	}

    /// Assigns a void value to the internal results storage of the task.
	template<typename tuple_type>
	void setResults() {
		setResults<tuple_type>(std::tuple<>{});
	}

    /// Adds a callback to this task's list, which will get notified during state changes.
    void addCallback(detail::TaskCallbackBase* cb, bool replayStateChanges) noexcept;

    /// Removes a callback from this task's list, which will no longer get notified about state changes.
    void removeCallback(detail::TaskCallbackBase* cb) noexcept;

    /// Registers a callback function that will be run when this task reaches the 'finished' state. 
    /// If the task is already in one of these states, the continuation function is invoked immediately.
    template<typename Function>
    void addContinuation(Function&& f, QMutexLocker<QMutex>& locker) {
        // Check if task is already finished.
        if(isFinished()) {
            // Run continuation function immediately.
            locker.unlock();
            if constexpr(std::is_invocable_v<Function, Task&>)
                std::forward<Function>(f)(*this);
            else
                std::forward<Function>(f)();
        }
        else {
            // Otherwise, insert into list to run continuation function later.
            registerContinuation(std::forward<Function>(f));
        }
    }

    /// Registers a callback function that will be run when this task reaches the 'finished' state. 
    /// Do not call this method if the task is already in the 'finished' state.
    template<typename Function>
    void registerContinuation(Function&& f) {
        OVITO_ASSERT(!isFinished());
        // Insert into list. Will run continuation function once the task finishes.
        if constexpr(std::is_invocable_v<Function, Task&>)
            _continuations.push_back(fu2::unique_function<void(Task&) noexcept>{std::forward<Function>(f)});
        else
            _continuations.push_back(fu2::unique_function<void(Task&) noexcept>{
                [f = std::forward<Function>(f)](Task&) mutable noexcept { return std::forward<Function>(f)(); }});
    }

    /// Puts this task into the 'started' state (without newly locking the task).
    bool startLocked() noexcept;

    /// Puts this task into the 'canceled' state (without newly locking the task).
    void exceptionLocked(std::exception_ptr&& ex) noexcept;

    /// Puts this task into the 'canceled' state (without newly locking the task).
    void cancelLocked(QMutexLocker<QMutex>& locker) noexcept;

    /// Puts this task into the 'finished' state (without newly locking the task).
    void finishLocked(QMutexLocker<QMutex>& locker) noexcept;

    /// Puts this task into the 'canceled' and 'finished' states (without newly locking the task).
    void cancelAndFinishLocked(QMutexLocker<QMutex>& locker) noexcept;

    /// Increments the counter of futures or parent tasks currently waiting for this task to complete.
    void incrementDependentsCount() noexcept { _dependentsCount.ref(); }

    /// Decrements the counter of futures or parent tasks currently waiting for this task to complete.
    /// If this counter reaches zero, the task gets canceled.
    void decrementDependentsCount() noexcept { 
        // Automatically cancel this task when there are no one left who depends on it.
        if(!_dependentsCount.deref())
            cancel();
    }

    /// Returns the mutex that is used to manage concurrent access to this task.
    QMutex& taskMutex() const { return _mutex; }

    /// The current state this task is in.
    QAtomicInt _state;

    /// The number of other parties currently waiting for this task to complete.
    QAtomicInt _dependentsCount{0};

    /// Used for managing concurrent access to this task.
    mutable QMutex _mutex;

    /// List of continuation functions that will be called when this task enters the 'finished' or the 'canceled' state.
    QVarLengthArray<fu2::unique_function<void(Task&) noexcept>, 2> _continuations;

    /// Holds the exception object when this shared state is in the failed state.
    std::exception_ptr _exceptionStore;

    /// Head of linked list of callback functions currently registered to this task.
    detail::TaskCallbackBase* _callbacks = nullptr;

    /// Pointer to a std::tuple<...> storing the result value(s) of this task.
    void* _resultsStorage = nullptr;

#ifdef OVITO_DEBUG
    /// Indicates whether the result value of the task has been set.
    std::atomic<bool> _hasResultsStored{false};

    /// Global counter of Task instances that exist at a time. Used only in debug builds to detect memory leaks.
    static std::atomic_size_t _globalTaskCounter;
#endif

    friend class FutureBase;
    friend class PromiseBase;
    friend class MainThreadOperation;
    friend class AsynchronousTaskBase;
    friend class detail::TaskReference;
    friend class detail::TaskCallbackBase;
    template<typename Derived> friend class detail::TaskCallback;
    template<typename... R2> friend class Future;
    template<typename... R2> friend class SharedFuture;
    template<typename... R2> friend class Promise;
};

}	// End of namespace
