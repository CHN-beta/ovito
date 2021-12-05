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


#include <ovito/mesh/Mesh.h>
#include <ovito/core/dataset/data/mesh/TriMeshObject.h>
#include "polytess/glu.h"

namespace Ovito::Mesh {

/**
 * \brief Helper class that can tessellate a set of non-convex polygons into triangles.
 */
class OVITO_MESH_EXPORT CapPolygonTessellator
{
public:

	/// Constructor.
	CapPolygonTessellator(TriMeshObject& output, size_t dim, bool createOppositePolygon = true, bool windingRuleNonzero = false) : mesh(output), dimz(dim), _createOppositePolygon(createOppositePolygon) {
		dimx = (dimz + 1) % 3;
		dimy = (dimz + 2) % 3;
		tess = gluNewTess();
		gluTessProperty(tess, GLU_TESS_WINDING_RULE, windingRuleNonzero ? GLU_TESS_WINDING_NONZERO : GLU_TESS_WINDING_ODD);
		gluTessCallback(tess, GLU_TESS_ERROR_DATA, (void (*)())errorData);
		gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (void (*)())beginData);
		gluTessCallback(tess, GLU_TESS_END_DATA, (void (*)())endData);
		gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void (*)())vertexData);
		gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void (*)())combineData);
	}

	/// Destructor.
	~CapPolygonTessellator() {
		gluDeleteTess(tess);
	}

	void beginPolygon() {
		gluTessNormal(tess, 0, 0, 1);
		gluTessBeginPolygon(tess, this);
	}

	void endPolygon() {
		gluTessEndPolygon(tess);
	}

	void beginContour() {
		gluTessBeginContour(tess);
	}

	void endContour() {
		gluTessEndContour(tess);
	}

	void vertex(const Point2& pos) {
		double vertexCoord[3];
		vertexCoord[0] = pos.x();
		vertexCoord[1] = pos.y();
		vertexCoord[2] = 0;
		Point3 p;
		p[dimx] = pos.x();
		p[dimy] = pos.y();
		p[dimz] = 0;
		intptr_t vindex = mesh.addVertex(p);
		if(_createOppositePolygon) {
			p[dimz] = 1;
			mesh.addVertex(p);
		}
		gluTessVertex(tess, vertexCoord, reinterpret_cast<void*>(vindex));
	}

	static void beginData(int type, void* polygon_data) {
		CapPolygonTessellator* tessellator = static_cast<CapPolygonTessellator*>(polygon_data);
		tessellator->primitiveType = type;
		tessellator->vertices.clear();
	}

	static void endData(void* polygon_data) {
		CapPolygonTessellator* tessellator = static_cast<CapPolygonTessellator*>(polygon_data);
		if(tessellator->primitiveType == GL_TRIANGLE_FAN) {
			OVITO_ASSERT(tessellator->vertices.size() >= 4);
			int facetVertices[3];
			facetVertices[0] = tessellator->vertices[0];
			facetVertices[1] = tessellator->vertices[1];
			for(auto v = tessellator->vertices.cbegin() + 2; v != tessellator->vertices.cend(); ++v) {
				facetVertices[2] = *v;
				tessellator->mesh.addFace().setVertices(facetVertices[2], facetVertices[1], facetVertices[0]);
				if(tessellator->_createOppositePolygon)
					tessellator->mesh.addFace().setVertices(facetVertices[0]+1, facetVertices[1]+1, facetVertices[2]+1);
				facetVertices[1] = facetVertices[2];
			}
		}
		else if(tessellator->primitiveType == GL_TRIANGLE_STRIP) {
			OVITO_ASSERT(tessellator->vertices.size() >= 3);
			int facetVertices[3];
			facetVertices[0] = tessellator->vertices[0];
			facetVertices[1] = tessellator->vertices[1];
			bool even = true;
			for(auto v = tessellator->vertices.cbegin() + 2; v != tessellator->vertices.cend(); ++v) {
				facetVertices[2] = *v;
				tessellator->mesh.addFace().setVertices(facetVertices[2], facetVertices[1], facetVertices[0]);
				if(tessellator->_createOppositePolygon)
					tessellator->mesh.addFace().setVertices(facetVertices[0]+1, facetVertices[1]+1, facetVertices[2]+1);
				if(even)
					facetVertices[0] = facetVertices[2];
				else
					facetVertices[1] = facetVertices[2];
				even = !even;
			}
		}
		else if(tessellator->primitiveType == GL_TRIANGLES) {
			for(auto v = tessellator->vertices.cbegin(); v != tessellator->vertices.cend(); v += 3) {
				tessellator->mesh.addFace().setVertices(v[2], v[1], v[0]);
				if(tessellator->_createOppositePolygon)
					tessellator->mesh.addFace().setVertices(v[0]+1, v[1]+1, v[2]+1);
			}
		}
		else OVITO_ASSERT(false);
	}

	static void vertexData(void* vertex_data, void* polygon_data) {
		CapPolygonTessellator* tessellator = static_cast<CapPolygonTessellator*>(polygon_data);
		tessellator->vertices.push_back(reinterpret_cast<intptr_t>(vertex_data));
	}

	static void combineData(double coords[3], void* vertex_data[4], float weight[4], void** outDatab, void* polygon_data) {
		CapPolygonTessellator* tessellator = static_cast<CapPolygonTessellator*>(polygon_data);
		Point3 p;
		p[tessellator->dimx] = coords[0];
		p[tessellator->dimy] = coords[1];
		p[tessellator->dimz] = 0;
		intptr_t vindex = tessellator->mesh.addVertex(p);
		*outDatab = reinterpret_cast<void*>(vindex);
		if(tessellator->_createOppositePolygon) {
			p[tessellator->dimz] = 1;
			tessellator->mesh.addVertex(p);
		}
	}

	static void errorData(int errnum, void* polygon_data) {
		if(errnum == GLU_TESS_NEED_COMBINE_CALLBACK)
			qDebug() << "ERROR: Could not tessellate cap polygon. It contains overlapping contours.";
		else
			qDebug() << "ERROR: Could not tessellate cap polygon. Error code: " << errnum;
	}

private:

	size_t dimx, dimy, dimz;
	GLUtesselator* tess;
	TriMeshObject& mesh;
	int primitiveType;
	std::vector<int> vertices;
	bool _createOppositePolygon;
};

}	// End of namespace
