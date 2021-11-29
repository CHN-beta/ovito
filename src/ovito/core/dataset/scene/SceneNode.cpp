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
#include <ovito/core/dataset/scene/SceneNode.h>
#include <ovito/core/dataset/animation/controller/LookAtController.h>
#include <ovito/core/dataset/animation/controller/PRSTransformationController.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/UndoStack.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/scene/SelectionSet.h>
#include <ovito/core/oo/CloneHelper.h>
#include <ovito/core/app/Application.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(SceneNode);
DEFINE_REFERENCE_FIELD(SceneNode, transformationController);
DEFINE_REFERENCE_FIELD(SceneNode, lookatTargetNode);
DEFINE_VECTOR_REFERENCE_FIELD(SceneNode, children);
DEFINE_VECTOR_REFERENCE_FIELD(SceneNode, hiddenInViewports);
SET_PROPERTY_FIELD_LABEL(SceneNode, transformationController, "Transformation");
SET_PROPERTY_FIELD_LABEL(SceneNode, lookatTargetNode, "Target");
SET_PROPERTY_FIELD_LABEL(SceneNode, children, "Children");
SET_PROPERTY_FIELD_LABEL(SceneNode, nodeName, "Name");
SET_PROPERTY_FIELD_LABEL(SceneNode, displayColor, "Display color");
SET_PROPERTY_FIELD_CHANGE_EVENT(SceneNode, nodeName, ReferenceEvent::TitleChanged);

/******************************************************************************
* Default constructor.
******************************************************************************/
SceneNode::SceneNode(DataSet* dataset) : RefTarget(dataset),
	_worldTransform(AffineTransformation::Identity()),
	_worldTransformValidity(TimeInterval::empty()),
	_boundingBoxValidity(TimeInterval::empty()),
	_displayColor(0,0,0)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void SceneNode::initializeObject(ExecutionContext executionContext)
{
	// Assign random color to node.
	static std::default_random_engine rng;
	setDisplayColor(Color::fromHSV(std::uniform_real_distribution<FloatType>()(rng), 1, 1));

	// Create a transformation controller for the node.
	setTransformationController(ControllerManager::createTransformationController(dataset(), executionContext));

	RefTarget::initializeObject(executionContext);
}

/******************************************************************************
* Returns this node's world transformation matrix.
* This matrix contains the transformation of the parent node.
******************************************************************************/
const AffineTransformation& SceneNode::getWorldTransform(TimePoint time, TimeInterval& validityInterval) const
{
	if(!_worldTransformValidity.contains(time)) {
		_worldTransformValidity.setInfinite();
		_worldTransform.setIdentity();
		// Get parent node's tm.
		if(parentNode() && !parentNode()->isRootNode()) {
			_worldTransform = _worldTransform * parentNode()->getWorldTransform(time, _worldTransformValidity);
		}
		// Apply own tm.
		if(transformationController())
			transformationController()->applyTransformation(time, _worldTransform, _worldTransformValidity);
	}
	validityInterval.intersect(_worldTransformValidity);
	return _worldTransform;
}

/******************************************************************************
* Returns this node's local transformation matrix.
* This matrix  does not contain the ObjectTransform of this node and
* does not contain the transformation of the parent node.
******************************************************************************/
AffineTransformation SceneNode::getLocalTransform(TimePoint time, TimeInterval& validityInterval) const
{
	AffineTransformation result = AffineTransformation::Identity();
	if(transformationController())
		transformationController()->applyTransformation(time, result, validityInterval);
	return result;
}

/******************************************************************************
* This method marks the world transformation cache as invalid,
* so it will be rebuilt during the next call to GetWorldTransform().
******************************************************************************/
void SceneNode::invalidateWorldTransformation()
{
	_worldTransformValidity.setEmpty();
	invalidateBoundingBox();
	for(SceneNode* child : children())
		child->invalidateWorldTransformation();
	notifyDependents(ReferenceEvent::TransformationChanged);
}

