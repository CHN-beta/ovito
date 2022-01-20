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
#include <ovito/core/app/Application.h>
#include <ovito/core/app/UserInterface.h>
#include <ovito/core/utilities/concurrent/MainThreadOperation.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/utilities/concurrent/detail/TaskCallback.h>

#ifdef Q_OS_UNIX
	#include <csignal>
#endif

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
MainThreadOperation::MainThreadOperation(TaskPtr p, UserInterface& userInterface, ObjectInitializationHints initializationHints, bool visibleInUserInterface) noexcept :
	Promise<>(std::move(p)), _userInterface(userInterface), _initializationHints(initializationHints)
{
	OVITO_ASSERT(isValid());
	OVITO_ASSERT(isStarted());
	OVITO_ASSERT(task()->isProgressingTask());

	// Register the container MainThreadOperation with the TaskManager to display its progress in the UI.
	if(visibleInUserInterface)
		userInterface.taskManager().registerTask(this->task());
}

/******************************************************************************
* Creates a separate MainThreadOperation that represents a sub-task of the running operation.
* If the parent task gets canceled, the sub-task is canceled as well, and vice versa.
******************************************************************************/
MainThreadOperation MainThreadOperation::createSubTask(bool visibleInUserInterface) 
{
	OVITO_ASSERT(isValid());

	class MainThreadSubTask : public ProgressingTask, public detail::TaskCallback<MainThreadSubTask>
	{
	public:
		MainThreadSubTask(const TaskPtr& parentTask) noexcept : ProgressingTask(Task::Started) {
			// Register a callback function to get notified when the parent task gets canceled.
			registerCallback(parentTask.get(), true);
		}

		/// Callback function, which is invoked by the Task base class whenever the state of the task changes.
		void taskStateChangedCallback(int state) noexcept {
			if(state & Canceled)
				this->cancel();
		}
	};

	return MainThreadOperation(std::make_shared<MainThreadSubTask>(task()), userInterface(), initializationHints(), visibleInUserInterface);
}

/******************************************************************************
* Suspends execution until the given task has reached the 'finished' state. 
* If the awaited task gets canceled while waiting, this task gets canceled too.
******************************************************************************/
bool MainThreadOperation::waitForTask(const TaskPtr& awaitedTask)
{
	OVITO_ASSERT(isValid());
	OVITO_ASSERT(awaitedTask);
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "MainThreadOperation::waitForTask", "Function may be called only from the main thread.");

	// Lock access to this task.
	QMutexLocker thisTaskLocker(&task()->taskMutex());

	// No need to wait for the other task if this task is already canceled.
	if(isCanceled())
		return false;

	// You should never invoke waitForTask() on a task object that has already finished.
	OVITO_ASSERT(!isFinished());

	// Before entering the costly local event loop, quick check if the awaited task has already finished.
	QMutexLocker awaitedTaskLocker(&awaitedTask->taskMutex());
	if(awaitedTask->isFinished()) {
		if(awaitedTask->isCanceled()) {
			// If the awaited was canceled, cancel this task as well.
			task()->cancelAndFinishLocked(thisTaskLocker);
			return false;
		}
		else {
			// It's ready, no need to wait.
			return true;
		}
	}

	// Create shared pointers on the stack to make sure the two task don't get 
	// destroyed during or right after the waiting phase and before we access them
	// again below.
	TaskPtr thisTaskPtr(this->task());
	TaskPtr awaitedTaskPtr(awaitedTask);

	thisTaskLocker.unlock();
	awaitedTaskLocker.unlock();

	// The local event loop we are going to start.
	QEventLoop eventLoop;

	// Register a callback function with the waiting task, which makes the event loop quit in case the waiting task gets canceled.
	detail::FunctionTaskCallback thisTaskCallback(thisTaskPtr.get(), [&eventLoop](int state) {
		if(state & (Task::Canceled | Task::Finished))
			QMetaObject::invokeMethod(&eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
	});

	// Register a callback function with the awaited task, which makes the event loop quit when the task gets canceled or finishes.
	detail::FunctionTaskCallback awaitedfTaskCallback(awaitedTaskPtr.get(), [&eventLoop](int state) {
		if(state & Task::Finished)
			QMetaObject::invokeMethod(&eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
	});

#ifdef Q_OS_UNIX
	// Boolean flag which is set by the POSIX signal handler when user
	// presses Ctrl+C to interrupt the program.
	static QAtomicInt userInterrupt;
	userInterrupt.storeRelease(0);

	// Install POSIX signal handler to catch Ctrl+C key signal.
	static QAtomicPointer<QEventLoop> activeEventLoop;
	QEventLoop* previousEventLoop = activeEventLoop.fetchAndStoreRelease(&eventLoop);
	auto oldSignalHandler = ::signal(SIGINT, [](int) {
		userInterrupt.storeRelease(1);
		if(QEventLoop* eventLoop = activeEventLoop.loadAcquire())
			QMetaObject::invokeMethod(eventLoop, &QEventLoop::quit, Qt::QueuedConnection);
	});
#endif

	// If this method was called as part of a script, temporarily switch to interactive mode now since
	// the user may perform actions in the user interface while the local event loop is active.
	bool wasCalledFromScript = (Application::instance()->executionContext() == ExecutionContext::Scripting);
	if(wasCalledFromScript)
		Application::instance()->switchExecutionContext(ExecutionContext::Interactive);

	// Enter the local event loop.
	eventLoop.exec();

	// Restore previous execution context state.
	if(wasCalledFromScript)
		Application::instance()->switchExecutionContext(ExecutionContext::Scripting);

	thisTaskCallback.unregisterCallback();
	awaitedfTaskCallback.unregisterCallback();

	thisTaskLocker.relock();
	OVITO_ASSERT(isValid());

#ifdef Q_OS_UNIX
	// Cancel the task if user pressed Ctrl+C.
	::signal(SIGINT, oldSignalHandler);
	activeEventLoop.storeRelaxed(previousEventLoop);
	if(userInterrupt.loadAcquire()) {
		thisTaskPtr->cancelAndFinishLocked(thisTaskLocker);
		return false;
	}
#endif

	// Check if the waiting task has been canceled.
	if(isCanceled())
		return false;

	// Now check if the awaited task has been canceled.
	awaitedTaskLocker.relock();

	if(awaitedTaskPtr->isCanceled()) {
		// If the awaited task was canceled, cancel this task as well.
		thisTaskPtr->cancelAndFinishLocked(thisTaskLocker);
		return false;
	}

	OVITO_ASSERT(awaitedTaskPtr->isFinished());
	return true;
}

}	// End of namespace
