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

#include <ovito/core/Core.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(SceneRenderer);
IMPLEMENT_OVITO_CLASS(ObjectPickInfo);

/******************************************************************************
* Constructor.
******************************************************************************/
SceneRenderer::SceneRenderer(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Returns the final size of the rendered image in pixels.
******************************************************************************/
QSize SceneRenderer::outputSize() const
{
	return { renderSettings()->outputImageWidth(), renderSettings()->outputImageHeight() };
}

/******************************************************************************
* Computes the bounding box of the entire scene to be rendered.
******************************************************************************/
Box3 SceneRenderer::computeSceneBoundingBox(TimePoint time, const ViewProjectionParameters& params, Viewport* vp, SynchronousOperation operation)
{
	OVITO_CHECK_OBJECT_POINTER(renderDataset()); // startRender() must be called first.

	try {
		_sceneBoundingBox.setEmpty();
		_isBoundingBoxPass = true;
		_time = time;
		_viewport = vp;
		setProjParams(params);

		// Perform bounding box rendering pass.
		if(renderScene(operation.subOperation())) {

			// Include other visual content that is only visible in the interactive viewports.
			if(isInteractive())
				renderInteractiveContent();

			// Include three-dimensional content from viewport layers in the bounding box.
			if(vp && (!isInteractive() || vp->renderPreviewMode())) {
				for(ViewportOverlay* layer : vp->underlays()) {
					if(layer->isEnabled())
						layer->render3D(vp, time, this, operation.subOperation());
				}
				for(ViewportOverlay* layer : vp->overlays()) {
					if(layer->isEnabled())
						layer->render3D(vp, time, this, operation.subOperation());
				}
			}
		}

		_isBoundingBoxPass = false;
	}
	catch(...) {
		_isBoundingBoxPass = false;
		throw;
	}

	return _sceneBoundingBox;
}

/******************************************************************************
* Renders all nodes in the scene
******************************************************************************/
bool SceneRenderer::renderScene(SynchronousOperation operation)
{
	OVITO_CHECK_OBJECT_POINTER(renderDataset());

	if(RootSceneNode* rootNode = renderDataset()->sceneRoot()) {
		// Recursively render all scene nodes.
		return renderNode(rootNode, operation.subOperation());
	}

	return true;
}

/******************************************************************************
* Render a scene node (and all its children).
******************************************************************************/
bool SceneRenderer::renderNode(SceneNode* node, SynchronousOperation operation)
{
    OVITO_CHECK_OBJECT_POINTER(node);

    // Set up transformation matrix.
	TimeInterval interval;
	const AffineTransformation& nodeTM = node->getWorldTransform(time(), interval);
	setWorldTransform(nodeTM);

	if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(node)) {

		// Do not render node if it is the view node of the viewport or
		// if it is the target of the view node.
		if(!viewport() || !viewport()->viewNode() || (viewport()->viewNode() != node && viewport()->viewNode()->lookatTargetNode() != node)) {

			// Evaluate data pipeline of object node and render the results.
			PipelineEvaluationFuture pipelineEvaluation;
			if(waitForLongOperationsEnabled()) {
				pipelineEvaluation = pipeline->evaluateRenderingPipeline(time());
				if(!operation.waitForFuture(pipelineEvaluation))
					return false;

				// After the rendering process has been temporarily interrupted above, rendering is resumed now.
				// Give the renderer the opportunity to restore any state that must be active (e.g. the active OpenGL context).
				resumeRendering();
			}
			const PipelineFlowState& state = pipelineEvaluation.isValid() ?
												pipelineEvaluation.result() :
												pipeline->evaluatePipelineSynchronous(true);

			// Invoke all vis elements of all data objects in the pipeline state.
			std::vector<const DataObject*> objectStack;
			if(state)
				renderDataObject(state.data(), pipeline, state, objectStack);
			OVITO_ASSERT(objectStack.empty());
		}
	}

	// Render trajectory when node transformation is animated.
	if(isInteractive() && !isPicking()) {
		renderNodeTrajectory(node);
	}

	// Render child nodes.
	for(SceneNode* child : node->children()) {
		if(!renderNode(child, operation.subOperation()))
			return false;
	}

	return !operation.isCanceled();
}

