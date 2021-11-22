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
#include <ovito/core/utilities/io/CompressedTextReader.h>
#include <ovito/mesh/tri/TriMeshObject.h>
#include "VTKFileImporter.h"

namespace Ovito::Mesh {

IMPLEMENT_OVITO_CLASS(VTKFileImporter);

/******************************************************************************
* Returns whether this importer class supports importing data of the given type.
******************************************************************************/
bool VTKFileImporter::OOMetaClass::supportsDataType(const DataObject::OOMetaClass& dataObjectType) const
{
	return TriMeshObject::OOClass().isDerivedFrom(dataObjectType);
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool VTKFileImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file.
	CompressedTextReader stream(file);

	// Read first line.
	stream.readLine(24);

	// VTK files start with the string "# vtk DataFile Version".
	if(stream.lineStartsWith("# vtk DataFile Version"))
		return true;

	return false;
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
void VTKFileImporter::FrameLoader::loadFile()
{
	// Open file for reading.
	CompressedTextReader stream(fileHandle());
	setProgressText(tr("Reading VTK file %1").arg(fileHandle().toString()));

	// Jump to byte offset.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset, frame().lineNumber);

	// Read first line.
	stream.readLine(1024);

	// Check header code in first line.
	if(!stream.lineStartsWith("# vtk DataFile Version"))
		throw Exception(tr("Invalid first line in VTK file."));

	// Ignore comment line.
	stream.readLine();

	// Read encoding type.
	stream.readLine();
	if(!stream.lineStartsWithToken("ASCII"))
		throw Exception(tr("Can read only text-based VTK files (ASCII format)."));

	// Read data set type.
	stream.readNonEmptyLine();
	bool isPolyData;
	if(stream.lineStartsWithToken("DATASET UNSTRUCTURED_GRID"))
		isPolyData = false;
	else if(stream.lineStartsWithToken("DATASET POLYDATA"))
		isPolyData = true;
	else
		throw Exception(tr("Can read only read VTK files containing triangle polydata or unstructured grids with triangle cells."));

	// Read point count.
	expectKeyword(stream, "POINTS");
	int pointCount;
	if(sscanf(stream.line() + 6, "%i", &pointCount) != 1 || pointCount < 0)
		throw Exception(tr("Invalid number of points in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));

	// Create mesh data structure.
	TriMeshPtr mesh = std::make_shared<TriMesh>();

	// Parse point coordinates.
	mesh->setVertexCount(pointCount);
	auto v = mesh->vertices().begin();
	size_t component = 0;
	for(int i = 0; i < pointCount; ) {
		if(stream.eof())
			throw Exception(tr("Unexpected end of VTK file in line %1.").arg(stream.lineNumber()));
		const char* s = stream.readLine();
		for(;;) {
			while(*s <= ' ' && *s != '\0') ++s;			// Skip whitespace in front of token
			if(*s == '\0' || i >= pointCount) break;
			(*v)[component++] = (FloatType)std::atof(s);
			if(component == 3) {
				component = 0;
				++v; ++i;
			}
			while(*s > ' ') ++s;						// Proceed to end of token
		}
	}
	mesh->invalidateVertices();

	int polygonCount;
	if(!isPolyData) {
		// Parse number of cells.
		expectKeyword(stream, "CELLS");
		if(sscanf(stream.line() + 5, "%i", &polygonCount) != 1 || polygonCount < 0)
			throw Exception(tr("Invalid number of cells in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
	}
	else {
		// Parse number of polygons.
		expectKeyword(stream, "POLYGONS");
		if(sscanf(stream.line() + 8, "%i", &polygonCount) != 1 || polygonCount < 0)
			throw Exception(tr("Invalid number of polygons in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
	}

	// Parse polygons.
	for(int i = 0; i < polygonCount; i++) {
		int vcount, s;
		const char* line = stream.readLine();
		if(sscanf(line, "%i%n", &vcount, &s) != 1 || vcount <= 2)
			throw Exception(tr("Invalid polygon/cell definition in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
		int vindices[3];
		for(int j = 0; j < vcount; j++) {
			line += s;
			if(sscanf(line, "%i%n", &vindices[std::min(j,2)], &s) != 1)
				throw Exception(tr("Invalid polygon/cell definition in VTK file (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(j >= 2) {
				if(vindices[0] >= pointCount || vindices[1] >= pointCount || vindices[2] >= pointCount)
					throw Exception(tr("Vertex indices out of range in polygon/cell (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
				TriMeshFace& f = mesh->addFace();
				f.setVertices(vindices[0], vindices[1], vindices[2]);
				vindices[1] = vindices[2];
			}
		}
	}
	mesh->determineEdgeVisibility();

	if(!isPolyData) {
		// Parse cell types
		expectKeyword(stream, "CELL_TYPES");
		for(int i = 0; i < polygonCount; i++) {
			int t;
			if(sscanf(stream.readLine(), "%i", &t) != 1 || t != 5)
				throw Exception(tr("Invalid cell type in VTK file (line %1): %2. Only triangle cells are supported by OVITO.").arg(stream.lineNumber()).arg(stream.lineString()));
		}

		// Look for color information.
		for(; !stream.eof() && !stream.lineStartsWith("CELL_DATA"); )
			stream.readLine();
		bool isPointData = false;
		for(; !stream.eof() && !stream.lineStartsWith("COLOR_SCALARS"); ) {
			if(stream.lineStartsWith("POINT_DATA"))
				isPointData = true;
			stream.readLine();
		}
		if(!stream.eof()) {
			int componentCount = 0;
			if(sscanf(stream.line(), "COLOR_SCALARS %*s %i", &componentCount) != 1 || (componentCount != 3 && componentCount != 4))
				throw Exception(tr("Invalid COLOR_SCALARS property in line %1 of VTK file. Component count must be 3 or 4.").arg(stream.lineNumber()));
			if(!isPointData) {
				mesh->setHasFaceColors(true);
				auto& faceColors = mesh->faceColors();
				std::fill(faceColors.begin(), faceColors.end(), ColorA(1,1,1,1));
				component = 0;
				for(int i = 0; i < polygonCount;) {
					if(stream.eof())
						throw Exception(tr("Unexpected end of VTK file in line %1.").arg(stream.lineNumber()));
					const char* s = stream.readLine();
					for(; ;) {
						while(*s <= ' ' && *s != '\0') ++s;			// Skip whitespace in front of token
						if(!*s || i >= polygonCount) break;
						faceColors[i][component++] = (FloatType)std::atof(s);
						if(component == componentCount) {
							component = 0;
							++i;
						}
						while(*s > ' ') ++s;						// Proceed to end of token
					}
				}
			}
			else {
				mesh->setHasVertexColors(true);
				auto& vertexColors = mesh->vertexColors();
				std::fill(vertexColors.begin(), vertexColors.end(), ColorA(1,1,1,1));
				component = 0;
				for(int i = 0; i < pointCount;) {
					if(stream.eof())
						throw Exception(tr("Unexpected end of VTK file in line %1.").arg(stream.lineNumber()));
					const char* s = stream.readLine();
					for(; ;) {
						while(*s <= ' ' && *s != '\0') ++s;			// Skip whitespace in front of token
						if(!*s || i >= pointCount) break;
						vertexColors[i][component++] = (FloatType)std::atof(s);
						if(component == componentCount) {
							component = 0;
							++i;
						}
						while(*s > ' ') ++s;						// Proceed to end of token
					}
				}
				mesh->invalidateVertices();
			}
		}
	}
	else {
		// ...to be implemented.
	}

	// Show some stats to the user.
	state().setStatus(tr("%1 vertices, %2 triangles").arg(pointCount).arg(mesh->faceCount()));

	// Add mesh to the data collection.
	TriMeshObject* meshObj = state().getMutableObject<TriMeshObject>();
	if(!meshObj)
		meshObj = state().createObject<TriMeshObject>(dataSource(), executionContext());
	meshObj->setMesh(std::move(mesh));	
}

/******************************************************************************
* Reads from the input stream and throws an exception if the given keyword
* is not present.
******************************************************************************/
void VTKFileImporter::FrameLoader::expectKeyword(CompressedTextReader& stream, const char* keyword)
{
	stream.readNonEmptyLine();

	// Skip METADATA sections written by Paraview.
	if(stream.lineStartsWith("METADATA")) {
		while(!stream.eof()) {
			const char* line = stream.readLineTrimLeft();
			if(line[0] <= ' ') break;
		}
		stream.readNonEmptyLine();
	}

	if(!stream.lineStartsWith(keyword))
		throw Exception(tr("Invalid or unsupported VTK file format. Expected token '%1' in line %2, but found '%3'.").arg(keyword).arg(stream.lineNumber()).arg(stream.lineString().trimmed()));
}

}	// End of namespace