/******************************************************************************
* Deletes this node from the scene. This will also delete all child nodes.
******************************************************************************/
void SceneNode::deleteNode()
{
	// Delete target too.
	OORef<SceneNode> tn = lookatTargetNode();
	if(tn) {
		// Clear reference first to prevent infinite recursion.
		_lookatTargetNode.set(this, PROPERTY_FIELD(lookatTargetNode), nullptr);
		tn->deleteNode();
	}

	// Delete all child nodes recursively.
	for(SceneNode* child : children())
		child->deleteNode();

	OVITO_ASSERT(children().empty());

	// Delete node itself.
	deleteReferenceObject();
}

/******************************************************************************
* Binds this scene node to a target node and creates a look at controller
* that lets this scene node look at the target. The target will automatically
* be deleted if this scene node is deleted and vice versa.
* Returns the newly created LookAtController assigned as rotation controller for this node.
******************************************************************************/
LookAtController* SceneNode::setLookatTargetNode(SceneNode* targetNode)
{
	_lookatTargetNode.set(this, PROPERTY_FIELD(lookatTargetNode), targetNode);

	// Let this node look at the target.
	PRSTransformationController* prs = dynamic_object_cast<PRSTransformationController>(transformationController());
	if(prs) {
		if(targetNode) {
			OVITO_CHECK_OBJECT_POINTER(targetNode);

			// Create a look at controller.
			OORef<LookAtController> lookAtCtrl = dynamic_object_cast<LookAtController>(prs->rotationController());
			if(!lookAtCtrl)
				lookAtCtrl = OORef<LookAtController>::create(dataset(), Application::instance()->executionContext());
			lookAtCtrl->setTargetNode(targetNode);

			// Assign it as rotation sub-controller.
			prs->setRotationController(std::move(lookAtCtrl));

			return dynamic_object_cast<LookAtController>(prs->rotationController());
		}
		else {
			// Save old rotation.
			TimePoint time = dataset()->animationSettings()->time();
			TimeInterval iv;
			Rotation rotation;
			prs->rotationController()->getRotationValue(time, rotation, iv);

			// Reset to default rotation controller.
			OORef<Controller> controller = ControllerManager::createRotationController(dataset(), Application::instance()->executionContext());
			controller->setRotationValue(time, rotation, true);
			prs->setRotationController(std::move(controller));
		}
	}

	return nullptr;
}

/******************************************************************************
* From RefMaker.
******************************************************************************/
bool SceneNode::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged) {
		if(source == transformationController()) {
			// TM has changed -> rebuild world tm cache.
			invalidateWorldTransformation();
		}
		else {
			// The bounding box might have changed if the object has changed.
			invalidateBoundingBox();
		}
	}
	else if(event.type() == ReferenceEvent::TargetDeleted && source == lookatTargetNode()) {
		// Lookat target node has been deleted -> delete this node too.
		if(!dataset()->undoStack().isUndoingOrRedoing())
			deleteNode();
	}
	else if(event.type() == ReferenceEvent::AnimationFramesChanged && children().contains(static_cast<SceneNode*>(source))) {
		// Forward pipeline changed events from the scene pipelines.
		return true;
	}
	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* From RefMaker.
******************************************************************************/
void SceneNode::referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(transformationController)) {
		// TM controller has changed -> rebuild world tm cache.
		invalidateWorldTransformation();
	}
	else if(field == PROPERTY_FIELD(children)) {
		// A child node has been replaced.
		SceneNode* oldChild = static_object_cast<SceneNode>(oldTarget);
		OVITO_ASSERT(oldChild->parentNode() == this);
		oldChild->_parentNode = nullptr;

		SceneNode* newChild = static_object_cast<SceneNode>(newTarget);
		OVITO_CHECK_OBJECT_POINTER(newChild);
		OVITO_ASSERT(newChild->parentNode() == nullptr);
		newChild->_parentNode = this;

		// Invalidate cached world bounding box of this parent node.
		invalidateBoundingBox();

		// The animation length might have changed when an object has been removed from the scene.
		notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}
	RefTarget::referenceReplaced(field, oldTarget, newTarget, listIndex);
}

