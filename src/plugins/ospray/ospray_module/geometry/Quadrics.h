///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

// ospcomon: vec3f, box3f, etcpp - generic helper stuff
#include <ospcommon/vec.h>
#include <ospcommon/box.h>
// ospray: everything that's related to the ospray ray tracing core
#include <geometry/Geometry.h>
#include <common/Model.h>

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {
  namespace ovito {
    // import ospcommon component - vec3f etc
    using namespace ospcommon;

    /*! a geometry type that implements quadrics. 
      This implements a new ospray geometry, and as such has
      to

      a) derive from ospray::Geometry
      b) implement a 'commit()' message that parses the
         parameters/data arrays that the app has specified as inputs
      c) create an actual ospray geometry instance with the
         proper intersect() and postIntersect() functions.

      Note that how this class is called does not particularly matter;
      all that matters is under which name it is registered in the cpp
      file (see comments on OSPRAY_REGISTER_GEOMETRY)
    */
    struct Quadrics : public ospray::Geometry
    {
      /*! constructor - will create the 'ispc equivalent' */
      Quadrics();

      /*! destructor - supposed to clean up all alloced memory */
      virtual ~Quadrics() override;

      /*! 'finalize' is what ospray calls when everything is set and
        done, and a actual user geometry has to be built */
      virtual void finalize(Model *model) override;

      size_t numQuadrics;
      size_t bytesPerQuadric;
      int32 materialID;
      int64 offset_center;
      int64 offset_radius;
      int64 offset_coeff;
      int64 offset_materialID;
      int64 offset_colorID;

      /*! the input data array. */
      Ref<Data> quadricData;

      Ref<Data> materialList;
      void     *_materialList {nullptr};
      Ref<Data> texcoordData;

      /*! data array from which we read the per-quadric color data; if
        NULL we do not have per-quadric data */
      Ref<Data> colorData;
      
      /*! stride in colorData array for accessing i'th quadric's
        color. color of disc i will be read as 3 floats from
        'colorOffset+i*colorStride */
      size_t    colorStride;
        
      /*! offset in colorData array for accessing i'th quadric's
        color. color of disc i will be read as 3 floats from
        'colorOffset+i*colorStride */
      size_t    colorOffset;
    };

  } // ::ospray::ovito
} // ::ospray

