////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/ViewportSettings.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/rendering/RenderSettings.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/data/camera/AbstractCameraObject.h>
#include <ovito/core/dataset/DataSetContainer.h>

/// The default field of view in world units used for orthogonal view types when the scene is empty.
#define DEFAULT_ORTHOGONAL_FIELD_OF_VIEW		FloatType(200)

/// The default field of view angle in radians used for perspective view types when the scene is empty.
#define DEFAULT_PERSPECTIVE_FIELD_OF_VIEW		FloatType(35*FLOATTYPE_PI/180)

/// Controls the margin size between the overlay render frame and the viewport border.
#define VIEWPORT_RENDER_FRAME_SIZE				FloatType(0.93)

namespace Ovito {

IMPLEMENT_OVITO_CLASS(Viewport);
DEFINE_PROPERTY_FIELD(Viewport, viewType);
DEFINE_PROPERTY_FIELD(Viewport, gridMatrix);
DEFINE_PROPERTY_FIELD(Viewport, fieldOfView);
DEFINE_PROPERTY_FIELD(Viewport, cameraTransformation);
DEFINE_PROPERTY_FIELD(Viewport, cameraUpDirection);
DEFINE_PROPERTY_FIELD(Viewport, renderPreviewMode);
DEFINE_PROPERTY_FIELD(Viewport, isGridVisible);
DEFINE_PROPERTY_FIELD(Viewport, stereoscopicMode);
DEFINE_PROPERTY_FIELD(Viewport, viewportTitle);
DEFINE_REFERENCE_FIELD(Viewport, viewNode);
DEFINE_REFERENCE_FIELD(Viewport, overlays);
DEFINE_REFERENCE_FIELD(Viewport, underlays);
SET_PROPERTY_FIELD_CHANGE_EVENT(Viewport, viewportTitle, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
Viewport::Viewport(DataSet* dataset) : RefTarget(dataset),
		_viewType(VIEW_NONE),
		_fieldOfView(100),
		_renderPreviewMode(false),
		_cameraTransformation(AffineTransformation::Identity()),
		_cameraUpDirection(Vector3::Zero()),
		_gridMatrix(AffineTransformation::Identity()),
		_isGridVisible(false),
		_stereoscopicMode(false)
{
	connect(&ViewportSettings::getSettings(), &ViewportSettings::settingsChanged, this, &Viewport::viewportSettingsChanged);
}

/******************************************************************************
* Destructor.
******************************************************************************/
Viewport::~Viewport()
{
	// Also destroy the associated GUI window of this viewport when the viewport is deleted.
	if(_window)
		_window->destroyViewportWindow();
}

/******************************************************************************
* Changes the view type.
******************************************************************************/
void Viewport::setViewType(ViewType type, bool keepCurrentView)
{
	if(type == viewType())
		return;

	// Reset camera node.
	if(type != VIEW_SCENENODE)
		setViewNode(nullptr);

	// Setup default view.
	Matrix3 coordSys = ViewportSettings::getSettings().coordinateSystemOrientation();
	switch(type) {
		case VIEW_TOP:
			setCameraTransformation(AffineTransformation(coordSys));
			setGridMatrix(cameraTransformation());
			break;
		case VIEW_BOTTOM:
			setCameraTransformation(AffineTransformation(coordSys * Matrix3(1,0,0, 0,-1,0, 0,0,-1)));
			setGridMatrix(cameraTransformation());
			break;
		case VIEW_LEFT:
			setCameraTransformation(AffineTransformation(coordSys * Matrix3(0,0,-1, -1,0,0, 0,1,0)));
			setGridMatrix(cameraTransformation());
			break;
		case VIEW_RIGHT:
			setCameraTransformation(AffineTransformation(coordSys * Matrix3(0,0,1, 1,0,0, 0,1,0)));
			setGridMatrix(cameraTransformation());
			break;
		case VIEW_FRONT:
			setCameraTransformation(AffineTransformation(coordSys * Matrix3(1,0,0, 0,0,-1, 0,1,0)));
			setGridMatrix(cameraTransformation());
			break;
		case VIEW_BACK:
			setCameraTransformation(AffineTransformation(coordSys * Matrix3(-1,0,0, 0,0,1, 0,1,0)));
			setGridMatrix(cameraTransformation());
			break;
		case VIEW_ORTHO:
			if(!keepCurrentView) {
				setCameraPosition(Point3::Origin());
				if(viewType() == VIEW_NONE)
					setCameraTransformation(AffineTransformation(coordSys));
			}
			setGridMatrix(AffineTransformation(coordSys));
			break;
		case VIEW_PERSPECTIVE:
			if(!keepCurrentView) {
				if(viewType() >= VIEW_TOP && viewType() <= VIEW_ORTHO) {
					setCameraPosition(cameraPosition() - (cameraDirection().normalized() * fieldOfView()));
				}
				else if(viewType() != VIEW_PERSPECTIVE) {
					setCameraPosition(ViewportSettings::getSettings().coordinateSystemOrientation() * Point3(0,0,-50));
					setCameraDirection(ViewportSettings::getSettings().coordinateSystemOrientation() * Vector3(0,0,1));
				}
			}
			setGridMatrix(AffineTransformation(coordSys));
			break;
		case VIEW_SCENENODE:
			setGridMatrix(AffineTransformation(coordSys));
			break;
		case VIEW_NONE:
			setGridMatrix(AffineTransformation(coordSys));
			break;
	}

	if(!keepCurrentView) {
		// Setup default zoom.
		if(type == VIEW_PERSPECTIVE) {
			if(viewType() != VIEW_PERSPECTIVE)
				setFieldOfView(DEFAULT_PERSPECTIVE_FIELD_OF_VIEW);
		}
		else {
			if(viewType() == VIEW_PERSPECTIVE || viewType() == VIEW_NONE)
				setFieldOfView(DEFAULT_ORTHOGONAL_FIELD_OF_VIEW);
		}
	}

	_viewType.set(this, PROPERTY_FIELD(viewType), type);
}

/******************************************************************************
* Returns true if the viewport is using a perspective project;
* returns false if it is using an orthogonal projection.
******************************************************************************/
bool Viewport::isPerspectiveProjection() const
{
	if(viewType() <= VIEW_ORTHO)
		return false;
	else if(viewType() == VIEW_PERSPECTIVE)
		return true;
	else
		return _projParams.isPerspective;
}

/******************************************************************************
* Changes the viewing direction of the camera.
******************************************************************************/
void Viewport::setCameraDirection(const Vector3& newDir)
{
	if(newDir != Vector3::Zero()) {
		Vector3 upVector = cameraUpDirection();
		if(upVector.isZero()) {
			upVector = ViewportSettings::getSettings().upVector();
		}
		setCameraTransformation(AffineTransformation::lookAlong(cameraPosition(), newDir, upVector).inverse());
	}
}

/******************************************************************************
* Computes the projection matrix and other parameters.
******************************************************************************/
ViewProjectionParameters Viewport::computeProjectionParameters(TimePoint time, FloatType aspectRatio, const Box3& sceneBoundingBox)
{
	OVITO_ASSERT(aspectRatio > FLOATTYPE_EPSILON);

	ViewProjectionParameters params;
	params.aspectRatio = aspectRatio;
	params.validityInterval.setInfinite();
	if(!sceneBoundingBox.isEmpty())
		params.boundingBox = sceneBoundingBox;
	else
		params.boundingBox = Box3(Point3::Origin(), 1);

	// Get transformation from view scene node.
	if(viewType() == VIEW_SCENENODE && viewNode()) {
		// Get camera transformation.
		params.inverseViewMatrix = viewNode()->getWorldTransform(time, params.validityInterval);
		params.viewMatrix = params.inverseViewMatrix.inverse();

		// Get camera settings (FOV etc.)
		const PipelineFlowState& state = viewNode()->evaluatePipelineSynchronous(false);
		if(const AbstractCameraObject* camera = state.data() ? state.data()->getObject<AbstractCameraObject>() : nullptr) {
			// Get remaining parameters from camera object.
			camera->projectionParameters(time, params);
		}
		else {
			params.fieldOfView = 1;
			params.isPerspective = false;
		}
	}
	else {
		params.inverseViewMatrix = cameraTransformation();
		params.viewMatrix = params.inverseViewMatrix.inverse();
		params.fieldOfView = fieldOfView();
		params.isPerspective = (viewType() == VIEW_PERSPECTIVE);
	}

	// Transform scene bounding box to camera space.
	Box3 bb = params.boundingBox.transformed(params.viewMatrix).centerScale(FloatType(1.01));

	// Compute projection matrix.
	if(params.isPerspective) {
		if(bb.minc.z() < 0) {
			params.zfar = -bb.minc.z();
			params.znear = std::max(-bb.maxc.z(), params.zfar * FloatType(1e-4));
		}
		else {
			params.zfar = std::max(params.boundingBox.size().length(), FloatType(1));
			params.znear = params.zfar * FloatType(1e-4);
		}
		params.zfar = std::max(params.zfar, params.znear * FloatType(1.01));
		params.projectionMatrix = Matrix4::perspective(params.fieldOfView, FloatType(1) / params.aspectRatio, params.znear, params.zfar);
	}
	else {
		if(!bb.isEmpty()) {
			params.znear = -bb.maxc.z();
			params.zfar  = -bb.minc.z();
			if(params.zfar <= params.znear)
				params.zfar  = params.znear + FloatType(1);
		}
		else {
			params.znear = 1;
			params.zfar = 100;
		}
		params.projectionMatrix = Matrix4::ortho(-params.fieldOfView / params.aspectRatio, params.fieldOfView / params.aspectRatio,
							-params.fieldOfView, params.fieldOfView,
							params.znear, params.zfar);
	}
	params.inverseProjectionMatrix = params.projectionMatrix.inverse();

	return params;
}

/******************************************************************************
* Zooms to the extents of the scene.
******************************************************************************/
void Viewport::zoomToSceneExtents(FloatType viewportAspectRatio)
{
	Box3 sceneBoundingBox = dataset()->sceneRoot()->worldBoundingBox(dataset()->animationSettings()->time());
	zoomToBox(sceneBoundingBox, viewportAspectRatio);
}

/******************************************************************************
* Zooms to the extents of the currently selected nodes.
******************************************************************************/
void Viewport::zoomToSelectionExtents(FloatType viewportAspectRatio)
{
	Box3 selectionBoundingBox;
	for(SceneNode* node : dataset()->selection()->nodes()) {
		selectionBoundingBox.addBox(node->worldBoundingBox(dataset()->animationSettings()->time()));
	}
	if(!selectionBoundingBox.isEmpty())
		zoomToBox(selectionBoundingBox, viewportAspectRatio);
	else
		zoomToSceneExtents(viewportAspectRatio);
}

/******************************************************************************
* Zooms to the extents of the given bounding box.
******************************************************************************/
void Viewport::zoomToBox(const Box3& box, FloatType viewportAspectRatio)
{
	if(box.isEmpty())
		return;

	if(viewType() == VIEW_SCENENODE)
		return;	// Do not reposition the camera object.

	if(isPerspectiveProjection()) {
		FloatType dist = box.size().length() * FloatType(0.5) / tan(fieldOfView() * FloatType(0.5));
		setCameraPosition(box.center() - cameraDirection().resized(dist));
	}
	else {
		// Set up projection.
		FloatType aspectRatio = viewportAspectRatio;
		if(aspectRatio == 0) {
			QSize vpSize = windowSize();
			aspectRatio = (vpSize.width() > 0) ? ((FloatType)vpSize.height() / vpSize.width()) : FloatType(1);
			if(renderPreviewMode()) {
				if(RenderSettings* renderSettings = dataset()->renderSettings())
					aspectRatio = renderSettings->outputImageAspectRatio();
			}
		}
		if(aspectRatio == 0)
			return;
		ViewProjectionParameters projParams = computeProjectionParameters(dataset()->animationSettings()->time(), aspectRatio, box);

		FloatType minX = FLOATTYPE_MAX, minY = FLOATTYPE_MAX;
		FloatType maxX = FLOATTYPE_MIN, maxY = FLOATTYPE_MIN;
		for(int i = 0; i < 8; i++) {
			Point3 trans = projParams.viewMatrix * box[i];
			if(trans.x() < minX) minX = trans.x();
			if(trans.x() > maxX) maxX = trans.x();
			if(trans.y() < minY) minY = trans.y();
			if(trans.y() > maxY) maxY = trans.y();
		}
		FloatType w = std::max(maxX - minX, FloatType(1e-12));
		FloatType h = std::max(maxY - minY, FloatType(1e-12));
		if(aspectRatio > h/w)
			setFieldOfView(w * aspectRatio * FloatType(0.55));
		else
			setFieldOfView(h * FloatType(0.55));
		setCameraPosition(box.center());
	}
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool Viewport::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged) {
		if(source == viewNode()) {
			// Update viewport when camera node has moved.
			updateViewport();
		}
		else if(_overlays.contains(source) || _underlays.contains(source)) {
			// Update viewport when one of the layers has changed.
			updateViewport();
		}
	}
	else if(source == viewNode() && event.type() == ReferenceEvent::TitleChanged) {
		// Update viewport title when camera node has been renamed.
		updateViewportTitle();
		updateViewport();
	}
	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void Viewport::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(viewNode)) {
		if(viewType() == VIEW_SCENENODE && newTarget == nullptr) {
			// If the camera node has been deleted, switch to Orthographic or Perspective view type.
			// Keep current camera orientation.
			setFieldOfView(projectionParams().fieldOfView);
			setCameraTransformation(projectionParams().inverseViewMatrix);
			setViewType(isPerspectiveProjection() ? VIEW_PERSPECTIVE : VIEW_ORTHO, true);
		}
		else if(viewType() != VIEW_SCENENODE && newTarget != nullptr) {
			setViewType(VIEW_SCENENODE);
		}

		// Update viewport when the camera has been replaced by another scene node.
		updateViewportTitle();
	}
	else if(field == PROPERTY_FIELD(overlays) || field == PROPERTY_FIELD(underlays)) {
		updateViewport();
	}
	RefTarget::referenceReplaced(field, oldTarget, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField.
******************************************************************************/
void Viewport::referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(overlays) || field == PROPERTY_FIELD(underlays)) {
		updateViewport();
	}
	RefTarget::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been removed from a VectorReferenceField.
******************************************************************************/
void Viewport::referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(overlays) || field == PROPERTY_FIELD(underlays)) {
		updateViewport();
	}
	RefTarget::referenceRemoved(field, oldTarget, listIndex);
}

