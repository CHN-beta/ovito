///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <core/Core.h>
#include "Task.h"

#include <QThreadPool>
#include <QMetaObject>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * \brief Manages the background tasks.
 */
class OVITO_CORE_EXPORT TaskManager : public QObject
{
	Q_OBJECT

private:

	/// Constructor.
	TaskManager(DataSetContainer& owner);

	/// Destructor.
	~TaskManager();

	/// Only DataSetContainer can create a TaskManager.
	friend class DataSetContainer;

public:

	/// Returns the dataset container owning this task manager.
	DataSetContainer& datasetContainer() { return _owner; }

    /// \brief Returns the watchers for all currently running tasks.
    /// \return A list of TaskWatcher objects, one for each registered task that is currently in the 'started' state.
    /// \note This method is *not* thread-safe and may only be called from the main thread.
	const std::vector<TaskWatcher*>& runningTasks() const { return _runningTaskStack; }

	/// \brief Executes an asynchronous task in a background thread.
	///
	/// This function is thread-safe. It returns a Future that is fulfilled when the task completed.
	template<class TaskType>
	auto runTaskAsync(const std::shared_ptr<TaskType>& task) {
		OVITO_ASSERT(task);
		QThreadPool::globalInstance()->start(task.get());
		registerTask(task);
		return task->future();
	}

    /// \brief Registers a future with the TaskManager, which will subsequently track the progress of the associated operation.
    /// \param future The Future whose shared state should be registered.
    /// \note This function is thread-safe.
    /// \sa registerTask()
    /// \sa FutureBase::task()
    void registerFuture(const FutureBase& future);

    /// \brief Registers a promise with the TaskManager, which will subsequently track the progress of the associated operation.
    /// \param promise The Promise whose shared state should be registered.
    /// \note This function is thread-safe.
    /// \sa registerTask()
    /// \sa PromiseBase::task()
    void registerPromise(const PromiseBase& promise);

    /// \brief Registers a Task with the TaskManager, which will subsequently track the progress of the associated operation.
    /// \note This function is thread-safe.
	void registerTask(const TaskPtr& task);

    /// \brief Creates a new promise for an asynchronous operation executing in the main thread and registers it with the TaskManager.
    /// \param startedState If true, the new task is put into the 'started' state right away. Otherwise, it must be put into the 'started' via PromiseBase::setStarted() by the caller.
    /// \tparam R The result value type of the operation.
    /// \return The Promise which allows controlling the task and setting the result value of the operation.
    /// \note This method may only be called from the main thread.
	template<typename... R>
	Promise<R...> createMainThreadOperation(bool startedState) {
		using tuple_type = std::tuple<R...>;
		Promise<R...> promise(std::make_shared<TaskWithResultStorage<MainThreadTask, tuple_type>>(
			typename TaskWithResultStorage<MainThreadTask, tuple_type>::no_result_init_t(),
			startedState ? Task::State(Task::Started) : Task::NoState, *this));
		addTaskInternal(promise.task());
		return promise;
	}

    /// \brief Waits for the given future to be fulfilled and displays a modal progress dialog to show the progress.
    /// \return False if the operation has been cancelled by the user.
    ///
    /// This function must be called from the main thread.
    bool waitForFuture(const FutureBase& future);

    /// \brief Waits for the given task to finish.
	/// \param task The task for which to wait.
	/// \param dependentTask Optionally another task that is waiting for \a task. The method will return early if the dependent task has been canceled.
	/// \return false if either \a task or \a dependentTask have been canceled.
    bool waitForTask(const TaskPtr& task, const TaskPtr& dependentTask = {});

	/// \brief Process events from the event queue when the tasks manager has started
	///        a local event loop. Otherwise does nothing and lets the main event loop
	///        do the processing.
	void processEvents();

	/// \brief This should be called whenever a local event handling loop is entered.
	void startLocalEventHandling();

	/// \brief This should be called whenever a local event handling loop is left.
	void stopLocalEventHandling();

public Q_SLOTS:

	/// Cancels all running tasks.
	void cancelAll();

	/// Cancels all running tasks and waits for them to finish.
	void cancelAllAndWait();

	/// Waits for all running tasks to finish.
	void waitForAll();

Q_SIGNALS:

    /// \brief This signal is generated by the task manager whenever one of the registered tasks started to run.
    /// \param watcher The TaskWatcher can be used to track the operation's progress.
	void taskStarted(TaskWatcher* taskWatcher);

    /// \brief This signal is generated by the task manager whenever one of the registered tasks has finished or stopped running.
    /// \param watcher The TaskWatcher that was used by the task manager to track the running task.
	void taskFinished(TaskWatcher* taskWatcher);

private:

	/// \brief Registers a promise with the progress manager.
	Q_INVOKABLE TaskWatcher* addTaskInternal(const TaskPtr& sharedState);

private Q_SLOTS:

	/// \brief Is called when a task has started to run.
	void taskStartedInternal();

	/// \brief Is called when a task has finished.
	void taskFinishedInternal();

private:

	/// The list of watchers for the active tasks.
	std::vector<TaskWatcher*> _runningTaskStack;

	/// Indicates that waitForTask() has started a local event loop.
	int _inLocalEventLoop = 0;

	/// The dataset container owning this task manager.
	DataSetContainer& _owner;

	// Needed by MainThreadTask::createSubOperation():
	friend class MainThreadTask;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::TaskPtr);


