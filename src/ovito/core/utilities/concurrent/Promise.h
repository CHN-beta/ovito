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
#include "Task.h"
#include "ProgressingTask.h"
#include "detail/FutureDetail.h"
#include "detail/TaskWithStorage.h"

namespace Ovito {

class OVITO_CORE_EXPORT PromiseBase
{
public:

    /// Default constructor.
    PromiseBase() noexcept = default;

    /// Move constructor.
    PromiseBase(PromiseBase&& p) noexcept = default;

    /// A promise is not copy constructible.
    PromiseBase(const PromiseBase& other) = delete;

    /// Destructor.
    ~PromiseBase() { reset(); }

    /// Returns whether this promise object points to a valid shared state.
    bool isValid() const { return (bool)_task; }

    /// Detaches this promise from its shared state and makes sure that it reached the 'finished' state.
    /// If the promise wasn't already finished when this function is called, it is automatically canceled.
    void reset() {
        if(TaskPtr task = std::move(_task)) {
            if(!task->isFinished()) {
                QMutexLocker locker(&task->taskMutex());
                task->startLocked(); // Just in case the task hasn't been started yet.
                task->cancelAndFinishLocked(locker);
            }
        }
    }

    /// Returns whether this promise has been canceled by a previous call to cancel().
    bool isCanceled() const { return task()->isCanceled(); }

    /// Returns true if the promise is in the 'started' state.
    bool isStarted() const { return task()->isStarted(); }

    /// Returns true if the promise is in the 'finished' state.
    bool isFinished() const { return task()->isFinished(); }

    /// Returns the maximum value for progress reporting.
    qlonglong progressMaximum() const { 
        OVITO_ASSERT(task()->isProgressingTask());
        return static_cast<ProgressingTask*>(task().get())->progressMaximum(); 
    }

    /// Sets the current maximum value for progress reporting.
    void setProgressMaximum(qlonglong maximum) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        static_cast<ProgressingTask*>(task().get())->setProgressMaximum(maximum); 
    }

    /// Returns the current progress value (in the range 0 to progressMaximum()).
    qlonglong progressValue() const { 
        OVITO_ASSERT(task()->isProgressingTask());
        return static_cast<ProgressingTask*>(task().get())->progressValue(); 
    }