/******************************************************************************
* Is called when the value of a property field of this object has changed.
******************************************************************************/
void Viewport::propertyChanged(const PropertyFieldDescriptor& field)
{
	RefTarget::propertyChanged(field);
	if(field == PROPERTY_FIELD(viewType)) {
		updateViewportTitle();
	}
	else if(field == PROPERTY_FIELD(cameraUpDirection) && !isBeingLoaded()) {
		// Update view matrix when the up-vector has been changed.
		setCameraDirection(cameraDirection());
	}
	else if(field == PROPERTY_FIELD(isGridVisible) || field == PROPERTY_FIELD(renderPreviewMode) || field == PROPERTY_FIELD(stereoscopicMode)) {
		Q_EMIT viewportChanged();
	}
	updateViewport();
}

/******************************************************************************
* This is called when the global viewport settings have changed.
******************************************************************************/
void Viewport::viewportSettingsChanged(ViewportSettings* newSettings)
{
	// Update camera TM if "up" axis has changed to make it point upward.
	if(ViewportSettings::getSettings().constrainCameraRotation())
		setCameraDirection(cameraDirection());

	// Redraw viewport.
	updateViewport();
}

/******************************************************************************
* Updates the title text of the viewport based on the current view type.
******************************************************************************/
void Viewport::updateViewportTitle()
{
	// Load viewport caption string.
	QString newTitle;
	switch(viewType()) {
		case VIEW_TOP: newTitle = tr("Top"); break;
		case VIEW_BOTTOM: newTitle = tr("Bottom"); break;
		case VIEW_FRONT: newTitle = tr("Front"); break;
		case VIEW_BACK: newTitle = tr("Back"); break;
		case VIEW_LEFT: newTitle = tr("Left"); break;
		case VIEW_RIGHT: newTitle = tr("Right"); break;
		case VIEW_ORTHO: newTitle = tr("Ortho"); break;
		case VIEW_PERSPECTIVE: newTitle = tr("Perspective"); break;
		case VIEW_SCENENODE: newTitle = viewNode() ? viewNode()->nodeName() : tr("No view node"); break;
		default: OVITO_ASSERT(false); // unknown viewport type
	}
	_viewportTitle.set(this, PROPERTY_FIELD(viewportTitle), std::move(newTitle));
	Q_EMIT viewportChanged();
}

