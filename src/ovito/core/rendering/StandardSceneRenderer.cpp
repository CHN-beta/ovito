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
#include <ovito/core/app/Application.h>
#include <ovito/core/app/PluginManager.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "StandardSceneRenderer.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(StandardSceneRenderer);
DEFINE_PROPERTY_FIELD(StandardSceneRenderer, antialiasingLevel);
SET_PROPERTY_FIELD_LABEL(StandardSceneRenderer, antialiasingLevel, "Antialiasing level");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(StandardSceneRenderer, antialiasingLevel, IntegerParameterUnit, 1, 6);

/******************************************************************************
* Constructor.
******************************************************************************/
StandardSceneRenderer::StandardSceneRenderer(DataSet* dataset) : SceneRenderer(dataset), 
	_antialiasingLevel(3) 
{
}

/******************************************************************************
* Prepares the renderer for rendering an image or animation and sets the dataset being rendered.
******************************************************************************/
bool StandardSceneRenderer::startRender(DataSet* dataset, RenderSettings* settings, const QSize& frameBufferSize)
{
	if(!SceneRenderer::startRender(dataset, settings, frameBufferSize))
		return false;

	// Create the internal renderer implementation.
	// Choose between OpenGL and Vulkan option.
	OVITO_ASSERT(!_internalRenderer);
	OvitoClassPtr rendererClass = PluginManager::instance().findClass("VulkanRenderer", "OffscreenVulkanSceneRenderer");
	if(!rendererClass)
		rendererClass = PluginManager::instance().findClass("OpenGLRenderer", "OffscreenOpenGLSceneRenderer");
	if(!rendererClass)
		throwException(tr("The OffscreenOpenGLSceneRenderer class is not available. Please make sure the OpenGLRenderer plugin is installed correctly."));
	_internalRenderer = static_object_cast<SceneRenderer>(rendererClass->createInstance(this->dataset(), Application::instance()->executionContext()));

	// Pass supersampling level requested by the user to the renderer implementation.
	_internalRenderer->setAntialiasingHint(std::max(1, antialiasingLevel()));

	if(!_internalRenderer->startRender(dataset, settings, frameBufferSize))
		return false;

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void StandardSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);

	// Call implementation class.
	_internalRenderer->beginFrame(time, params, vp);
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool StandardSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, SynchronousOperation operation)
{
	// Delegate rendering work to implementation class.
	if(!_internalRenderer->renderFrame(frameBuffer, stereoTask, std::move(operation)))
		return false;

	return true;
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void StandardSceneRenderer::endFrame(bool renderingSuccessful, FrameBuffer* frameBuffer)
{
	// Call implementation class.
	_internalRenderer->endFrame(renderingSuccessful, frameBuffer);
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void StandardSceneRenderer::endRender()
{
	if(_internalRenderer) {
		// Call implementation class.
		_internalRenderer->endRender();
		// Release implementation.
		_internalRenderer.reset();
	}
	SceneRenderer::endRender();
}

}	// End of namespace