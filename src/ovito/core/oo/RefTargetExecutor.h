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
#include <ovito/core/oo/ExecutionContext.h>
#include <ovito/core/oo/RefTarget.h>

namespace Ovito {

/**
 * \brief An executor that can be used with Future<>::then(), which runs the closure
 *        routine in the context (and in the thread) of this RefTarget.
 */
class OVITO_CORE_EXPORT RefTargetExecutor
{
public:

	/// Constructor.
	RefTargetExecutor(const RefTarget* obj, ExecutionContext executionContext, bool deferredExecution) noexcept : _obj(obj), _executionContext(executionContext), _deferredExecution(deferredExecution) { 
		OVITO_ASSERT(obj != nullptr); 
	}

	/// Creates some work that can be submitted for execution later.
	template<typename Function>
	auto schedule(Function&& f) {

		/// Helper class that is used by this executor to transmit a callable object
		/// to the UI thread where it is executed in the context on a RefTarget.
		class WorkEvent : public QEvent, public RefTargetExecutor
		{
		public:

			/// Constructor.
			WorkEvent(RefTargetExecutor&& executor, std::decay_t<Function>&& f, TaskPtr task) :
				QEvent(workEventType()), 
				RefTargetExecutor(std::move(executor)), 
				_callable(std::move(f)),
				_task(std::move(task)) { OVITO_ASSERT(this->object()); }

			/// Event destructor, which runs the work function.
			virtual ~WorkEvent() {
				// Qt events should only be destroyed in the main thread.
				OVITO_ASSERT(!QCoreApplication::instance() || QCoreApplication::closingDown() || QThread::currentThread() == QCoreApplication::instance()->thread());

				if(!QCoreApplication::closingDown()) {
					/// Activate the original execution context under which the work was submitted.
					activateExecutionContext();
					// Execute the work function.
					if constexpr(std::is_invocable_v<Function, Task&>)
						std::move(_callable)(*_task);
					else
						std::move(_callable)();
					/// Restore the execution context as it was before the work was executed.
					restoreExecutionContext();
				}
			}

		private:
			std::decay_t<Function> _callable;
			const TaskPtr _task;
		};
				
		if constexpr(std::is_invocable_v<Function, Task&>) {
			return [f = std::forward<Function>(f), *this](Task& task) mutable noexcept {					
				OVITO_ASSERT(this->object()); 
				if(_deferredExecution || (QCoreApplication::instance() && QThread::currentThread() != object()->thread())) {
					if(!QCoreApplication::closingDown()) {
						// When not in the main thread, schedule work for later execution in the main thread.
						WorkEvent* event = new WorkEvent(std::move(*this), std::move(f), task.shared_from_this());
						OVITO_ASSERT(event->object()); 
						QCoreApplication::postEvent(const_cast<RefTarget*>(event->object()), event);
					}
				}
				else {
					// When already in the main thread, execute work immediately.
					// Activate the original execution context under which the work was submitted.
					activateExecutionContext();
					// Execute the work function.
					std::move(f)(task);
					// Restore the execution context as it was before the work was executed.
					restoreExecutionContext();
				}
			};
		}
		else {
			return [f = std::forward<Function>(f), *this]() mutable noexcept {
				OVITO_ASSERT(this->object()); 
				if(_deferredExecution || (QCoreApplication::instance() && QThread::currentThread() != object()->thread())) {
					if(!QCoreApplication::closingDown()) {
						// When not in the main thread, schedule work for later execution in the main thread.
						WorkEvent* event = new WorkEvent(std::move(*this), std::move(f), nullptr);
						OVITO_ASSERT(event->object()); 
						QCoreApplication::postEvent(const_cast<RefTarget*>(event->object()), event);
					}
				}
				else {
					// When already in the main thread, execute work immediately.
					// Activate the original execution context under which the work was submitted.
					activateExecutionContext();
					// Execute the work function.
					std::move(f)();
					// Restore the execution context as it was before the work was executed.
					restoreExecutionContext();
				}
			};
		}
	}

	/// Returns the RefTarget this executor is associated with.
	/// Work submitted to this executor will be executed in the context of the RefTarget.
	const RefTarget* object() const { return _obj.get(); }

	/// Returns the unique Qt event type ID used by this class to schedule asynchronous work.
	static QEvent::Type workEventType() {
		static const int _workEventType = QEvent::registerEventType();
		return static_cast<QEvent::Type>(_workEventType);
	}

private:

	/// Activates the original execution context under which the work was submitted.
	void activateExecutionContext();

	/// Restores the execution context as it was before the work was executed.
	void restoreExecutionContext();

private:

	/// The object the work is submitted to.
	OORef<const RefTarget> _obj;

	/// The execution context (interactive or scripting) under which the work has been submitted.
	ExecutionContext _executionContext;

	/// Controls whether execution of the work will be deferred even if immediate execution would be possible.
	const bool _deferredExecution;
};

}	// End of namespace