/******************************************************************************
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void Viewport::updateViewport()
{
	if(_window)
		_window->renderLater();
}

/******************************************************************************
* Immediately redraws the contents of this viewport.
******************************************************************************/
void Viewport::redrawViewport()
{
	if(_window)
		_window->renderNow();
}

/******************************************************************************
* If an update request is pending for this viewport, immediately processes it
* and redraw the viewport.
******************************************************************************/
void Viewport::processUpdateRequest()
{
	if(_window)
		_window->processViewportUpdate();
}

/******************************************************************************
* Renders the contents of the interactive viewport in a window.
******************************************************************************/
void Viewport::renderInteractive(SceneRenderer* renderer)
{
	OVITO_ASSERT_MSG(!isRendering(), "Viewport::renderInteractive()", "Viewport is already rendering.");
	OVITO_ASSERT_MSG(!dataset()->viewportConfig()->isRendering(), "Viewport::renderInteractive()", "Some other viewport is already rendering.");
	OVITO_ASSERT(!dataset()->viewportConfig()->isSuspended());

	QSize vpSize = windowSize();
	if(vpSize.height() <= 0 || vpSize.width() <= 0)
		return;

	try {
		_isRendering = true;
		TimePoint time = dataset()->animationSettings()->time();
		RenderSettings* renderSettings = dataset()->renderSettings();
		OVITO_ASSERT(renderSettings != nullptr);

		// Set up the renderer.
		renderer->startRender(dataset(), renderSettings);

		// Set up preliminary projection without a known bounding box.
		FloatType aspectRatio = (FloatType)vpSize.height() / vpSize.width();
		_projParams = computeProjectionParameters(time, aspectRatio);

		// Adjust projection if render frame is shown.
		if(renderPreviewMode())
			adjustProjectionForRenderFrame(_projParams);

		// This is the async operation object used when calling rendering functions in the following.
		SynchronousOperation renderOperation = SynchronousOperation::createSignal(dataset()->taskManager());

		// Determine scene bounding box.
		Box3 boundingBox = renderer->computeSceneBoundingBox(time, _projParams, this, renderOperation.subOperation());

		// Set up final projection with the now known bounding box.
		_projParams = computeProjectionParameters(time, aspectRatio, boundingBox);

		// Adjust projection if render frame is shown.
		if(renderPreviewMode())
			adjustProjectionForRenderFrame(_projParams);

		// Set up the viewport renderer.
		renderer->beginFrame(time, _projParams, this);

		// Render viewport "underlays".
		if(renderPreviewMode() && !renderer->isPicking()) {
			if(boost::algorithm::any_of(underlays(), [](ViewportOverlay* layer) { return layer->isEnabled(); })) {
				// Let layers paint into QImage buffer, which will then be copied into the OpenGL frame buffer.
				renderLayers(renderer, time, renderSettings, vpSize, boundingBox, underlays(), renderOperation);
			}
		}

		if(!_projParams.isPerspective || !stereoscopicMode() || renderer->isPicking()) {

			// Pass final projection parameters to renderer.
			renderer->setProjParams(_projParams);

			// Call the viewport renderer to render the scene objects.
			renderer->renderFrame(nullptr, SceneRenderer::NonStereoscopic, renderOperation.subOperation());
		}
		else {

			// Stereoscopic parameters
			FloatType eyeSeparation = FloatType(16);
			FloatType convergence = (orbitCenter() - Point3::Origin() - _projParams.inverseViewMatrix.translation()).length();
			convergence = std::max(convergence, _projParams.znear);
			ViewProjectionParameters params = _projParams;

			// Setup projection of left eye.
			FloatType top = params.znear * tan(params.fieldOfView / 2);
			FloatType bottom = -top;
			FloatType a = tan(params.fieldOfView / 2) / params.aspectRatio * convergence;
			FloatType b = a - eyeSeparation / 2;
			FloatType c = a + eyeSeparation / 2;
			FloatType left = -b * params.znear / convergence;
			FloatType right = c * params.znear / convergence;
			params.projectionMatrix = Matrix4::frustum(left, right, bottom, top, params.znear, params.zfar);
			params.inverseProjectionMatrix = params.projectionMatrix.inverse();
			params.viewMatrix = AffineTransformation::translation(Vector3(eyeSeparation / 2, 0, 0)) * _projParams.viewMatrix;
			params.inverseViewMatrix = params.viewMatrix.inverse();
			renderer->setProjParams(params);

			// Render image of left eye.
			renderer->renderFrame(nullptr, SceneRenderer::StereoscopicLeft, renderOperation.subOperation());

			// Setup projection of right eye.
			left = -c * params.znear / convergence;
			right = b * params.znear / convergence;
			params.projectionMatrix = Matrix4::frustum(left, right, bottom, top, params.znear, params.zfar);
			params.inverseProjectionMatrix = params.projectionMatrix.inverse();
			params.viewMatrix = AffineTransformation::translation(Vector3(-eyeSeparation / 2, 0, 0)) * _projParams.viewMatrix;
			params.inverseViewMatrix = params.viewMatrix.inverse();
			renderer->setProjParams(params);

			// Render image of right eye.
			renderer->renderFrame(nullptr, SceneRenderer::StereoscopicRight, renderOperation.subOperation());
		}

		// Render viewport overlays.
		if(renderPreviewMode() && !renderer->isPicking()) {
			if(boost::algorithm::any_of(overlays(), [](ViewportOverlay* layer) { return layer->isEnabled(); })) {
				// Let overlays paint into QImage buffer, which will then be copied over the OpenGL frame buffer.
				renderLayers(renderer, time, renderSettings, vpSize, boundingBox, overlays(), renderOperation);
			}
		}

		// Let GUI window render its own graphics on top of the scene.
		if(!renderer->isPicking()) {
			window()->renderGui(renderer);
		}

		// Finish rendering.
		renderer->endFrame(true);
		renderer->endRender();

		// Discard unused vis element resources.
		dataset()->visCache().discardUnusedObjects();

		_isRendering = false;
	}
	catch(...) {
		_isRendering = false;
		throw;
	}
}