/******************************************************************************
* From RefMaker.
******************************************************************************/
void SceneNode::referenceInserted(const PropertyFieldDescriptor* field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(children)) {
		// A new child node has been added.
		SceneNode* child = static_object_cast<SceneNode>(newTarget);
		OVITO_CHECK_OBJECT_POINTER(child);
		OVITO_ASSERT(child->parentNode() == nullptr);
		child->_parentNode = this;

		// Invalidate cached world bounding box of this parent node.
		invalidateBoundingBox();

		// The animation length might have changed when an object has been removed from the scene.
		if(!isBeingLoaded())
			notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}
	RefTarget::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* From RefMaker.
******************************************************************************/
void SceneNode::referenceRemoved(const PropertyFieldDescriptor* field, RefTarget* oldTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(children)) {
		// A child node has been removed.
		SceneNode* child = static_object_cast<SceneNode>(oldTarget);
		OVITO_ASSERT(child->parentNode() == this);
		child->_parentNode = nullptr;

		if(!isAboutToBeDeleted()) {
			// Invalidate cached world bounding box of this parent node.
			invalidateBoundingBox();

			// The animation length might have changed when an object has been removed from the scene.
			notifyDependents(ReferenceEvent::AnimationFramesChanged);
		}
	}
	RefTarget::referenceRemoved(field, oldTarget, listIndex);
}

/******************************************************************************
* This method marks the cached bounding box as invalid,
* so it will be rebuilt during the next call to worldBoundingBox().
******************************************************************************/
void SceneNode::invalidateBoundingBox()
{
	_boundingBoxValidity.setEmpty();
	if(parentNode())
		parentNode()->invalidateBoundingBox();
}

/******************************************************************************
* Adds a child scene node to this node.
******************************************************************************/
void SceneNode::insertChildNode(int index, SceneNode* newChild)
{
	OVITO_CHECK_OBJECT_POINTER(newChild);

	// Check whether it is already a child of this parent.
	if(newChild->parentNode() == this) {
		OVITO_ASSERT(children().contains(newChild));
		return;
	}

	// Remove new child from old parent node first.
	if(newChild->parentNode()) {
		auto oldIndex = newChild->parentNode()->children().indexOf(newChild);
		newChild->parentNode()->removeChildNode(oldIndex);
	}
	OVITO_ASSERT(newChild->parentNode() == nullptr);

	// Insert into children array of this parent.
	_children.insert(this, PROPERTY_FIELD(children), index, newChild);
	// This node should have been automatically set as the child's parent by referenceInserted().
	OVITO_ASSERT(newChild->parentNode() == this);

	// Adjust transformation to preserve world position.
	TimeInterval iv = TimeInterval::infinite();
	const AffineTransformation& newParentTM = getWorldTransform(dataset()->animationSettings()->time(), iv);
	if(newParentTM != AffineTransformation::Identity())
		newChild->transformationController()->changeParent(dataset()->animationSettings()->time(), AffineTransformation::Identity(), newParentTM, newChild);
	newChild->invalidateWorldTransformation();
}

/******************************************************************************
* Removes a child node from this parent node.
******************************************************************************/
void SceneNode::removeChildNode(int index)
{
	OVITO_ASSERT(index >= 0 && index < children().size());

	SceneNode* child = children()[index];
	OVITO_ASSERT_MSG(child->parentNode() == this, "SceneNode::removeChildNode()", "The node to be removed is not a child of this parent node.");

	// Remove child node from array.
	_children.remove(this, PROPERTY_FIELD(children), index);
	OVITO_ASSERT(children().contains(child) == false);
	OVITO_ASSERT(child->parentNode() == nullptr);

	// Update child node.
	TimeInterval iv = TimeInterval::infinite();
	AffineTransformation oldParentTM = getWorldTransform(dataset()->animationSettings()->time(), iv);
	if(oldParentTM != AffineTransformation::Identity())
		child->transformationController()->changeParent(dataset()->animationSettings()->time(), oldParentTM, AffineTransformation::Identity(), child);
	child->invalidateWorldTransformation();
}

