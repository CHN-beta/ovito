////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 OVITO GmbH, Germany
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


#include <ovito/grid/Grid.h>
#include <ovito/mesh/surface/SurfaceMeshAccess.h>
#include <ovito/core/utilities/concurrent/Task.h>

namespace Ovito { namespace Grid {

/**
* The Marching Cubes algorithm for constructing isosurfaces from grid data.
*/
class OVITO_GRID_EXPORT MarchingCubes
{
public:

    // Constructor
    MarchingCubes(SurfaceMeshAccess& outputMesh, int size_x, int size_y, int size_z, bool lowerIsSolid, std::function<FloatType(int i, int j, int k)> field, bool infiniteDomain = false);

    bool generateIsosurface(FloatType iso, Task& task);

    /// Returns the generated surface mesh.
    const SurfaceMeshAccess& mesh() const { return _outputMesh; }

private:

    /// Tessellates one cube.
    void processCube(int i, int j, int k);

    /// Tests if the components of the tessellation of the cube should be
    /// connected by the interior of an ambiguous face.
    bool testFace(signed char face);

    /// Tests if the components of the tessellation of the cube should be
    /// connected through the interior of the cube.
    bool testInterior(signed char s);

    /// Computes almost all the vertices of the mesh by interpolation along the cubes edges.
    void computeIntersectionPoints(FloatType iso, Task& promise);

    /// Adds triangles to the mesh.
    void addTriangle(int i, int j, int k, const signed char* trig, signed char n, SurfaceMeshAccess::vertex_index v12 = SurfaceMeshAccess::InvalidIndex);

    /// Adds a vertex on the current horizontal edge.
    SurfaceMeshAccess::vertex_index createEdgeVertexX(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i + u - (_pbcFlags[0]?0:1), j - (_pbcFlags[1]?0:1), k - (_pbcFlags[2]?0:1)));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 0] = v;
        return v;
    }

    /// Adds a vertex on the current longitudinal edge.
    SurfaceMeshAccess::vertex_index createEdgeVertexY(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i - (_pbcFlags[0]?0:1), j + u - (_pbcFlags[1]?0:1), k - (_pbcFlags[2]?0:1)));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 1] = v;
        return v;
    }

    /// Adds a vertex on the current vertical edge.
    SurfaceMeshAccess::vertex_index createEdgeVertexZ(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i - (_pbcFlags[0]?0:1), j - (_pbcFlags[1]?0:1), k + u - (_pbcFlags[2]?0:1)));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 2] = v;
        return v;
    }

    /// Adds a vertex inside the current cube.
    SurfaceMeshAccess::vertex_index createCenterVertex(int i, int j, int k);

    /// Accesses the pre-computed vertex on a lower edge of a specific cube.
    SurfaceMeshAccess::vertex_index getEdgeVert(int i, int j, int k, int axis) const {
        OVITO_ASSERT(i >= 0 && i <= _size_x);
        OVITO_ASSERT(j >= 0 && j <= _size_y);
        OVITO_ASSERT(k >= 0 && k <= _size_z);
        OVITO_ASSERT(axis >= 0 && axis < 3);
        if(i == _size_x) i = 0;
        if(j == _size_y) j = 0;
        if(k == _size_z) k = 0;
        return _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + axis];
    }

private:

	const std::array<bool,3> _pbcFlags; ///< PBC flags
    int _size_x;  ///< width  of the grid
    int _size_y;  ///< depth  of the grid
    int _size_z;  ///< height of the grid
    std::function<FloatType(int i, int j, int k)> getFieldValue;

    bool _lowerIsSolid; ///< Controls the inward/outward orientation of the created triangle surface.
    bool _infiniteDomain; ///< Controls whether the volumetric domain is infinite extended. 
                          ///< Setting this to true will result in an isosource that is not closed.  
                          ///< This option is used by the VoxelGridSliceModifierDelegate to construct the slice plane.

    /// Vertices created along cube edges.
    std::vector<SurfaceMeshAccess::vertex_index> _cubeVerts;

    FloatType     _cube[8];   ///< values of the implicit function on the active cube
    unsigned char _lut_entry; ///< cube sign representation in [0..255]
    signed char _case;        ///< case of the active cube in [0..15]
    signed char _config;      ///< configuration of the active cube
    signed char _subconfig;   ///< subconfiguration of the active cube

    /// The generated surface mesh.
    SurfaceMeshAccess& _outputMesh;

#ifdef FLOATTYPE_FLOAT
    static constexpr FloatType _epsilon = FloatType(1e-12);
#else
    static constexpr FloatType _epsilon = FloatType(1e-18);
#endif
};

}	// End of namespace
}	// End of namespace