/******************************************************************************
* Renders the viewport layers to an image buffer.
******************************************************************************/
void Viewport::renderLayers(SceneRenderer* renderer, TimePoint time, RenderSettings* renderSettings, QSize vpSize, const Box3& boundingBox, const QVector<ViewportOverlay*>& layers, SynchronousOperation& operation)
{
	// Let layers paint into QImage buffer, which will then be copied over the OpenGL frame buffer.
	QImage paintBuffer(vpSize, QImage::Format_ARGB32_Premultiplied);
	paintBuffer.fill(0);
	Box2 renderFrameBox = renderFrameRect();
	QRect renderFrameRect(
			(renderFrameBox.minc.x() + 1) * paintBuffer.width() / 2,
			(renderFrameBox.minc.y() + 1) * paintBuffer.height() / 2,
			renderFrameBox.width() * paintBuffer.width() / 2,
			renderFrameBox.height() * paintBuffer.height() / 2);
	ViewProjectionParameters renderProjParams = computeProjectionParameters(time, renderSettings->outputImageAspectRatio(), boundingBox);
	for(ViewportOverlay* layer : layers) {
		if(layer->isEnabled()) {
			QPainter painter(&paintBuffer);
			painter.setWindow(QRect(0, 0, renderSettings->outputImageWidth(), renderSettings->outputImageHeight()));
			painter.setViewport(renderFrameRect);
			painter.setRenderHint(QPainter::Antialiasing);
			layer->renderInteractive(this, time, painter, renderProjParams, renderSettings, operation.subOperation());
		}
	}
	std::shared_ptr<ImagePrimitive> paintBufferPrim = renderer->createImagePrimitive();
	paintBufferPrim->setImage(paintBuffer);
	paintBufferPrim->renderViewport(renderer, Point2(-1,-1), Vector2(2, 2));
}

