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

#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SurfaceMesh.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/app/Application.h>
#include "SurfaceMeshSliceModifierDelegate.h"

namespace Ovito::Mesh {

IMPLEMENT_OVITO_CLASS(SurfaceMeshSliceModifierDelegate);

/******************************************************************************
* Performs the slicing of a surface mesh.
******************************************************************************/
PipelineStatus SurfaceMeshSliceModifierDelegate::apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(modifier);
	QString statusMessage;

	// Obtain modifier parameter values.
	Plane3 plane;
	FloatType sliceWidth;
	std::tie(plane, sliceWidth) = mod->slicingPlane(time, state.mutableStateValidity(), state);
	sliceWidth /= 2;
	bool invert = mod->inverse();

	for(const DataObject* obj : state.data()->objects()) {
		if(const SurfaceMesh* inputMesh = dynamic_object_cast<SurfaceMesh>(obj)) {
			inputMesh->verifyMeshIntegrity();
			SurfaceMesh* outputMesh = state.makeMutable(inputMesh);
			if(!mod->createSelection()) {
				QVector<Plane3> planes = outputMesh->cuttingPlanes();
				if(sliceWidth <= 0) {
					planes.push_back(plane);
				}
				else {
					planes.push_back(Plane3( plane.normal,  plane.dist + sliceWidth));
					planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth));
				}
				outputMesh->setCuttingPlanes(std::move(planes));
			}
			else {
				// Create a mesh vertex selection.
				if(SurfaceMeshVertices* outputVertices = outputMesh->makeVerticesMutable()) {
					ConstPropertyAccess<Point3> vertexPositionProperty = outputVertices->expectProperty(SurfaceMeshVertices::PositionProperty);
					PropertyAccess<int> vertexSelectionProperty = outputVertices->createProperty(SurfaceMeshVertices::SelectionProperty, false, Application::instance()->executionContext());
					size_t numSelectedVertices = 0;
					boost::transform(vertexPositionProperty, vertexSelectionProperty.begin(), [&](const Point3& pos) {
						bool selectionState = 
							(sliceWidth <= 0) ?
								(plane.pointDistance(pos) > 0) :
								(invert == (plane.classifyPoint(pos, sliceWidth) == 0));
						if(selectionState)
							numSelectedVertices++;
						return selectionState ? 1 : 0;
					});
					if(!statusMessage.isEmpty()) statusMessage += QChar('\n');
					statusMessage += tr("%1 of %2 mesh vertices selected").arg(numSelectedVertices).arg(outputVertices->elementCount());

					// Create a mesh face selection.
					if(SurfaceMeshFaces* outputFaces = outputMesh->makeFacesMutable()) {
						PropertyAccess<int> faceSelectionProperty = outputFaces->createProperty(SurfaceMeshFaces::SelectionProperty, false, Application::instance()->executionContext());
						size_t numSelectedFaces = 0;
						const SurfaceMeshTopology* topology = outputMesh->topology();
						auto firstFaceEdge = topology->firstFaceEdges().cbegin();
						OVITO_ASSERT(topology->firstFaceEdges().size() == faceSelectionProperty.size());
						for(int& selected : faceSelectionProperty) {
							SurfaceMesh::edge_index ffe = *firstFaceEdge++;
							SurfaceMesh::edge_index e = ffe;
							selected = 1;
							do {
								SurfaceMesh::vertex_index ev = topology->vertex2(e);
								if(ev < 0 || ev >= vertexSelectionProperty.size() || !vertexSelectionProperty[ev]) {
									selected = 0;
									break;
								}
								e = topology->nextFaceEdge(e);
							}
							while(e != ffe);
							if(selected)
								numSelectedFaces++;
						}
						if(!statusMessage.isEmpty()) statusMessage += QChar('\n');
						statusMessage += tr("%1 of %2 mesh faces selected").arg(numSelectedFaces).arg(outputFaces->elementCount());
					}
				}
			}
		}
	}

	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

}	// End of namespace
