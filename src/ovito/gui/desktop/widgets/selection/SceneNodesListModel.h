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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/oo/RefTargetListener.h>

namespace Ovito {

/**
 * A Qt model/view system list model that contains all scene nodes in the current scene.
 */
class OVITO_GUI_EXPORT SceneNodesListModel : public QAbstractListModel
{
	Q_OBJECT

public:

	/// Constructs the model.
	SceneNodesListModel(DataSetContainer& datasetContainer, ActionManager* actionManager, QWidget* parent = nullptr);

	/// Returns the number of rows of the model.
	virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;

	/// Returns the model's data stored under the given role for the item referred to by the index.
	virtual QVariant data(const QModelIndex & index, int role) const override;

	/// Returns the item flags for the given index.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

	/// Returns the current list of scene nodes in the scene.
	const QVector<SceneNode*>& sceneNodes() const { return _nodeListener.targets(); }

	/// Returns the scene node at the given index of the list model.
	SceneNode* sceneNodeFromListIndex(int index) const { 
		int nodeIndex = index - firstSceneNodeIndex();
		return (nodeIndex >= 0 && nodeIndex < sceneNodes().size()) ? sceneNodes()[nodeIndex] : nullptr;
	}

public Q_SLOTS:

	/// This slot executes the action associated with the given list item.
	void activateItem(int index);

	/// Performs a deletion action on an item.
	void deleteItem(int index);

Q_SIGNALS:

	/// This signal is emitted by the model to request a selection change in the widget it is attached to.
	void selectionChangeRequested(int index);

private Q_SLOTS:

	/// This is called when a new dataset has been loaded.
	void onDataSetChanged(DataSet* newDataSet);

	/// This is called whenever a different scene node has been selected by the user.
	void onSceneSelectionChanged();

	/// This handles reference events generated by the root node.
	void onRootNodeNotificationEvent(RefTarget* source, const ReferenceEvent& event);

	/// This handles reference events generated by the scene nodes.
	void onNodeNotificationEvent(RefTarget* source, const ReferenceEvent& event);

private:

	/// Returns the list index of the first scene node item.
	int firstSceneNodeIndex() const { return 1; }

	/// Returns the list index of the first action item.
	int firstActionIndex() const { return sceneNodes().empty() ? 3 : (2 + sceneNodes().size()); }

	/// The container of the dataset.
	DataSetContainer& _datasetContainer;

	/// Used to receive reference events generated by the scene nodes.
	VectorRefTargetListener<SceneNode> _nodeListener;

	/// Used to receive signals sent by the root node.
	RefTargetListener<RootSceneNode> _rootNodeListener;

	/// The actions that are displayed in the combobox.
	QVector<QAction*> _pipelineActions;

	/// The background brush used for list section headers.
	QBrush _sectionHeaderBackgroundBrush;

	/// The foreground brush used for list section headers.
	QBrush _sectionHeaderForegroundBrush;

	/// Icon representing a pipeline scene node.
	QIcon _pipelineSceneNodeIcon;

	/// Font for rendering selected scene nodes.
	QFont _selectedNodeFont;
};

}	// End of namespace