/******************************************************************************
* Modifies the projection such that the render frame painted over the 3d scene exactly
* matches the true visible area.
******************************************************************************/
void Viewport::adjustProjectionForRenderFrame(ViewProjectionParameters& params)
{
	QSize vpSize = windowSize();
	RenderSettings* renderSettings = dataset()->renderSettings();
	if(!renderSettings || vpSize.width() == 0 || vpSize.height() == 0)
		return;

	FloatType renderAspectRatio = renderSettings->outputImageAspectRatio();
	FloatType windowAspectRatio = (FloatType)vpSize.height() / (FloatType)vpSize.width();

	if(_projParams.isPerspective) {
		if(renderAspectRatio < windowAspectRatio)
			params.fieldOfView = atan(tan(params.fieldOfView/2) / (VIEWPORT_RENDER_FRAME_SIZE / windowAspectRatio * renderAspectRatio))*2;
		else
			params.fieldOfView = atan(tan(params.fieldOfView/2) / VIEWPORT_RENDER_FRAME_SIZE)*2;
		params.projectionMatrix = Matrix4::perspective(params.fieldOfView, FloatType(1) / params.aspectRatio, params.znear, params.zfar);
	}
	else {
		if(renderAspectRatio < windowAspectRatio)
			params.fieldOfView /= VIEWPORT_RENDER_FRAME_SIZE / windowAspectRatio * renderAspectRatio;
		else
			params.fieldOfView /= VIEWPORT_RENDER_FRAME_SIZE;
		params.projectionMatrix = Matrix4::ortho(-params.fieldOfView / params.aspectRatio, params.fieldOfView / params.aspectRatio,
							-params.fieldOfView, params.fieldOfView,
							params.znear, params.zfar);
	}
	params.inverseProjectionMatrix = params.projectionMatrix.inverse();
}

