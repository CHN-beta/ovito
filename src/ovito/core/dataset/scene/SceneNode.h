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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/utilities/Color.h>
#include <ovito/core/oo/RefTarget.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/animation/controller/Controller.h>

namespace Ovito {

/**
 * \brief Tree node in the scene hierarchy.
 *
 * A SceneNode is a node in the scene graph. Every object shown in the viewports
 * has an associated SceneNode.
 */
class OVITO_CORE_EXPORT SceneNode : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(SceneNode)

protected:

	/// \brief constructor.
	SceneNode(DataSet* dataset);

public:

	/// \brief Returns this node's world transformation matrix.
	/// \param[in] time The animation for which the transformation matrix should be computed.
	/// \param[in,out] validityInterval The validity interval of the returned transformation matrix.
	///                                 The interval passed to the method is reduced to the time interval during which the transformation stays constant.
	/// \return The matrix that transforms from this node's local space to absolute world space.
	///         This matrix contains also the transformation of the parent node.
	const AffineTransformation& getWorldTransform(TimePoint time, TimeInterval& validityInterval) const;

	/// \brief Returns this node's local transformation matrix.
	/// \param[in] time The animation for which the transformation matrix should be computed.
	/// \param[in,out] validityInterval The validity interval of the returned transformation matrix.
	///                                 The interval passed to the method is reduced to the time interval during which the transformation stays constant.
	/// \return The matrix that transforms from this node's local space to the coordinate space of the parent node.
	///         This matrix does therefore not contain the transformation of the parent node.
	///
	/// The local transformation does not contain the object transform of this node and
	/// does not contain the transformation of the parent node.
	AffineTransformation getLocalTransform(TimePoint time, TimeInterval& validityInterval) const;

	/// \brief Returns the parent node of this node in the scene tree graph.
	/// \return This node's parent node or \c NULL if this is the root node.
	SceneNode* parentNode() const { return _parentNode; }

	/// \brief Deletes this node from the scene.
	///
	/// This will also deletes all child nodes.
	///
	/// \undoable
	Q_INVOKABLE virtual void deleteNode();

	/// \brief Inserts a scene node into this node's list of children.
	/// \param index The position at which to insert the child node into the list of children.
	/// \param newChild The node that becomes a child of this node. If \a newChild is already a child
	///                 of another parent node then it is first removed from that parent.
	///
	/// This method preserves the world transformation of the new child node by calling
	/// Transformation::changeParents() on the node's local transformation controller.
	///
	/// \undoable
	/// \sa children(), addChildNode(), removeChildNode()
	void insertChildNode(int index, SceneNode* newChild);

	/// \brief Adds a child scene node to this node.
	/// \param newChild The node that becomes a child of this node. If \a newChild is already a child
	///                 of another parent node then it is first removed from that parent.
	///
	/// This method preserves the world transformation of the new child node by calling
	/// Transformation::changeParents() on the node's local transformation controller.
	///
	/// \undoable
	/// \sa children(), insertChildNode(), removeChildNode()
	void addChildNode(SceneNode* newChild) {
		insertChildNode(children().size(), newChild);
	}

	/// \brief Removes a child node from this parent node.
	/// \param index An index into this node's list of children.
	///
	/// This method preserves the world transformation of the child node by calling
	/// Transformation::changeParents() on the node's local transformation controller.
	///
	/// \undoable
	/// \sa children(), insertChildNode(), addChildNode()
	void removeChildNode(int index);

	/// \brief Returns whether the given node is a parent of this node.
	bool isChildOf(SceneNode* node) const {
		SceneNode* p = parentNode();
		while(p) {
			if(p == node) return true;
			p = p->parentNode();
		}
		return false;
	}

	/// \brief Recursively visits all nodes below this parent node
	///        and invokes the given visitor function for every node.
	///
	/// \param fn A function that takes a SceneNode pointer as argument and returns a boolean value.
	/// \return true if all child nodes have been visited; false if the loop has been
	///         terminated early because the visitor function has returned false.
	///
	/// The visitor function must return a boolean value to indicate whether
	/// it wants to continue visit more nodes. A return value of false
	/// leads to early termination and no further nodes are visited.
	template<class Function>
	bool visitChildren(Function fn) const {
		for(SceneNode* child : children()) {
			if(!fn(child) || !child->visitChildren(fn))
				return false;
		}
		return true;
	}

	/// \brief Recursively visits all object nodes below this parent node
	///        and invokes the given visitor function for every PipelineSceneNode.
	///
	/// \param fn A function that takes an PipelineSceneNode pointer as argument and returns a boolean value.
	/// \return true if all object nodes have been visited; false if the loop has been
	///         terminated early because the visitor function has returned false.
	///
	/// The visitor function must return a boolean value to indicate whether
	/// it wants to continue visit more nodes. A return value of false
	/// leads to early termination and no further nodes are visited.
	template<class Function>
	bool visitObjectNodes(Function fn) const {
		for(SceneNode* child : children()) {
			if(PipelineSceneNode* objNode = dynamic_object_cast<PipelineSceneNode>(child)) {
				if(!fn(objNode))
					return false;
			}
			else if(!child->visitObjectNodes(fn))
				return false;
		}
		return true;
	}

