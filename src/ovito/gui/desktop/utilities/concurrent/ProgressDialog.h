////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 OVITO GmbH, Germany
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>

namespace Ovito {

class OVITO_GUI_EXPORT ProgressDialog : public QDialog
{
public:

	/// Constructor.
	ProgressDialog(QWidget* parent, TaskManager& taskManager, const QString& dialogTitle = QString());

	/// Destructor.
	~ProgressDialog();

	/// Returns the TaskManager that manages the running task displayed in this progress dialog.
	TaskManager& taskManager() { return _taskManager; }

	/// Create a new synchronous operation, whose progress will be displayed in this dialog.
	SynchronousOperation createOperation();

	/// Shows the progress of the given task in this dialog.
	void registerTask(const TaskPtr& task);

protected:

	/// Is called when the user tries to close the dialog.
	virtual void closeEvent(QCloseEvent* event) override;

	/// Is called when the user tries to close the dialog.
	virtual void reject() override;

private:

	/// The task manager.
	TaskManager& _taskManager;
};

}	// End of namespace