/******************************************************************************
* Returns the geometry of the render frame, i.e., the region of the viewport that
* will be visible in a rendered image.
* The returned box is given in viewport coordinates (interval [-1,+1]).
******************************************************************************/
Box2 Viewport::renderFrameRect() const
{
	QSize vpSize = windowSize();
	RenderSettings* renderSettings = dataset()->renderSettings();
	if(!renderSettings || vpSize.width() == 0 || vpSize.height() == 0)
		return { Point2(-1), Point2(+1) };

	// Compute a rectangle that has the same aspect ratio as the rendered image.
	FloatType renderAspectRatio = renderSettings->outputImageAspectRatio();
	FloatType windowAspectRatio = (FloatType)vpSize.height() / (FloatType)vpSize.width();
	FloatType frameWidth, frameHeight;
	if(renderAspectRatio < windowAspectRatio) {
		frameWidth = VIEWPORT_RENDER_FRAME_SIZE;
		frameHeight = frameWidth / windowAspectRatio * renderAspectRatio;
	}
	else {
		frameHeight = VIEWPORT_RENDER_FRAME_SIZE;
		frameWidth = frameHeight / renderAspectRatio * windowAspectRatio;
	}

	return Box2(-frameWidth, -frameHeight, frameWidth, frameHeight);
}

