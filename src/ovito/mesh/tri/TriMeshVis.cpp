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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/MeshPrimitive.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "TriMeshVis.h"

namespace Ovito::Mesh {

IMPLEMENT_OVITO_CLASS(TriMeshVis);
DEFINE_REFERENCE_FIELD(TriMeshVis, transparencyController);
SET_PROPERTY_FIELD_LABEL(TriMeshVis, color, "Display color");
SET_PROPERTY_FIELD_LABEL(TriMeshVis, transparencyController, "Transparency");
SET_PROPERTY_FIELD_LABEL(TriMeshVis, highlightEdges, "Highlight edges");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TriMeshVis, transparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
TriMeshVis::TriMeshVis(DataSet* dataset) : DataVis(dataset),
	_color(0.85, 0.85, 1),
	_highlightEdges(false)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void TriMeshVis::initializeObject(ExecutionContext executionContext)
{
	setTransparencyController(ControllerManager::createFloatController(dataset(), executionContext));

	DataVis::initializeObject(executionContext);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TriMeshVis::boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// Compute bounding box.
	if(const TriMeshObject* triMeshObj = dynamic_object_cast<TriMeshObject>(path.back())) {
		if(triMeshObj->mesh())
			return triMeshObj->mesh()->boundingBox();
	}
	return Box3();
}

/******************************************************************************
* Lets the vis element render a data object.
******************************************************************************/
PipelineStatus TriMeshVis::render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	if(!renderer->isBoundingBoxPass()) {

		// The key type used for caching the rendering primitive:
		using CacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			ConstDataObjectRef,			// Mesh object
			ColorA,						// Display color
			bool						// Edge highlighting
		>;

		FloatType transp = 0;
		TimeInterval iv;
		if(transparencyController()) transp = transparencyController()->getFloatValue(time, iv);
		ColorA color_mesh(color(), FloatType(1) - transp);

		// Lookup the rendering primitive in the vis cache.
		auto& meshPrimitive = dataset()->visCache().get<std::shared_ptr<MeshPrimitive>>(CacheKey(renderer, path.back(), color_mesh, highlightEdges()));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!meshPrimitive) {
			meshPrimitive = renderer->createMeshPrimitive();
			meshPrimitive->setEmphasizeEdges(highlightEdges());
			meshPrimitive->setUniformColor(color_mesh);
			if(const TriMeshObject* triMeshObj = dynamic_object_cast<TriMeshObject>(path.back()))
				if(triMeshObj->mesh())
					meshPrimitive->setMesh(*triMeshObj->mesh());
		}

		renderer->beginPickObject(contextNode);
		renderer->renderMesh(meshPrimitive);
		renderer->endPickObject();
	}
	else {
		// Add mesh to bounding box.
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, path, contextNode, flowState, validityInterval));
	}

	return {};
}

}	// End of namespace
