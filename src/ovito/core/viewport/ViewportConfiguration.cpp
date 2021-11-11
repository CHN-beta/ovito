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
#include <ovito/core/viewport/ViewportConfiguration.h>
#include <ovito/core/viewport/Viewport.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/dataset/scene/RootSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/app/Application.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ViewportConfiguration);
DEFINE_VECTOR_REFERENCE_FIELD(ViewportConfiguration, viewports);
DEFINE_REFERENCE_FIELD(ViewportConfiguration, activeViewport);
DEFINE_REFERENCE_FIELD(ViewportConfiguration, maximizedViewport);
DEFINE_PROPERTY_FIELD(ViewportConfiguration, orbitCenterMode);
DEFINE_PROPERTY_FIELD(ViewportConfiguration, userOrbitCenter);
DEFINE_REFERENCE_FIELD(ViewportConfiguration, layoutRootCell);

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportSuspender::ViewportSuspender(RefMaker* object) noexcept : ViewportSuspender(object->dataset()->viewportConfig())
{
}

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportConfiguration::ViewportConfiguration(DataSet* dataset) : RefTarget(dataset),
	_orbitCenterMode(ORBIT_SELECTION_CENTER),
	_userOrbitCenter(Point3::Origin())
{
	// Repaint viewports when the camera orbit center changed.
	connect(this, &ViewportConfiguration::cameraOrbitCenterChanged, this, &ViewportConfiguration::updateViewports);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void ViewportConfiguration::referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(activeViewport)) {
		Q_EMIT activeViewportChanged(activeViewport());
	}
	else if(field == PROPERTY_FIELD(maximizedViewport)) {
		Q_EMIT maximizedViewportChanged(maximizedViewport());
	}
	else if(field == PROPERTY_FIELD(layoutRootCell)) {
		if(!isBeingLoaded() && !isAboutToBeDeleted())
			updateListOfViewports();
	}
	RefTarget::referenceReplaced(field, oldTarget, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool ViewportConfiguration::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged) {
		if(source == layoutRootCell() && !isBeingLoaded() && !isAboutToBeDeleted()) {
			updateListOfViewports();
			Q_EMIT viewportLayoutChanged();
		}
	}
	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ViewportConfiguration::propertyChanged(const PropertyFieldDescriptor* field)
{
	if(field == PROPERTY_FIELD(orbitCenterMode) || field == PROPERTY_FIELD(userOrbitCenter)) {
		Q_EMIT cameraOrbitCenterChanged();
	}
	RefTarget::propertyChanged(field);
}

/******************************************************************************
* This flags all viewports for redrawing.
******************************************************************************/
void ViewportConfiguration::updateViewports()
{
	// Check if viewport updates are suppressed.
	if(_viewportSuspendCount > 0) {
		_viewportsNeedUpdate = true;
		return;
	}
	_viewportsNeedUpdate = false;

	for(Viewport* vp : viewports())
		vp->updateViewport();
}

/******************************************************************************
* This immediately redraws the viewports reflecting all
* changes made to the scene.
******************************************************************************/
void ViewportConfiguration::processViewportUpdates()
{
	if(isSuspended())
		return;

	for(Viewport* vp : viewports())
		vp->processUpdateRequest();
}

/******************************************************************************
* Return true if there is currently a rendering operation going on.
* No new windows or dialogs should be shown during this phase
* to prevent an infinite update loop.
******************************************************************************/
bool ViewportConfiguration::isRendering() const
{
	// Check if any of the viewport windows is rendering.
	for(Viewport* vp : viewports())
		if(vp->isRendering()) return true;

	return false;
}

/******************************************************************************
* This will resume redrawing of the viewports after a call to suspendViewportUpdates().
******************************************************************************/
void ViewportConfiguration::resumeViewportUpdates()
{
	OVITO_ASSERT(_viewportSuspendCount > 0);
	_viewportSuspendCount--;
	if(_viewportSuspendCount == 0) {
		Q_EMIT viewportUpdateResumed();
		if(_viewportsNeedUpdate)
			updateViewports();
	}
}