/******************************************************************************
* Renders a data object and all its sub-objects.
******************************************************************************/
void SceneRenderer::renderDataObject(const DataObject* dataObj, const PipelineSceneNode* pipeline, const PipelineFlowState& state, std::vector<const DataObject*>& objectStack)
{
	bool isOnStack = false;

	// Call all vis elements of the data object.
	for(DataVis* vis : dataObj->visElements()) {
		// Let the PipelineSceneNode substitude the vis element with another one.
		vis = pipeline->getReplacementVisElement(vis);
		if(vis->isEnabled()) {
			// Push the data object onto the stack.
			if(!isOnStack) {
				objectStack.push_back(dataObj);
				isOnStack = true;
			}
			try {
				// Let the vis element do the rendering.
				vis->render(time(), objectStack, state, this, pipeline);
			}
			catch(const Exception& ex) {
				ex.logError();
			}
		}
	}

	// Recursively visit the sub-objects of the data object and render them as well.
	dataObj->visitSubObjects([&](const DataObject* subObject) {
		// Push the data object onto the stack.
		if(!isOnStack) {
			objectStack.push_back(dataObj);
			isOnStack = true;
		}
		renderDataObject(subObject, pipeline, state, objectStack);
		return false;
	});

	// Pop the data object from the stack.
	if(isOnStack) {
		objectStack.pop_back();
	}
}

/******************************************************************************
* Gets the trajectory of motion of a node. The returned data buffer stores an 
* array of Point3 (if the node's position is animated) or a null pointer 
* (if the node's position is static).
******************************************************************************/
ConstDataBufferPtr SceneRenderer::getNodeTrajectory(const SceneNode* node)
{
	DataBufferAccessAndRef<Point3> vertices;
	Controller* ctrl = node->transformationController();
	if(ctrl && ctrl->isAnimated()) {
		AnimationSettings* animSettings = node->dataset()->animationSettings();
		int firstFrame = animSettings->firstFrame();
		int lastFrame = animSettings->lastFrame();
		OVITO_ASSERT(lastFrame >= firstFrame);
		vertices = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, lastFrame - firstFrame + 1, DataBuffer::Float, 3, 0, false);
		auto v = vertices.begin();
		for(int frame = firstFrame; frame <= lastFrame; frame++) {
			TimeInterval iv;
			const Vector3& pos = node->getWorldTransform(animSettings->frameToTime(frame), iv).translation();
			*v++ = Point3::Origin() + pos;
		}
		OVITO_ASSERT(v == vertices.end());
	}
	return vertices.take();
}

/******************************************************************************
* Renders the trajectory of motion of a node in the interactive viewports.
******************************************************************************/
void SceneRenderer::renderNodeTrajectory(const SceneNode* node)
{
	// Do not render the trajectory of the camera node of the viewport.
	if(viewport() && viewport()->viewNode() == node) return;

	if(ConstDataBufferPtr trajectory = getNodeTrajectory(node)) {
		setWorldTransform(AffineTransformation::Identity());

		if(!isBoundingBoxPass()) {

			// Render lines connecting the trajectory points.
			if(trajectory->size() >= 2) {
				DataBufferAccessAndRef<Point3> lineVertices = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, (trajectory->size() - 1) * 2, DataBuffer::Float, 3, 0, false);
				ConstDataBufferAccess<Point3> trajectoryPoints(trajectory);
				for(size_t index = 0; index < trajectory->size(); index++) {
					if(index != 0)
						lineVertices[index * 2 - 1] = trajectoryPoints[index];
					if(index != trajectory->size() - 1)
						lineVertices[index * 2] = trajectoryPoints[index];
				}
				std::shared_ptr<LinePrimitive> trajLine = createLinePrimitive();
				trajLine->setPositions(lineVertices.take());
				trajLine->setUniformColor(ColorA(1.0, 0.8, 0.4));
				renderLines(trajLine);
			}

			// Render the trajectory points themselves using marker primitives.
			std::shared_ptr<MarkerPrimitive> frameMarkers = createMarkerPrimitive(MarkerPrimitive::DotShape);
			frameMarkers->setPositions(std::move(trajectory));
			frameMarkers->setColor(ColorA(1, 1, 1));
			renderMarkers(frameMarkers);
		}
		else {
			Box3 bb;
			bb.addPoints(ConstDataBufferAccess<Point3>(trajectory));
			addToLocalBoundingBox(bb);
		}
	}
}