/******************************************************************************
* Computes the world size of an object that should appear always in the
* same size on the screen.
******************************************************************************/
FloatType Viewport::nonScalingSize(const Point3& worldPosition)
{
	if(!window()) return 1;

	// Get window size in device-independent pixels.
	int height = window()->viewportWindowDeviceIndependentSize().height();

	if(height == 0) return 1;

	const FloatType baseSize = 60;

	if(isPerspectiveProjection()) {

		Point3 p = projectionParams().viewMatrix * worldPosition;
		if(p.z() == 0) return FloatType(1);

        Point3 p1 = projectionParams().projectionMatrix * p;
		Point3 p2 = projectionParams().projectionMatrix * (p + Vector3(1,0,0));

		return FloatType(0.8) * baseSize / (p1 - p2).length() / (FloatType)height;
	}
	else {
		return _projParams.fieldOfView / (FloatType)height * baseSize;
	}
}

/******************************************************************************
* Computes a point in the given coordinate system based on the given screen
* position and the current snapping settings.
******************************************************************************/
bool Viewport::snapPoint(const QPointF& screenPoint, Point3& snapPoint, const AffineTransformation& snapSystem)
{
	// Compute the intersection point of the ray with the X-Y plane of the snapping coordinate system.
    Ray3 ray = snapSystem.inverse() * screenRay(screenPoint);

    Plane3 plane(Vector3(0,0,1), 0);
	FloatType t = plane.intersectionT(ray, FloatType(1e-3));
	if(t == FLOATTYPE_MAX) return false;
	if(isPerspectiveProjection() && t <= 0) return false;

	snapPoint = ray.point(t);
	snapPoint.z() = 0;

	return true;
}

