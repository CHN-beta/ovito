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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/rendering/MarkerPrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief This class is responsible for rendering marker primitives using OpenGL.
 */
class OpenGLMarkerPrimitive : public MarkerPrimitive
{
public:

	/// Constructor.
	OpenGLMarkerPrimitive(OpenGLSceneRenderer* renderer, MarkerShape shape);

	/// \brief Renders the geometry.
	void render(OpenGLSceneRenderer* renderer);

private:

	/// The internal OpenGL vertex buffer that stores the marker positions.
	OpenGLBuffer<Point_3<float>> _positionsBuffer;

	/// The OpenGL shader program that is used to render the markers.
	QOpenGLShaderProgram* _shader = nullptr;
};

}	// End of namespace
