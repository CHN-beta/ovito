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
#include <ovito/core/dataset/scene/SelectionSet.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(SelectionSet);
DEFINE_VECTOR_REFERENCE_FIELD(SelectionSet, nodes);
SET_PROPERTY_FIELD_LABEL(SelectionSet, nodes, "Nodes");

/******************************************************************************
* Default constructor.
******************************************************************************/
SelectionSet::SelectionSet(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Adds a scene node to this selection set.
******************************************************************************/
void SelectionSet::push_back(SceneNode* node)
{
	OVITO_CHECK_OBJECT_POINTER(node);
	if(nodes().contains(node))
		throwException(tr("Node is already in the selection set."));

	// Insert into children array.
	_nodes.push_back(this, PROPERTY_FIELD(nodes), node);
}

/******************************************************************************
* Inserts a scene node into this selection set.
******************************************************************************/
void SelectionSet::insert(int index, SceneNode* node)
{
	OVITO_CHECK_OBJECT_POINTER(node);
	if(nodes().contains(node))
		throwException(tr("Node is already in the selection set."));

	// Insert into children array.
	_nodes.insert(this, PROPERTY_FIELD(nodes), index, node);
}

/******************************************************************************
* Removes a scene node from this selection set.
******************************************************************************/
void SelectionSet::remove(SceneNode* node)
{
	int index = _nodes.indexOf(node);
	if(index == -1) return;
	removeByIndex(index);
	OVITO_ASSERT(!nodes().contains(node));
}

/******************************************************************************
* Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
******************************************************************************/
void SelectionSet::referenceInserted(const PropertyFieldDescriptor* field, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(nodes)) {
		Q_EMIT selectionChanged(this);
		if(!_selectionChangeInProgress) {
			_selectionChangeInProgress = true;
			QMetaObject::invokeMethod(this, "onSelectionChangeCompleted", Qt::QueuedConnection);
		}
	}
	RefTarget::referenceInserted(field, newTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been removed from a VectorReferenceField of this RefMaker.
******************************************************************************/
void SelectionSet::referenceRemoved(const PropertyFieldDescriptor* field, RefTarget* oldTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(nodes)) {
		Q_EMIT selectionChanged(this);
		if(!_selectionChangeInProgress) {
			_selectionChangeInProgress = true;
			QMetaObject::invokeMethod(this, "onSelectionChangeCompleted", Qt::QueuedConnection);
		}
	}
	RefTarget::referenceRemoved(field, oldTarget, listIndex);
}

/******************************************************************************
* Is called when a RefTarget has been replaced in a VectorReferenceField of this RefMaker.
******************************************************************************/
void SelectionSet::referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(nodes)) {
		Q_EMIT selectionChanged(this);
		if(!_selectionChangeInProgress) {
			_selectionChangeInProgress = true;
			QMetaObject::invokeMethod(this, "onSelectionChangeCompleted", Qt::QueuedConnection);
		}
	}
	RefTarget::referenceReplaced(field, oldTarget, newTarget, listIndex);
}

/******************************************************************************
* This method is invoked after the change of the selection set is complete.
* It emits the selectionChangeComplete() signal.
******************************************************************************/
void SelectionSet::onSelectionChangeCompleted()
{
	OVITO_ASSERT(_selectionChangeInProgress);
	_selectionChangeInProgress = false;
	Q_EMIT selectionChangeComplete(this);
}

}	// End of namespace