    /// Sets the current progress value (must be in the range 0 to progressMaximum()).
    /// Returns false if the promise has been canceled.
    bool setProgressValue(qlonglong progressValue) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        return static_cast<ProgressingTask*>(task().get())->setProgressValue(progressValue); 
    }

    /// Increments the progress value by 1.
    /// Returns false if the promise has been canceled.
    bool incrementProgressValue(qlonglong increment = 1) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        return static_cast<ProgressingTask*>(task().get())->incrementProgressValue(increment); 
    }

    /// Sets the progress value of the promise but generates an update event only occasionally.
    /// Returns false if the promise has been canceled.
    bool setProgressValueIntermittent(qlonglong progressValue, int updateEvery = 2000) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        return static_cast<ProgressingTask*>(task().get())->setProgressValueIntermittent(progressValue, updateEvery); 
    }

    // Progress reporting for tasks with sub-steps:

    /// Begins a sequence of sub-steps in the progress range of this promise.
    /// This is used for long and complex computations, which consist of several logical sub-steps, each with
    /// a separate progress range.
    void beginProgressSubStepsWithWeights(std::vector<int> weights) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        static_cast<ProgressingTask*>(task().get())->beginProgressSubStepsWithWeights(std::move(weights)); 
    }

    /// Convenience version of the function above, which creates N substeps, all with the same weight.
    void beginProgressSubSteps(int nsteps) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        static_cast<ProgressingTask*>(task().get())->beginProgressSubSteps(nsteps); 
    }

    /// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
    void nextProgressSubStep() const { 
        OVITO_ASSERT(task()->isProgressingTask());
        static_cast<ProgressingTask*>(task().get())->nextProgressSubStep(); 
    }

    /// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
    void endProgressSubSteps() const { 
        OVITO_ASSERT(task()->isProgressingTask());
        static_cast<ProgressingTask*>(task().get())->endProgressSubSteps(); 
    }

    /// Return the current status text set for this promise.
    QString progressText() const { 
        OVITO_ASSERT(task()->isProgressingTask());
        return static_cast<ProgressingTask*>(task().get())->progressText(); 
    }

    /// Changes the status text of this promise.
    void setProgressText(const QString& progressText) const { 
        OVITO_ASSERT(task()->isProgressingTask());
        static_cast<ProgressingTask*>(task().get())->setProgressText(progressText); 
    }

    /// Cancels this promise.
    void cancel() const { task()->cancel(); }

    /// This must be called after creating a promise to put it into the 'started' state.
    /// Returns false if the promise is or was already in the 'started' state.
    bool setStarted() const { return task()->setStarted(); }

    /// This must be called after the promise has been fulfilled (even if an exception occurred).
    void setFinished() const { task()->setFinished(); }

    /// Sets the promise into the 'exception' state to signal that an exception has occurred
    /// while trying to fulfill it. This method should be called from a catch(...) exception handler.
    void captureException() const { task()->captureException(); }

    /// Sets the promise into the 'exception' state to signal that an exception has occurred
    /// while trying to fulfill it.
    void setException(std::exception_ptr&& ex) const { task()->setException(std::move(ex)); }

    /// Sets the promise into the 'exception' and 'finished' states to signal that an exception has occurred
    /// while trying to fulfill it. This method should be called from a catch(...) exception handler.
    void captureExceptionAndFinish() const { task()->captureExceptionAndFinish(); }

    /// Move assignment operator.
    PromiseBase& operator=(PromiseBase&& p) = default;

    /// A promise is not copy assignable.
    PromiseBase& operator=(const PromiseBase& other) = delete;

    /// Returns the task object associated with this promise (the shared state).
    const TaskPtr& task() const {
        OVITO_ASSERT(isValid());
        return _task;
    }

    /// Runs the given function in any case once this promise's task has reached the 'finished' or 'canceled' state.
    /// The continuation function will always be executed, even if this task was canceled or set to an error state.
    /// The continuation function must accept a TaskPtr (pointing to the finished task) as a parameter.
    template<typename Executor, typename Function>
    void finally(Executor&& executor, Function&& f) {
        OVITO_ASSERT_MSG(isValid(), "PromiseBase::finally()", "Promise must be valid.");
        task()->finally(std::forward<Executor>(executor), std::forward<Function>(f));
    }

    /// Overload of the method above using the inline executor.
    template<typename Function>
    void finally(Function&& f) {
        OVITO_ASSERT_MSG(isValid(), "PromiseBase::finally()", "Promise must be valid.");
        task()->finally(std::forward<Function>(f));
    }

protected:

    /// Constructor.
    PromiseBase(TaskPtr&& p) noexcept : _task(std::move(p)) {}

    /// Pointer to the state, which is shared with futures.
    TaskPtr _task;

    template<typename... R2> friend class Future;
    template<typename... R2> friend class SharedFuture;
};

template<typename... R>
class Promise : public PromiseBase
{
public:

    using tuple_type = std::tuple<R...>;
    using future_type = Future<R...>;
    using shared_future_type = SharedFuture<R...>;

    /// Default constructor.
    Promise() noexcept = default;

    /// Creates a promise together with a new task.
    template<typename task_type>
    static Promise create(bool stateStarted) {
        return Promise(std::make_shared<detail::TaskWithStorage<tuple_type, task_type>>(Task::State(stateStarted ? Task::Started : Task::NoState)));
    }

    /// Returns a Future that is associated with the same shared state as this promise.
    future_type future() {
#ifdef OVITO_DEBUG
        OVITO_ASSERT_MSG(!_futureCreated, "Promise::future()", "Only a single Future may be created from a Promise.");
        _futureCreated = true;
#endif
        return future_type(TaskPtr(task()));
    }