/******************************************************************************
* Returns the world space point around which the viewport camera orbits.
******************************************************************************/
Point3 ViewportConfiguration::orbitCenter(Viewport* vp)
{
	// Update orbiting center.
	if(orbitCenterMode() == ORBIT_SELECTION_CENTER) {
		TimePoint time = dataset()->animationSettings()->time();
		Box3 selectionBoundingBox;
		for(SceneNode* node : dataset()->selection()->nodes()) {
			selectionBoundingBox.addBox(node->worldBoundingBox(time, vp));
		}
		if(!selectionBoundingBox.isEmpty())
			return selectionBoundingBox.center();
		else {
			Box3 sceneBoundingBox = dataset()->sceneRoot()->worldBoundingBox(time, vp);
			if(!sceneBoundingBox.isEmpty())
				return sceneBoundingBox.center();
		}
	}
	else if(orbitCenterMode() == ORBIT_USER_DEFINED) {
		return _userOrbitCenter;
	}
	return Point3::Origin();
}

/******************************************************************************
* Zooms all viewports to the extents of the currently selected nodes.
******************************************************************************/
void ViewportConfiguration::zoomToSelectionExtents() 
{
	for(Viewport* vp : viewports())
		vp->zoomToSelectionExtents();
}

/******************************************************************************
* Zooms to the extents of the scene.
******************************************************************************/
void ViewportConfiguration::zoomToSceneExtents() 
{
	for(Viewport* vp : viewports())
		vp->zoomToSceneExtents();
}

/******************************************************************************
* Helper function for recursively gathering all viewports in a layout tree.
******************************************************************************/
static void gatherViewportsFromLayout(const ViewportLayoutCell* cell, std::vector<Viewport*>& viewportList) 
{
	if(cell) {
		if(cell->viewport())
			viewportList.push_back(cell->viewport());
		for(const ViewportLayoutCell* child : cell->children())
			gatherViewportsFromLayout(child, viewportList);
	}
}

/******************************************************************************
* Rebuilds the linear list of all viewports that are part of the current viewport layout tree. 
******************************************************************************/
void ViewportConfiguration::updateListOfViewports()
{
	std::vector<Viewport*> viewportList;
	gatherViewportsFromLayout(layoutRootCell(), viewportList);
	_viewports.setTargets(this, PROPERTY_FIELD(viewports), std::move(viewportList));
}

/******************************************************************************
* This method is called once for this object after it has been completely
* loaded from a stream.
******************************************************************************/
void ViewportConfiguration::loadFromStreamComplete(ObjectLoadStream& stream)
{
	RefTarget::loadFromStreamComplete(stream);

	// For backward compatibility with OVITO 3.5.4:
	// Create a standard viewport layout for the linear list of viewports loaded from the old session state.
	if(!layoutRootCell()) {
		OVITO_ASSERT(viewports().size() == 4);

		ExecutionContext executionContext = Application::instance()->executionContext();
		OORef<ViewportLayoutCell> rootCell = OORef<ViewportLayoutCell>::create(dataset(), executionContext);
		rootCell->setSplitDirection(ViewportLayoutCell::Horizontal);
		rootCell->addChild(OORef<ViewportLayoutCell>::create(dataset(), executionContext));
		rootCell->addChild(OORef<ViewportLayoutCell>::create(dataset(), executionContext));

		rootCell->children()[0]->setSplitDirection(ViewportLayoutCell::Vertical);
		rootCell->children()[0]->addChild(OORef<ViewportLayoutCell>::create(dataset(), executionContext));
		rootCell->children()[0]->addChild(OORef<ViewportLayoutCell>::create(dataset(), executionContext));
		rootCell->children()[0]->children()[0]->setViewport(viewports().size() > 0 ? viewports()[0] : nullptr); // Upper left
		rootCell->children()[0]->children()[1]->setViewport(viewports().size() > 2 ? viewports()[2] : nullptr); // Lower left

		rootCell->children()[1]->setSplitDirection(ViewportLayoutCell::Vertical);
		rootCell->children()[1]->addChild(OORef<ViewportLayoutCell>::create(dataset(), executionContext));
		rootCell->children()[1]->addChild(OORef<ViewportLayoutCell>::create(dataset(), executionContext));
		rootCell->children()[1]->children()[0]->setViewport(viewports().size() > 1 ? viewports()[1] : nullptr); // Upper right
		rootCell->children()[1]->children()[1]->setViewport(viewports().size() > 3 ? viewports()[3] : nullptr); // Lower right
		setLayoutRootCell(std::move(rootCell));
	}
}

}	// End of namespace