/******************************************************************************
* Renders the visual representation of the modifiers.
******************************************************************************/
void SceneRenderer::renderModifiers(bool renderOverlay)
{
	// Visit all objects in the scene.
	renderDataset()->sceneRoot()->visitObjectNodes([this, renderOverlay](PipelineSceneNode* pipeline) -> bool {
		renderModifiers(pipeline, renderOverlay);
		return true;
	});
}

/******************************************************************************
* Renders the visual representation of the modifiers in a pipeline.
******************************************************************************/
void SceneRenderer::renderModifiers(PipelineSceneNode* pipeline, bool renderOverlay)
{
	ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(pipeline->dataProvider());
	while(modApp) {
		Modifier* mod = modApp->modifier();

		// Setup local transformation.
		TimeInterval interval;
		setWorldTransform(pipeline->getWorldTransform(time(), interval));

		// Render modifier.
		mod->renderModifierVisual(time(), pipeline, modApp, this, renderOverlay);

		// Traverse up the pipeline.
		modApp = dynamic_object_cast<ModifierApplication>(modApp->input());
	}
}

/******************************************************************************
* Computes the world-space radius of an object located at the given world-space position,
* which should appear exactly one pixel wide in the rendered image.
******************************************************************************/
FloatType SceneRenderer::projectedPixelSize(const Point3& worldPosition) const
{
	// Get window size in device-independent pixels.
	int height = outputSize().height();
	if(height == 0) return 0;

	// The projected size in pixels:
	const FloatType baseSize = 1.0 * devicePixelRatio();

	if(projParams().isPerspective) {

		Point3 p = projParams().viewMatrix * worldPosition;
		if(p.z() == 0) return 1;

        Point3 p1 = projParams().projectionMatrix * p;
		Point3 p2 = projParams().projectionMatrix * (p + Vector3(1,0,0));

		return baseSize / (p1 - p2).length() / (FloatType)height;
	}
	else {
		return projParams().fieldOfView / (FloatType)height * baseSize;
	}
}

/******************************************************************************
* Sets the destination rectangle for rendering the image in viewport coordinates.
******************************************************************************/
void ImagePrimitive::setRectViewport(const SceneRenderer* renderer, const Box2& rect)
{ 
	OVITO_ASSERT(!rect.isEmpty());
	QSize windowSize = renderer->outputSize();
	Point2 minc((rect.minc.x() + 1.0) * windowSize.width() / 2.0, (-rect.maxc.y() + 1.0) * windowSize.height() / 2.0);
	Point2 maxc((rect.maxc.x() + 1.0) * windowSize.width() / 2.0, (-rect.minc.y() + 1.0) * windowSize.height() / 2.0);
	setRectWindow(Box2(minc, maxc));
}

/******************************************************************************
* Sets the destination rectangle for rendering the image in viewport coordinates.
******************************************************************************/
void TextPrimitive::setPositionViewport(const SceneRenderer* renderer, const Point2& pos)
{ 
	QSize windowSize = renderer->outputSize();
	Point2 pwin((pos.x() + 1.0) * windowSize.width() / 2.0, (-pos.y() + 1.0) * windowSize.height() / 2.0);
	setPositionWindow(pwin);
}

/******************************************************************************
* Indicates whether the mesh is fully opaque (no semi-transparent colors).
******************************************************************************/
bool MeshPrimitive::isFullyOpaque() const
{ 
	if(_isMeshFullyOpaque == boost::none) {
		if(_perInstanceColors)
			_isMeshFullyOpaque = boost::algorithm::none_of(ConstDataBufferAccess<ColorA>(_perInstanceColors), [](const ColorA& c) { return c.a() != FloatType(1); });		
		else if(mesh().hasVertexColors())
			_isMeshFullyOpaque = (uniformColor().a() >= FloatType(1)) && boost::algorithm::none_of(mesh().vertexColors(), [](const ColorA& c) { return c.a() != FloatType(1); });
		else if(mesh().hasFaceColors())
			_isMeshFullyOpaque = (uniformColor().a() >= FloatType(1)) && boost::algorithm::none_of(mesh().faceColors(), [](const ColorA& c) { return c.a() != FloatType(1); });
		else if(!materialColors().empty())
			_isMeshFullyOpaque = boost::algorithm::none_of(materialColors(), [](const ColorA& c) { return c.a() != FloatType(1); });
		else
			_isMeshFullyOpaque = (uniformColor().a() >= FloatType(1));
	}
	return *_isMeshFullyOpaque; 
}

}	// End of namespace
