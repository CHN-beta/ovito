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


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataVis.h>

namespace Ovito::StdObj {

/**
 * \brief A visual element that renders a SimulationCellObject as a wireframe box.
 */
class OVITO_STDOBJ_EXPORT SimulationCellVis : public DataVis
{
	OVITO_CLASS(SimulationCellVis)
	Q_CLASSINFO("DisplayName", "Simulation cell");

public:

	/// \brief Constructor.
	Q_INVOKABLE SimulationCellVis(ObjectCreationParams params);

	/// \brief Lets the visualization element render the data object.
	virtual PipelineStatus render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// \brief Indicates whether this object should be surrounded by a selection marker in the viewports when it is selected.
	virtual bool showSelectionMarker() override { return false; }

protected:

	/// Renders the given simulation using wireframe mode.
	void renderWireframe(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

	/// Renders the given simulation using solid shading mode.
	void renderSolid(TimePoint time, const SimulationCellObject* cell, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

protected:

	/// Controls the line width used to render the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cellLineWidth, setCellLineWidth);
	DECLARE_SHADOW_PROPERTY_FIELD(cellLineWidth);

	/// Controls whether the simulation cell is visible.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, renderCellEnabled, setRenderCellEnabled);

	/// Controls the rendering color of the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, cellColor, setCellColor, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