/******************************************************************************
* Computes a ray in world space going through a pixel of the viewport window.
******************************************************************************/
Ray3 Viewport::screenRay(const QPointF& screenPoint)
{
	QSize vpSize = windowSize();
	return viewportRay(Point2(
			(FloatType)screenPoint.x() / vpSize.width() * FloatType(2) - FloatType(1),
			FloatType(1) - (FloatType)screenPoint.y() / vpSize.height() * FloatType(2)));
}

/******************************************************************************
* Computes a ray in world space going through a viewport pixel.
******************************************************************************/
Ray3 Viewport::viewportRay(const Point2& viewportPoint)
{
	if(projectionParams().isPerspective) {
		Point3 ndc1(viewportPoint.x(), viewportPoint.y(), 1);
		Point3 ndc2(viewportPoint.x(), viewportPoint.y(), 0);
		Point3 p1 = projectionParams().inverseViewMatrix * (projectionParams().inverseProjectionMatrix * ndc1);
		Point3 p2 = projectionParams().inverseViewMatrix * (projectionParams().inverseProjectionMatrix * ndc2);
		return { Point3::Origin() + _projParams.inverseViewMatrix.translation(), p1 - p2 };
	}
	else {
		Point3 ndc(viewportPoint.x(), viewportPoint.y(), -1);
		return { projectionParams().inverseViewMatrix * (projectionParams().inverseProjectionMatrix * ndc), projectionParams().inverseViewMatrix * Vector3(0,0,-1) };
	}
}

/******************************************************************************
* Computes the intersection of a ray going through a point in the
* viewport projection plane and the grid plane.
*
* Returns true if an intersection has been found.
******************************************************************************/
bool Viewport::computeConstructionPlaneIntersection(const Point2& viewportPosition, Point3& intersectionPoint, FloatType epsilon)
{
	// The construction plane in grid coordinates.
	Plane3 gridPlane = Plane3(Vector3(0,0,1), 0);

	// Compute the ray and transform it to the grid coordinate system.
	Ray3 ray = gridMatrix().inverse() * viewportRay(viewportPosition);

	// Compute intersection point.
	FloatType t = gridPlane.intersectionT(ray, epsilon);
    if(t == std::numeric_limits<FloatType>::max()) return false;
	if(isPerspectiveProjection() && t <= 0) return false;

	intersectionPoint = ray.point(t);
	intersectionPoint.z() = 0;

	return true;
}

/******************************************************************************
* Returns the current orbit center for this viewport.
******************************************************************************/
Point3 Viewport::orbitCenter()
{
	// Use the target of a camera as the orbit center.
	if(viewNode() && viewType() == Viewport::VIEW_SCENENODE && viewNode()->lookatTargetNode()) {
		TimeInterval iv;
		TimePoint time = dataset()->animationSettings()->time();
		return Point3::Origin() + viewNode()->lookatTargetNode()->getWorldTransform(time, iv).translation();
	}
	else {
		Point3 currentOrbitCenter = dataset()->viewportConfig()->orbitCenter();

		if(viewNode() && isPerspectiveProjection()) {
			// If a free camera node is selected, the current orbit center is at the same location as the camera.
			// In this case, we should shift the orbit center such that it is in front of the camera.
			Point3 camPos = Point3::Origin() + projectionParams().inverseViewMatrix.translation();
			if(currentOrbitCenter.equals(camPos))
				currentOrbitCenter = camPos - FloatType(50) * projectionParams().inverseViewMatrix.column(2);
		}
		return currentOrbitCenter;
	}
}

}	// End of namespace
