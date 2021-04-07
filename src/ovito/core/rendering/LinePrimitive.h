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
#include <ovito/core/dataset/data/DataBuffer.h>
#include <ovito/core/dataset/data/DataBufferAccess.h>
#include "PrimitiveBase.h"

namespace Ovito {

/**
 * \brief Abstract base class for line drawing primitives.
 */
class OVITO_CORE_EXPORT LinePrimitive : public PrimitiveBase
{
public:

	/// \brief Sets the coordinates of the line vertices.
	virtual void setPositions(ConstDataBufferPtr coordinates) {
		OVITO_ASSERT(coordinates);
		OVITO_ASSERT(coordinates->dataType() == DataBuffer::Float && coordinates->componentCount() == 3);
		_positions = std::move(coordinates);
	}

	/// \brief Sets the coordinates of the line vertices.
	template<typename InputIterator>
	void setPositions(DataSet* dataset, InputIterator begin, InputIterator end) {
		size_t count = std::distance(begin, end);
		DataBufferAccessAndRef<Point3> buffer = DataBufferPtr::create(dataset, ExecutionContext::Scripting, count, DataBuffer::Float, 3, 0, false);
		std::copy(std::move(begin), std::move(end), buffer.begin());
		setPositions(buffer.take());
	}

	/// \brief Sets the coordinates of the line vertices.
	template<typename Range>
	void setPositions(DataSet* dataset, const Range& range) {
		setPositions(dataset, std::begin(range), std::end(range));
	}

	/// Returns the buffer storing the vertex positions.
	const ConstDataBufferPtr& positions() const { return _positions; }

	/// \brief Sets the colors of the vertices.
	virtual void setColors(ConstDataBufferPtr colors) {
		OVITO_ASSERT(!colors || colors->dataType() == DataBuffer::Float && colors->componentCount() == 4);
		_colors = std::move(colors);
	}

	/// \brief Sets the colors of the vertices.
	template<typename InputIterator>
	void setColors(DataSet* dataset, InputIterator begin, InputIterator end) {
		size_t count = std::distance(begin, end);
		DataBufferAccessAndRef<ColorA> buffer = DataBufferPtr::create(dataset, ExecutionContext::Scripting, count, DataBuffer::Float, 4, 0, false);
		std::copy(std::move(begin), std::move(end), buffer.begin());
		setColors(buffer.take());
	}

	/// \brief Sets the colors of the vertices.
	template<typename Range>
	void setColors(DataSet* dataset, const Range& range) {
		setColors(dataset, std::begin(range), std::end(range));
	}

	/// Returns the buffer storing the per-vertex colors.
	const ConstDataBufferPtr& colors() const { return _colors; }

	/// \brief Sets the color of all vertices to the given value.
	virtual void setUniformColor(const ColorA& color) { _uniformColor = color; }

	/// \brief Returns the uniform color of all vertices.
	const ColorA& uniformColor() const { return _uniformColor; }

	/// \brief Returns the line width in pixels.
	FloatType lineWidth() const { return _lineWidth; }

	/// \brief Sets the line width in pixels.
	virtual void setLineWidth(FloatType width) { _lineWidth = width; }

private:

	/// The uniform line color.
	ColorA _uniformColor{1,1,1,1};

	/// The line width in pixels.
	FloatType _lineWidth = 0.0;

	/// The buffer storing the vertex positions.
	ConstDataBufferPtr _positions; // Array of Point3

	/// The buffer storing the vertex colors.
	ConstDataBufferPtr _colors; // Array of ColorA
};

}	// End of namespace
