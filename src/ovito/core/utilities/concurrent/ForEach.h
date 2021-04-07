////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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

template<typename WorkingState, typename InputRange, typename F, class Executor>
auto for_each(WorkingState&& inputState, InputRange&& inputRange, Executor&& executor, F&& f) noexcept
{
    class ForEachTask : public ThreadSafeTask
    {
    public:
        using WorkingStateValue = std::decay_t<WorkingState>;

		/// Constructor.
		ForEachTask(WorkingState&& inputState, InputRange&& inputRange, Executor&& executor, F&& f) : 
			ThreadSafeTask(Task::Started, executor.taskManager()),
			_state(std::forward<WorkingState>(inputState)),
			_range(std::forward<InputRange>(inputRange)),
            _executor(std::forward<Executor>(executor)),
            _func(std::forward<F>(f)),
            _iterator(std::begin(_range)) {}

		/// Creates a future that returns the results of this asynchronous task to the caller.
		Future<WorkingStateValue> future() {
			return Future<WorkingStateValue>::createFromTask(shared_from_this(), _state);
		}

		/// Is called when this task gets canceled by the system.
		virtual void cancel() noexcept override {
            if(isCanceled() || isFinished()) return;
			_future.reset(); // Cancel function call that is currently in progress.
			ThreadSafeTask::cancel();
			setFinished();
		}

		void go() {
			if(isCanceled()) return;
			if(_iterator != std::end(_range)) {
				_future = _func(*_iterator++, _state);
				_future.finally(_executor, true, 
					std::bind(&ForEachTask::next, static_pointer_cast<ForEachTask>(shared_from_this()), std::placeholders::_1));
			}
			else {
#ifdef OVITO_DEBUG
        		this->_resultSet = true;
#endif	
				setFinished();
			}
		}

        void next(const TaskPtr& task) {
			try {
				if(!isCanceled() && !task->isCanceled()) {
					OVITO_ASSERT(_future.isValid());
					_future.result();
					_future.reset();
					go();
				}
				else cancel();
			}
			catch(...) {
				captureException();
				setFinished();
			}
        }

	private:

		/// The working state of this asynchronous task, which is also the result being returned to the caller.
		WorkingStateValue _state;

        /// The range of items to be processed.
        std::decay_t<InputRange> _range;

        /// The user function to call on each item of the input range.
        F _func;

        /// The executor used for sub-tasks.
        Executor _executor;

        /// The iterator pointing to the current item from the range.
        typename InputRange::iterator _iterator;

        /// The future returned by the last call to the user function.
		Future<> _future;
    };

	// Create the asynchronous task object.
	std::shared_ptr<ForEachTask> task = std::make_shared<ForEachTask>(
        std::forward<WorkingState>(inputState), std::forward<InputRange>(inputRange), 
        std::forward<Executor>(executor), std::forward<F>(f));
	task->go();
	return task->future();
}

}	// End of namespace
