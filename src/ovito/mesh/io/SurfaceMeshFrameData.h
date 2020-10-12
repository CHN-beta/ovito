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

#pragma once


#include <ovito/mesh/Mesh.h>
#include <ovito/mesh/surface/SUrfaceMeshData.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>

namespace Ovito { namespace Mesh {

/**
 * \brief Container for a surface mesh read from a file by a file importer.
 */
class SurfaceMeshFrameData : public FileSourceImporter::FrameData
{
public:

	/// Inserts the loaded loaded into the provided pipeline state structure. This function is
	/// called by the system from the main thread after the asynchronous loading task has finished.
	virtual OORef<DataCollection> handOver(const DataCollection* existing, bool isNewFile, CloneHelper& cloneHelper, FileSource* fileSource, const QString& identifierPrefix = {}) override;

	/// Returns the surface mesh data structure.
	const SurfaceMeshData& mesh() const { return _mesh; }

	/// Returns a reference to the surface mesh data structure.
	SurfaceMeshData& mesh() { return _mesh; }

private:

	/// The surface mesh data.
	SurfaceMeshData _mesh;
};

}	// End of namespace
}	// End of namespace
