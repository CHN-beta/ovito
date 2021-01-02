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


#include <ovito/core/Core.h>
#include <ovito/core/rendering/LinePrimitive.h>
#include "OpenGLBuffer.h"

namespace Ovito {

/**
 * \brief This class is responsible for rendering line primitives using OpenGL.
 */
class OpenGLLinePrimitive : public LinePrimitive
{
public:

	/// Constructor.
	OpenGLLinePrimitive(OpenGLSceneRenderer* renderer);

	/// \brief Sets the coordinates of the line vertices.
	virtual void setPositions(ConstDataBufferPtr positions) override {
		LinePrimitive::setPositions(std::move(positions));
		_vectorsBuffer.destroy();
	}

	/// \brief Renders the geometry.
	void render(OpenGLSceneRenderer* renderer);

protected:

	/// \brief Renders the lines using GL_LINES mode.
	void renderThinLines(OpenGLSceneRenderer* renderer);

	/// \brief Renders the lines using polygons.
	void renderThickLines(OpenGLSceneRenderer* renderer);

private:

	/// The internal OpenGL vertex buffer that stores the vertex positions.
	OpenGLBuffer<Point_3<float>> _positionsBuffer;

	/// The internal OpenGL vertex buffer that stores the vertex colors.
	OpenGLBuffer<ColorAT<float>> _colorsBuffer;

	/// The internal OpenGL vertex buffer that stores the line segment vectors.
	OpenGLBuffer<Vector_3<float>> _vectorsBuffer;

	/// The internal OpenGL vertex buffer that stores the indices for a call to glDrawElements().
	OpenGLBuffer<GLuint> _indicesBuffer;

	/// The client-side buffer that stores the indices for a call to glDrawElements().
	std::vector<GLuint> _indicesBufferClient;

	/// The OpenGL shader program used to render the lines.
	QOpenGLShaderProgram* _thinLineShader;

	/// The OpenGL shader program used to render thick lines.
	QOpenGLShaderProgram* _thickLineShader;
};

}	// End of namespace
