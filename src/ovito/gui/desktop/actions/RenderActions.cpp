////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2021 OVITO GmbH, Germany
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/actions/WidgetActionManager.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include <ovito/gui/desktop/widgets/rendering/FrameBufferWindow.h>
#include <ovito/gui/desktop/utilities/concurrent/ProgressDialog.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/viewport/ViewportConfiguration.h>

namespace Ovito {

/******************************************************************************
* Handles the ACTION_RENDER_ACTIVE_VIEWPORT command.
******************************************************************************/
void WidgetActionManager::on_RenderActiveViewport_triggered()
{
	try {
		// Set input focus to main window.
		// This will process any pending user inputs in QLineEdit fields that haven't been processed yet.
		mainWindow().setFocus();

		// Stop animation playback.
		dataset()->animationSettings()->stopAnimationPlayback();

		// Get the current render settings.
		RenderSettings* renderSettings = dataset()->renderSettings();

		// Get the current viewport configuration.
		ViewportConfiguration* viewportConfig = dataset()->viewportConfig();

		// Create a task object that represents the rendering operation.
		MainThreadOperation renderingOperation = MainThreadOperation::create(mainWindow(), true);

		// Allocate and resize frame buffer and display the frame buffer window.
		std::shared_ptr<FrameBuffer> frameBuffer = mainWindow().createAndShowFrameBuffer(renderSettings->outputImageWidth(), renderSettings->outputImageHeight(), renderingOperation);

		// Call high-level rendering function, which will take care of the rest.
		dataset()->renderScene(renderSettings, viewportConfig, frameBuffer.get(), renderingOperation);
	}
	catch(Exception& ex) {
		ex.logError();
		
		// Make sure the error message dialog gets shown in front of the framebuffer window by GuiApplication::showErrorMessages(), not behind it.
		// This can be achieved by associating the error with the framebuffer window. 
		if(mainWindow().frameBufferWindow() && mainWindow().frameBufferWindow()->isVisible())
			ex.setContext(mainWindow().frameBufferWindow());

		ex.reportError();
	}
}

}	// End of namespace
