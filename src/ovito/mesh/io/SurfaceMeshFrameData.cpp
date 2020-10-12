////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/io/FileSource.h>
#include "SurfaceMeshFrameData.h"

namespace Ovito { namespace Mesh {

/******************************************************************************
* Inserts the loaded data into the provided container object.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
OORef<DataCollection> SurfaceMeshFrameData::handOver(const DataCollection* existing, bool isNewFile, CloneHelper& cloneHelper, FileSource* fileSource, const QString& identifierPrefix)
{
	OORef<DataCollection> output = new DataCollection(fileSource->dataset());

	QString identifier = identifierPrefix.isEmpty() ? QStringLiteral("surface") : identifierPrefix;

	OORef<SurfaceMesh> surfaceObj;
	if(const SurfaceMesh* existingSurfaceObj = existing ? static_object_cast<SurfaceMesh>(existing->getLeafObject(SurfaceMesh::OOClass(), identifier)) : nullptr) {
		surfaceObj = cloneHelper.cloneObject(existingSurfaceObj, false);
		output->addObject(surfaceObj);
	}
	else {
		surfaceObj = output->createObject<SurfaceMesh>(fileSource, identifierPrefix.isEmpty() ? QStringLiteral("Surface mesh") : QStringLiteral("Mesh: %1").arg(identifierPrefix));
		surfaceObj->setIdentifier(identifier);
		OORef<SurfaceMeshVis> vis = new SurfaceMeshVis(fileSource->dataset());
		vis->setSurfaceIsClosed(false);
		if(!identifierPrefix.isEmpty())
			vis->setTitle(QStringLiteral("Surface mesh: %1").arg(identifierPrefix));
		if(Application::instance()->executionContext() == Application::ExecutionContext::Interactive)
			vis->loadUserDefaults();
		surfaceObj->setVisElement(vis);
	}
	mesh().transferTo(surfaceObj);

	return output;
}

}	// End of namespace
}	// End of namespace