    /// Returns a SharedFuture that is associated with the same shared state as this promise.
    shared_future_type sharedFuture() {
        return shared_future_type(TaskPtr(task()));
    }	

    /// Sets the result value of the promise.
    template<typename... R2>
    void setResults(R2&&... result) {
        task()->template setResults<tuple_type>(std::forward_as_tuple(std::forward<R2>(result)...));
    }

protected:

    /// Create a promise that is ready and provides immediate default-constructed results.
    static Promise createImmediateEmpty() {
        return Promise(std::make_shared<detail::TaskWithStorage<tuple_type, Task>>(
            Task::State(Task::Started | Task::Finished), tuple_type{}));
    }

    /// Create a promise that is ready and provides an immediate result.
    template<typename... R2>
    static Promise createImmediate(R2&&... result) {
        return Promise(std::make_shared<detail::TaskWithStorage<tuple_type, Task>>(
            Task::State(Task::Started | Task::Finished), 
            std::forward_as_tuple(std::forward<R2>(result)...)));
    }

    /// Create a promise that is ready and provides an immediate result.
    template<typename... Args>
    static Promise createImmediateEmplace(Args&&... args) {
        using first_type = typename std::tuple_element<0, tuple_type>::type;
        return Promise(std::make_shared<detail::TaskWithStorage<tuple_type, Task>>(
            Task::State(Task::Started | Task::Finished), 
            std::forward_as_tuple(first_type(std::forward<Args>(args)...))));
    }

    /// Creates a promise that is in the 'exception' state.
    static Promise createFailed(const Exception& ex) {
        Promise promise(std::make_shared<Task>(Task::State(Task::Started | Task::Finished)));
        promise.task()->_exceptionStore = std::make_exception_ptr(ex);
        return promise;
    }

    /// Creates a promise that is in the 'exception' state.
    static Promise createFailed(Exception&& ex) {
        Promise promise(std::make_shared<Task>(Task::State(Task::Started | Task::Finished)));
        promise.task()->_exceptionStore = std::make_exception_ptr(std::move(ex));
        return promise;
    }

    /// Creates a promise that is in the 'exception' state.
    static Promise createFailed(std::exception_ptr ex_ptr) {
        Promise promise(std::make_shared<Task>(Task::State(Task::Started | Task::Finished)));
        promise.task()->_exceptionStore = std::move(ex_ptr);
        return promise;
    }

    /// Creates a promise without results that is in the canceled state.
    static Promise createCanceled() {
        return Promise(std::make_shared<Task>(Task::State(Task::Started | Task::Canceled | Task::Finished)));
    }

    /// Constructor
    Promise(TaskPtr p) noexcept : PromiseBase(std::move(p)) {}

    // Assigns a result to this promise.
    template<typename source_tuple_type>
    auto setResultsDirect(source_tuple_type&& results) -> std::enable_if_t<std::tuple_size_v<source_tuple_type> != 0> {
        static_assert(std::tuple_size_v<tuple_type> != 0, "Must not be an empty tuple");
        static_assert(std::is_same_v<tuple_type, std::decay_t<source_tuple_type>>, "Must assign a compatible tuple");
        task()->template setResults<tuple_type>(std::forward<source_tuple_type>(results));
    }

    // Assigns a result to this promise.
    template<typename value_type>
    void setResultsDirect(value_type&& result) {
        static_assert(std::tuple_size_v<tuple_type> == 1, "Must be a tuple of size 1");
        task()->template setResults<tuple_type>(std::forward_as_tuple(std::forward<value_type>(result)));
    }

#ifdef OVITO_DEBUG
    bool _futureCreated = false;
#endif

    template<typename... R2> friend class Future;
    template<typename... R2> friend class SharedFuture;
};

}	// End of namespace
