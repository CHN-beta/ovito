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
#include "detail/TaskWithStorage.h"
#include "Future.h"

namespace Ovito {

template<typename... R>
class AsynchronousTask : public detail::TaskWithStorage<std::tuple<R...>>, public QRunnable
{
public:

	/// Constructor.
	AsynchronousTask() {
		QRunnable::setAutoDelete(false);
	}

	/// Destructor.
	virtual ~AsynchronousTask() {
		// If task was never submitted for execution, cancel and finish it.
		if(!this->isFinished()) {
			this->cancel();
			this->setFinished();
		}
	}

#ifndef OVITO_DISABLE_THREADING
	/// Submits the task to a thread pool for execution and returns a future for the task's results.
	Future<R...> submit(QThreadPool* pool) {
		OVITO_ASSERT(!_thisTask);
		OVITO_ASSERT(!this->isStarted());

		// Store a shared_ptr to this task to keep it alive while running.
		_thisTask = this->shared_from_this();
		this->setStarted();
		pool->start(this);
		return Future<R...>::createFromTask(_thisTask);
	}
#endif

	/// Runs the task in place and returns a future for the task's results.
	Future<R...> runImmediately() {
		OVITO_ASSERT(!_thisTask);
		OVITO_ASSERT(!this->isStarted());

		this->setStarted();
		run();
		return Future<R...>::createFromTask(this->shared_from_this());
	}

	/// Sets the result value of the task.
	template<typename... R2>
	void setResult(R2&&... result) {
		this->template setResults<std::tuple<R...>>(std::forward_as_tuple(std::forward<R2>(result)...));
	}

    /// \brief Blocks execution until another future is complete. 
	/// \param future The future to wait for.
	/// \return false if either the future or this task have been canceled.
    bool waitForFuture(const FutureBase& future) {
		OVITO_ASSERT(false); // Not implemented yet.
		return false;
	}

	/// This virtual function is responsible for computing the results of the task.
	virtual void perform() = 0;

private:

	/// Implementation of QRunnable.
	virtual void run() override {
		OVITO_ASSERT(this->isStarted());
		try {
			perform();
		}
		catch(...) {
			this->captureException();
		}
		this->setFinished();
		this->_thisTask.reset(); // No need to keep the task object alive any longer.
	}

	/// A shared pointer to the task itself, which is used to keep the C++ object alive
	/// while the task is running in a thread pool.
	TaskPtr _thisTask;

	friend class TaskManager;
};

}	// End of namespace
