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
#include <ovito/core/dataset/data/DataBuffer.h>
#include "PrimitiveBase.h"

namespace Ovito {

/**
 * \brief Abstract base class for rendering cylinder primitives and arrow glyphs.
 */
class OVITO_CORE_EXPORT CylinderPrimitive : public PrimitiveBase
{
public:

	enum ShadingMode {
		NormalShading,
		FlatShading,
	};
	Q_ENUMS(ShadingMode);

	enum RenderingQuality {
		LowQuality,
		MediumQuality,
		HighQuality
	};
	Q_ENUMS(RenderingQuality);

	enum Shape {
		CylinderShape,
		ArrowShape,
	};
	Q_ENUMS(Shape);

public:

	/// Constructor.
	CylinderPrimitive(Shape shape, ShadingMode shadingMode, RenderingQuality renderingQuality) :
		_shape(shape), _shadingMode(shadingMode), _renderingQuality(renderingQuality) {}

	/// \brief Returns the shading mode for elements.
	ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Returns the rendering quality of elements.
	RenderingQuality renderingQuality() const { return _renderingQuality; }

	/// \brief Returns the selected element shape.
	Shape shape() const { return _shape; }

	/// Returns the cylinder radius assigned to all primtives.
	FloatType uniformRadius() const { return _uniformRadius; }

	/// Sets the cylinder radius of all primitives to the given value.
	virtual void setUniformRadius(FloatType radius) {
		_uniformRadius = radius;
	}

	/// Returns the color assigned to all primitives.
	const Color& uniformColor() const { return _uniformColor; }

	/// Sets the color of all primitives to the given value.
	virtual void setUniformColor(const Color& color) {
		_uniformColor = color;
	}

	/// Returns the buffer storing the base positions.
	const ConstDataBufferPtr& basePositions() const { return _basePositions; }

	/// Returns the buffer storing the head positions.
	const ConstDataBufferPtr& headPositions() const { return _headPositions; }

	/// Sets the coordinates of the base and the head points.
	virtual void setPositions(ConstDataBufferPtr baseCoordinates, ConstDataBufferPtr headCoordinates) {
		OVITO_ASSERT(baseCoordinates && headCoordinates);
		OVITO_ASSERT(baseCoordinates->dataType() == DataBuffer::Float && baseCoordinates->componentCount() == 3);
		OVITO_ASSERT(headCoordinates->dataType() == DataBuffer::Float && headCoordinates->componentCount() == 3);
		OVITO_ASSERT(baseCoordinates->size() == headCoordinates->size());
		_basePositions = std::move(baseCoordinates);
		_headPositions = std::move(headCoordinates);
	}

	/// Returns the buffer storing the colors.
	const ConstDataBufferPtr& colors() const { return _colors; }

	/// Sets the per-primitive colors.
	virtual void setColors(ConstDataBufferPtr colors) {
		OVITO_ASSERT(!colors || (colors->dataType() == DataBuffer::Float && colors->componentCount() == 3));
		_colors = std::move(colors);
	}

	/// Sets the transparency values of the primitives.
	virtual void setTransparencies(ConstDataBufferPtr transparencies) {
		OVITO_ASSERT(!transparencies || (transparencies->dataType() == DataBuffer::Float && transparencies->componentCount() == 1));
		_transparencies = std::move(transparencies);
	}

	/// Returns the buffer storing the transparancy values.
	const ConstDataBufferPtr& transparencies() const { return _transparencies; }

	/// Sets the radii of the primitives.
	virtual void setRadii(ConstDataBufferPtr radii) {
		OVITO_ASSERT(!radii || (radii->dataType() == DataBuffer::Float && radii->componentCount() == 1));
		_radii = std::move(radii);
	}

	/// Returns the buffer storing the per-primitive radius values.
	const ConstDataBufferPtr& radii() const { return _radii; }

private:

	/// Controls the shading.
	ShadingMode _shadingMode;

	/// Controls the rendering quality.
	RenderingQuality _renderingQuality;

	/// The shape of the elements.
	Shape _shape;

	/// The color to be used if no per-primitive colors have been specified.
	Color _uniformColor{1,1,1};

	/// The radius of the cylinders.
	FloatType _uniformRadius{1.0};

	/// Buffer storing the coordinates of the arrow/cylinder base points.
	ConstDataBufferPtr _basePositions; // Array of Point3

	/// Buffer storing the coordinates of the arrow/cylinder head points.
	ConstDataBufferPtr _headPositions; // Array of Point3

	/// Buffer storing the colors of the arrows/cylinders.
	ConstDataBufferPtr _colors; // Array of Color

	/// Buffer storing the semi-transparency values.
	ConstDataBufferPtr _transparencies; // Array of FloatType	

	/// Buffer storing the per-primitive radius values.
	ConstDataBufferPtr _radii; // Array of FloatType	
};

}	// End of namespace

Q_DECLARE_METATYPE(Ovito::CylinderPrimitive::ShadingMode);
Q_DECLARE_METATYPE(Ovito::CylinderPrimitive::RenderingQuality);
Q_DECLARE_METATYPE(Ovito::CylinderPrimitive::Shape);
Q_DECLARE_TYPEINFO(Ovito::CylinderPrimitive::ShadingMode, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::CylinderPrimitive::RenderingQuality, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::CylinderPrimitive::Shape, Q_PRIMITIVE_TYPE);
