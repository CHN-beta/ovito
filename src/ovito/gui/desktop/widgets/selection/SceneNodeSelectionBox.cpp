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
#include "SceneNodeSelectionBox.h"
#include "SceneNodesListModel.h"

namespace Ovito {

/******************************************************************************
* Constructs the widget.
******************************************************************************/
SceneNodeSelectionBox::SceneNodeSelectionBox(DataSetContainer& datasetContainer, ActionManager* actionManager, QWidget* parent) : QComboBox(parent)
{
	setInsertPolicy(QComboBox::NoInsert);
	setEditable(false);
	setMinimumContentsLength(40);
	setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	setToolTip(tr("Pipeline selector"));
	setIconSize(QSize(24, 24));

	// Set the list model, which tracks the list of pipelines in the scene.
	setModel(new SceneNodesListModel(datasetContainer, actionManager, this));

	// Wire the combobox selection to the list model.
	connect(this, QOverload<int>::of(&QComboBox::activated), static_cast<SceneNodesListModel*>(model()), &SceneNodesListModel::activateItem);
	connect(static_cast<SceneNodesListModel*>(model()), &SceneNodesListModel::selectionChangeRequested, this, &QComboBox::setCurrentIndex);
}

}	// End of namespace