	/// \brief Binds this scene node to a target node and creates a LookAtController
	///        that lets this scene node look at the target.
	/// \param targetNode The target to look at or \c NULL to unbind the node from its old target.
	/// \return The newly created LookAtController assigned as rotation controller for this node.
	///
	/// The target node will automatically be deleted if this SceneNode is deleted and vice versa.
	/// \undoable
	LookAtController* setLookatTargetNode(SceneNode* targetNode);

	/// \brief Returns the bounding box of the scene node in local coordinates.
	/// \param time The time at which the bounding box should be computed.
	/// \return An axis-aligned box in the node's local coordinate system that contains
	///         the whole node geometry.
	/// \note The returned box does not contains the bounding boxes of the child nodes.
	/// \sa worldBoundingBox()
	virtual Box3 localBoundingBox(TimePoint time, TimeInterval& validity) const = 0;

	/// \brief Returns the bounding box of the scene node in world coordinates.
	/// \param time The time at which the bounding box should be computed.
	/// \param vp The viewport in which to compute the bounding box. If specified, the method takes into account per-viewport visibility of the scene nodes.
	/// \return An axis-aligned box in the world local coordinate system that contains
	///         the whole node geometry including the bounding boxes of all child nodes.
	/// \note The returned box does also contain the bounding boxes of the child nodes.
	Box3 worldBoundingBox(TimePoint time, Viewport* vp = nullptr) const;

	/// \brief Returns whether this scene node is currently selected.
	/// \return \c true if this node is part of the current SelectionSet;
	///         \c false otherwise.
	///
	/// A node is considered selected if it is in the current SelectionSet of the scene
	/// or if its upper most closed group parent is in the selection set.
	bool isSelected() const;

	/// \brief Returns whether this is the root scene node.
	/// \return \c true if this is the root node of the scene.
	///
	/// \sa DataSet::sceneRoot()
	/// \sa parentNode()
	virtual bool isRootNode() const { return false; }

	/// \brief Returns whether this node is part of a scene.
	/// \return \c true if the node has a root node.
	bool isInScene() const {
		const SceneNode* n = this;
		do {
			if(n->isRootNode()) return true;
			n = n->parentNode();
		}
		while(n != nullptr);
		return false;
	}

	/// \brief Returns the title of this object.
	virtual QString objectTitle() const override { return _nodeName; }

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;

	/// Shows/hides this node in the given viewport, i.e. turns rendering on or off.
	void setPerViewportVisibility(Viewport* vp, bool visible);

	/// Returns whether this scene node (or one of its parents in the node hierarchy) has been hidden 
	/// specifically in the given viewport.
	bool isHiddenInViewport(Viewport* vp, bool includeHierarchyParent) const;

protected:

	/// From RefMaker.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// From RefMaker.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex) override;

	/// From RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// From RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) const override;

	/// This method marks the world transformation cache as invalid,
	/// so it will be rebuilt during the next call to getWorldTransform().
	virtual void invalidateWorldTransformation();

	/// This method marks the cached world bounding box as invalid,
	/// so it will be rebuilt during the next call to worldBoundingBox().
	virtual void invalidateBoundingBox();

private:

	/// This node's parent node in the hierarchy.
	SceneNode* _parentNode = nullptr;

	/// Transformation matrix controller.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<Controller>, transformationController, setTransformationController, PROPERTY_FIELD_ALWAYS_DEEP_COPY);

	/// The name of this scene node.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, nodeName, setNodeName);

	/// The display color of the node.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, displayColor, setDisplayColor);

	/// Stores the target node this scene node is bound to using a look
	/// at controller or null if this scene node is not bound to a target node.
	DECLARE_REFERENCE_FIELD_FLAGS(OORef<SceneNode>, lookatTargetNode, PROPERTY_FIELD_ALWAYS_CLONE | PROPERTY_FIELD_NO_SUB_ANIM);

	/// The child nodes of this node.
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(OORef<SceneNode>, children, PROPERTY_FIELD_ALWAYS_CLONE | PROPERTY_FIELD_NO_SUB_ANIM);

	/// Viewports in which this node should NOT be rendered. Can be used to control the visibility in different viewports. 
	DECLARE_VECTOR_REFERENCE_FIELD_FLAGS(Viewport*, hiddenInViewports, PROPERTY_FIELD_NEVER_CLONE_TARGET | PROPERTY_FIELD_WEAK_REF);

	/// This node's cached world transformation matrix.
	/// It contains the transformation of the parent node.
	mutable AffineTransformation _worldTransform;

	/// This time interval indicates for which times the cached world transformation matrix
	/// has been computed.
	mutable TimeInterval _worldTransformValidity;

	/// The cached local bounding box of this node.
	mutable Box3 _localBoundingBox;

	/// Validity time interval of the cached local bounding box.
	mutable TimeInterval _boundingBoxValidity;

	friend class RootSceneNode;
};

}	// End of namespace
