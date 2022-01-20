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
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include "RefTargetExecutor.h"

namespace Ovito {

/******************************************************************************
* Activates the original execution context under which the work was submitted.
******************************************************************************/
void RefTargetExecutor::activateExecutionContext()
{
    if(Application* app = Application::instance()) {
        ExecutionContext previousContext = app->executionContext();
        app->switchExecutionContext(_executionContext);
        _executionContext = previousContext;

        // In the current implementation, deferred work is always executed without undo recording.
        // Thus, we should suspend the undo stack while running the work function.
        if(object()->dataset()) 
            object()->dataset()->undoStack().suspend();
    }
}

/******************************************************************************
* Restores the execution context as it was before the work was executed.
******************************************************************************/
void RefTargetExecutor::restoreExecutionContext()
{
    if(Application* app = Application::instance()) {
        ExecutionContext previousContext = app->executionContext();
        app->switchExecutionContext(_executionContext);
        _executionContext = previousContext;

        // Restore undo recording state.
        if(object()->dataset()) 
            object()->dataset()->undoStack().resume();
    }
}

}	// End of namespace
