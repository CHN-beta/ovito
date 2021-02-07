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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/ColorParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanParameterUI.h>
#include <ovito/gui/desktop/properties/FloatParameterUI.h>
#include <ovito/gui/desktop/properties/BooleanGroupBoxParameterUI.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include "SurfaceMeshVisEditor.h"

namespace Ovito { namespace Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshVisEditor);
SET_OVITO_OBJECT_EDITOR(SurfaceMeshVis, SurfaceMeshVisEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SurfaceMeshVisEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams, "visual_elements.surface_mesh.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* surfaceGroupBox = new QGroupBox(tr("Surface"));
	QGridLayout* sublayout = new QGridLayout(surfaceGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(surfaceGroupBox);

	ColorParameterUI* surfaceColorUI = new ColorParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::surfaceColor));
	sublayout->addWidget(surfaceColorUI->label(), 0, 0);
	sublayout->addWidget(surfaceColorUI->colorPicker(), 0, 1);

	FloatParameterUI* surfaceTransparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::surfaceTransparencyController));
	sublayout->addWidget(new QLabel(tr("Transparency:")), 1, 0);
	sublayout->addLayout(surfaceTransparencyUI->createFieldLayout(), 1, 1);

	BooleanParameterUI* smoothShadingUI = new BooleanParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::smoothShading));
	sublayout->addWidget(smoothShadingUI->checkBox(), 2, 0, 1, 2);

	BooleanParameterUI* reverseOrientationUI = new BooleanParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::reverseOrientation));
	sublayout->addWidget(reverseOrientationUI->checkBox(), 3, 0, 1, 2);

	BooleanParameterUI* highlightEdgesUI = new BooleanParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::highlightEdges));
	sublayout->addWidget(highlightEdgesUI->checkBox(), 4, 0, 1, 2);

	BooleanGroupBoxParameterUI* capGroupUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::showCap));
	capGroupUI->groupBox()->setTitle(tr("Cap polygons"));
	sublayout = new QGridLayout(capGroupUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(capGroupUI->groupBox());

	ColorParameterUI* capColorUI = new ColorParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::capColor));
	sublayout->addWidget(capColorUI->label(), 0, 0);
	sublayout->addWidget(capColorUI->colorPicker(), 0, 1);

	FloatParameterUI* capTransparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(SurfaceMeshVis::capTransparencyController));
	sublayout->addWidget(new QLabel(tr("Transparency:")), 1, 0);
	sublayout->addLayout(capTransparencyUI->createFieldLayout(), 1, 1);

	// Show the 'Cop polygons' UI only for surface meshes which are closed.
	connect(this, &PropertiesEditor::contentsReplaced, this, [this, box = capGroupUI->groupBox()](RefTarget* editObject) {
		SurfaceMeshVis* surfaceMeshVis = static_object_cast<SurfaceMeshVis>(editObject);
		box->setVisible(surfaceMeshVis && surfaceMeshVis->surfaceIsClosed());
		container()->updateRollouts();
	});
}

}	// End of namespace
}	// End of namespace
