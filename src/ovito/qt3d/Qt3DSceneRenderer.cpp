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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/rendering/RenderSettings.h>
#include "Qt3DSceneRenderer.h"

namespace Ovito {


/******************************************************************************
* Constructor.
******************************************************************************/
Qt3DSceneRenderer::Qt3DSceneRenderer(DataSet* dataset) 
    : SceneRenderer(dataset)
{
}

/******************************************************************************
* Destructor.
******************************************************************************/
Qt3DSceneRenderer::~Qt3DSceneRenderer()
{
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void Qt3DSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, const QRect& viewportRect)
{
	SceneRenderer::beginFrame(time, params, vp, viewportRect);

    // Enable depth tests by default.
    setDepthTestEnabled(true);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool Qt3DSceneRenderer::renderFrame(FrameBuffer* frameBuffer, const QRect& viewportRect, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	// Render the 3D scene objects.
	if(renderScene(operation.subOperation())) {

		// Call virtual method to render additional content that is only visible in the interactive viewports.
        if(viewport() && isInteractive()) {
    		renderInteractiveContent();
        }
    }

	return !operation.isCanceled();
}

/******************************************************************************
* Temporarily enables/disables the depth test while rendering.
******************************************************************************/
void Qt3DSceneRenderer::setDepthTestEnabled(bool enabled)
{
    _depthTestEnabled = enabled;
}

/******************************************************************************
* Activates the special highlight rendering mode.
******************************************************************************/
void Qt3DSceneRenderer::setHighlightMode(int pass)
{
}

}	// End of namespace
