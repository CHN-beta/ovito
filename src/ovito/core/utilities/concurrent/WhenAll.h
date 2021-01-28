////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include "ThreadSafeTask.h"

namespace Ovito {
namespace detail {

template<typename input_tuple_type>
class WhenAllTask : public ThreadSafeTask
{
public:

	/// Constructor.
	WhenAllTask(std::vector<TaskDependency> continuedTasks) :
		ThreadSafeTask(Task::Started),
		_inputTasks(std::move(continuedTasks))
	{
        // Inform the Task class about the storage location of the task's results.
        this->_resultsTuple = &_inputTasksResults;
		_inputTasksResults.reserve(_inputTasks.size());
	}

	/// This method should be called right after the task object has been constructed.
	void go() {

		for(const TaskDependency& inputTask : _inputTasks) {
			OVITO_ASSERT_MSG(inputTask, "when_all()", "All futures must be valid.");

			// Register a continuation function with each input task, which gets executed once the task finishes.
			// The lambda keeps a shared_ptr reference to the parent task to keep it alive as long as any of the input 
			// tasks are running.
			inputTask->finally(Ovito::detail::InlineExecutor(), false, [this_ = shared_from_this()](const TaskPtr& finishedTask) mutable noexcept {
				static_pointer_cast<WhenAllTask>(std::move(this_))->inputTaskFinished(finishedTask);
			});
		}

		// If we are waiting for zero input tasks, then we are done immediately.
		if(_inputTasks.empty()) {
			setFinished();
		}
	}

	/// Cancels this task.
	virtual void cancel() noexcept override {
		if(isCanceled() || isFinished()) return;

		// Keep this object alive for the duration of the method call.
		TaskPtr selfLock = shared_from_this();

		// Call base class implementation.
		ThreadSafeTask::cancel();

		// Discard all remaining dependencies on the input tasks.
		QMutexLocker locker(&this->_mutex);
		_inputTasks.clear();
	}

private:

	/// This callback function is invoked whenever one of the input tasks this task is waiting for has finished.
	inline void inputTaskFinished(const TaskPtr& finishedTask) {
		// Lock this task (the dependent task) to synchronize access to its data.
		QMutexLocker locker(&this->_mutex);

		// If the waiting task has already been marked as finished, because another input task has returned an error state,
		// then ignore any results from the current input task.
		if(isFinished()) {
			return;
		}

		// If the dependent task has been canceled in the meantime, there is no need to collect the results from the 
		// input task we were waiting for.
		if(isCanceled()) {
			OVITO_ASSERT(_inputTasks.empty());
			locker.unlock();
			setFinished();
			return;
		}

		// If the input task we are waiting for was canceled for some reason, cancel the dependent task as well.
		OVITO_ASSERT(finishedTask);
		OVITO_ASSERT(finishedTask->isFinished());
		if(finishedTask->isCanceled()) {
			locker.unlock();
			cancel();
			setFinished();
			return;
		}

		// If the input task returned an exception state, adopt that exception state in the continuation task.
		if(finishedTask->exceptionStore()) {
			locker.unlock();
			setException(finishedTask->takeExceptionStore());
			setFinished();
			return;
		}

		// Move the results of the finished input task into this task.
		auto iter = _inputTasksResults.begin();
		for(auto& inputTask : _inputTasks) {
			if(!inputTask) {
				OVITO_ASSERT(iter != _inputTasksResults.end());
				++iter;
			}
			else if(inputTask.get() == finishedTask) {
				_inputTasksResults.insert(iter, finishedTask->template takeResults<input_tuple_type>());
				inputTask.reset();

				// Check if we have gathered the results of all input tasks by now.
				if(_inputTasksResults.size() == _inputTasks.size()) {
#ifdef OVITO_DEBUG
					// This is used in debug builds to detect programming errors and explicitly keep track of whether a result has
					// been assigned to the task.
					this->_resultSet = true;
#endif
					locker.unlock();
					setFinished();
				}
				return;
			}
		}
		OVITO_ASSERT(false);
	}

private:

	/// The tasks this continuation task depends on.
	std::vector<TaskDependency> _inputTasks;

	/// The list of result values returned by the input tasks.
	std::vector<input_tuple_type> _inputTasksResults;
};

}	// End of namespace

/// Returns a new future that, upon the fulfillment of all futures in the given list, will be fulfilled with the list of results from the input futures.
template<typename R>
Future<std::vector<R>> when_all(std::vector<Future<R>>&& futures) noexcept 
{
	// Infer the exact future/promise/task types to create.
	using input_tuple_type = std::tuple<R>;
	using continuation_task_type = Ovito::detail::WhenAllTask<input_tuple_type>;

	// The dependencies on the input tasks are moved from the Future objects into the continuation task.
	std::vector<TaskDependency> inputTasks;
	for(auto& inputFuture : futures) {
		inputTasks.push_back(inputFuture.takeTaskDependency());
	}

	// Create the continuation task.
	auto task = std::make_shared<continuation_task_type>(std::move(inputTasks));
	// Start the task.
	task->go();

	// Return a Future to the caller, which is linked to the task.
	return Future<std::vector<R>>::createFromTask(std::move(task));
}

}	// End of namespace
