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

namespace Ovito {

/**
 * \brief Provides a Qt signal/slots interface to an asynchronous task.
 */
class OVITO_CORE_EXPORT TaskWatcher : public QObject
{
	Q_OBJECT

public:

	/// Constructor that creates a watcher that is not associated with
	/// any future/promise yet.
	explicit TaskWatcher(QObject* parent = nullptr) : QObject(parent) {}

	/// Destructor.
	virtual ~TaskWatcher() {
		watch(nullptr, false);
	}

	/// Returns whether this watcher is currently monitoring a shared state.
	bool isWatching() const { return (bool)_task; }

	/// Returns the shared state being monitored by this watcher.
	const TaskPtr& task() const { return _task; }

	/// Makes this watcher monitor the given shared state.
	void watch(const TaskPtr& promiseState, bool pendingAssignment = true);

	/// Detaches this watcher from the shared state.
	void reset() { watch(nullptr); }

	/// Returns true if the shared state monitored by this object has been canceled.
	bool isCanceled() const;

	/// Returns true if the shared state monitored by this object has reached the 'finished' state.
	bool isFinished() const;

	/// Returns the maximum value for the progress of the shared state monitored by this object.
	qlonglong progressMaximum() const;

	/// Returns the current value for the progress of the shared state monitored by this object.
	qlonglong progressValue() const;

	/// Returns the status text of the shared state monitored by this object.
	QString progressText() const;

public Q_SLOTS:

	/// Cancels the operation being watched by this watcher.
	void cancel();

Q_SIGNALS:

	void canceled();
	void finished();
	void started();
	void progressRangeChanged(qlonglong maximum);
	void progressValueChanged(qlonglong progressValue);
	void progressTextChanged(const QString& progressText);

private Q_SLOTS:

	void promiseCanceled();
	void promiseFinished();
	void promiseStarted();
	void promiseProgressRangeChanged(qlonglong maximum);
	void promiseProgressValueChanged(qlonglong progressValue);
	void promiseProgressTextChanged(const QString& progressText);

private:

	/// The shared state being monitored.
	TaskPtr _task;

	/// Indicates that the shared state has reached the 'finished' state.
    bool _finished = false;

	/// Linked list pointer for list of registered watchers of the current shared state.
	TaskWatcher* _nextInList;

	friend class Task;
	friend class ProgressiveTask;
};

}	// End of namespace