/******************************************************************************
* Returns true if this node is currently selected.
******************************************************************************/
bool SceneNode::isSelected() const
{
	return dataset()->selection()->nodes().contains(const_cast<SceneNode*>(this));
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void SceneNode::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const
{
	RefTarget::saveToStream(stream, excludeRecomputableData);

	stream.beginChunk(0x02);
	// This is for future use...
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void SceneNode::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);

	stream.expectChunkRange(0x01, 0x02);
	// This is for future use...
	stream.closeChunk();

	// Restore parent/child hierarchy.
	for(SceneNode* child : children())
		child->_parentNode = this;
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> SceneNode::clone(bool deepCopy, CloneHelper& cloneHelper) const
{
	// Let the base class create an instance of this class.
	OORef<SceneNode> clone = static_object_cast<SceneNode>(RefTarget::clone(deepCopy, cloneHelper));

	// Clone orientation target node too.
	if(clone->lookatTargetNode()) {
		OVITO_ASSERT(lookatTargetNode());

		// Insert the cloned target into the same scene as out target.
		if(lookatTargetNode()->parentNode() && !clone->lookatTargetNode()->parentNode()) {
			lookatTargetNode()->parentNode()->addChildNode(clone->lookatTargetNode());
		}

		// Set new target for look-at controller.
		clone->setLookatTargetNode(clone->lookatTargetNode());
	}

	return clone;
}

/******************************************************************************
* Returns the bounding box of the scene node in world coordinates.
*    time - The time at which the bounding box should be returned.
******************************************************************************/
Box3 SceneNode::worldBoundingBox(TimePoint time, Viewport* vp) const
{
	if(vp && isHiddenInViewport(vp, true))
		return Box3();
    if(!_boundingBoxValidity.contains(time)) {
		_boundingBoxValidity.setInfinite();
		_localBoundingBox = localBoundingBox(time, _boundingBoxValidity);
	}
	TimeInterval iv;
	const AffineTransformation& tm = getWorldTransform(time, iv);
	Box3 worldBoundingBox = _localBoundingBox.transformed(tm);
	for(SceneNode* child : children()) {
		worldBoundingBox.addBox(child->worldBoundingBox(time, vp));
	}
	return worldBoundingBox;
}

/******************************************************************************
* Shows/hides this node in the given viewport, i.e. turns rendering on or off.
******************************************************************************/
void SceneNode::setPerViewportVisibility(Viewport* vp, bool visible)
{
	OVITO_ASSERT(vp);

	if(visible) {
		int index = _hiddenInViewports.indexOf(vp);
		if(index >= 0)
			_hiddenInViewports.remove(this, PROPERTY_FIELD(hiddenInViewports), index);
	}
	else {
		if(!_hiddenInViewports.contains(vp))
			_hiddenInViewports.push_back(this, PROPERTY_FIELD(hiddenInViewports), vp);
	}
}

/******************************************************************************
* Returns whether this scene node (or one of its parents in the node hierarchy) has been hidden 
* specifically in the given viewport.
******************************************************************************/
bool SceneNode::isHiddenInViewport(Viewport* vp, bool includeHierarchyParent) const
{
	OVITO_ASSERT(vp);
	if(_hiddenInViewports.contains(vp))
		return true;
	if(includeHierarchyParent && parentNode())
		return parentNode()->isHiddenInViewport(vp, true);
	else
		return false;
}

}	// End of namespace
