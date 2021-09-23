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
#include "PrimitiveBase.h"

namespace Ovito {

/**
 * \brief A set of particles to be rendered by a SceneRenderer implementation.
 */
class OVITO_CORE_EXPORT ParticlePrimitive : public PrimitiveBase
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
		HighQuality,
		AutoQuality
	};
	Q_ENUMS(RenderingQuality);

	enum ParticleShape {
		SphericalShape,
		SquareCubicShape,
		BoxShape,
		EllipsoidShape,
		SuperquadricShape
	};
	Q_ENUMS(ParticleShape);

public:

	/// Constructor.
	ParticlePrimitive(ParticleShape shape, ShadingMode shadingMode, RenderingQuality renderingQuality)
		: _particleShape(shape), _shadingMode(shadingMode), _renderingQuality(renderingQuality) {
	}

	/// \brief Sets the array of particle indices to render.
	virtual void setIndices(ConstDataBufferPtr indices) {
		OVITO_ASSERT(!indices || (indices->dataType() == DataBuffer::Int && indices->componentCount() == 1));
		_indices = std::move(indices);
	}

	/// \brief Sets the coordinates of the particles.
	virtual void setPositions(ConstDataBufferPtr coordinates) {
		OVITO_ASSERT(coordinates);
		OVITO_ASSERT(coordinates->dataType() == DataBuffer::Float && coordinates->componentCount() == 3);
		_positions = std::move(coordinates);
	}

	/// \brief Sets the radii of the particles.
	virtual void setRadii(ConstDataBufferPtr radii) {
		OVITO_ASSERT(!radii || (radii->dataType() == DataBuffer::Float && radii->componentCount() == 1));
		_radii = std::move(radii);
	}

	/// \brief Sets the radius of all particles to the given value.
	virtual void setUniformRadius(FloatType radius) {
		_uniformParticleRadius = radius;
	}

	/// \brief Sets the colors of the particles.
	virtual void setColors(ConstDataBufferPtr colors) {
		OVITO_ASSERT(!colors || (colors->dataType() == DataBuffer::Float && colors->componentCount() == 3));
		_colors = std::move(colors);
	}

	/// \brief Sets the color of all particles to the given value.
	virtual void setUniformColor(const Color& color) {
		_uniformParticleColor = color;
	}

	/// \brief Sets the selection flags of the particles.
	virtual void setSelection(ConstDataBufferPtr selection) {
		OVITO_ASSERT(!selection || (selection->dataType() == DataBuffer::Int && selection->componentCount() == 1));
		_selection = std::move(selection);
	}

	/// \brief Sets the color to be used for rendering the selected particles.
	void setSelectionColor(const Color& color) {
		_selectionParticleColor = color;
	}

	/// \brief Sets the transparency values of the particles.
	virtual void setTransparencies(ConstDataBufferPtr transparencies) {
		OVITO_ASSERT(!transparencies || (transparencies->dataType() == DataBuffer::Float && transparencies->componentCount() == 1));
		_transparencies = std::move(transparencies);
	}

	/// \brief Sets the aspherical shape of the particles.
	virtual void setAsphericalShapes(ConstDataBufferPtr shapes) {
		OVITO_ASSERT(!shapes || (shapes->dataType() == DataBuffer::Float && shapes->componentCount() == 3));
		_asphericalShapes = std::move(shapes);		
	}

	/// \brief Sets the aspherical shape of the particles.
	virtual void setOrientations(ConstDataBufferPtr orientations) {
		OVITO_ASSERT(!orientations || (orientations->dataType() == DataBuffer::Float && orientations->componentCount() == 4));
		_orientations = std::move(orientations);		
	}

	/// \brief Sets the superquadric roundness values of the particles.
	virtual void setRoundness(ConstDataBufferPtr roundness) {
		OVITO_ASSERT(!roundness || (roundness->dataType() == DataBuffer::Float && roundness->componentCount() == 2));
		_roundness = std::move(roundness);
	}

	/// \brief Returns the shading mode for particles.
	ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Returns the rendering quality of particles.
	RenderingQuality renderingQuality() const { return _renderingQuality; }

	/// \brief Returns the display shape of particles.
	ParticleShape particleShape() const { return _particleShape; }

	/// Returns the buffer storing the array of particle indices to render.
	const ConstDataBufferPtr& indices() const { return _indices; }

	/// Returns the buffer storing the particle positions.
	const ConstDataBufferPtr& positions() const { return _positions; }

	/// Returns the buffer storing the particle radii.
	const ConstDataBufferPtr& radii() const { return _radii; }

	/// Returns the buffer storing the particle colors.
	const ConstDataBufferPtr& colors() const { return _colors; }

	/// Returns the buffer storing the particle selection flags.
	const ConstDataBufferPtr& selection() const { return _selection; }

	/// Returns the buffer storing the particle transparancy values.
	const ConstDataBufferPtr& transparencies() const { return _transparencies; }

	/// Returns the buffer stroring the shapes of aspherical particles.
	const ConstDataBufferPtr& asphericalShapes() const { return _asphericalShapes; }

	/// Returns the buffer storing the orientations of aspherical particles.
	const ConstDataBufferPtr& orientations() const { return _orientations; }

	/// Returns the buffer storing the roundness values of superquadric particles.
	const ConstDataBufferPtr& roundness() const { return _roundness; }

	/// Returns the radius assigned to all particles.
	FloatType uniformRadius() const { return _uniformParticleRadius; }

	/// Returns the color assigned to all particles.
	const Color& uniformColor() const { return _uniformParticleColor; }

	/// Returns the color used for rendering all selected particles.
	const Color& selectionColor() const { return _selectionParticleColor; }

private:

	/// Controls the shading of particles.
	ShadingMode _shadingMode;

	/// Controls the rendering quality.
	RenderingQuality _renderingQuality;

	/// Selects the type of visual shape of the rendered particles.
	ParticleShape _particleShape;

	/// The internal buffer storing the array of particle indices to render.
	ConstDataBufferPtr _indices; // Array of int

	/// The internal buffer storing the array of particle coordinates.
	ConstDataBufferPtr _positions; // Array of Vector3

	/// The internal buffer stores the array of particle radii.
	ConstDataBufferPtr _radii; // Array of FloatType

	/// The internal buffer storing the particle RGB colors.
	ConstDataBufferPtr _colors; // Array of Color

	/// The internal buffer storing the array of particle selection flags.
	ConstDataBufferPtr _selection; // Array of int

	/// The internal buffer storing the particle semi-transparency values.
	ConstDataBufferPtr _transparencies; // Array of FloatType	

	/// The internal buffer storing the shapes of aspherical particles.
	ConstDataBufferPtr _asphericalShapes; // Array of Vector3

	/// The internal buffer storing the orientations of aspherical particles.
	ConstDataBufferPtr _orientations; // Array of Quaternion

	/// The internal buffer storing the roundness values of superquadric particles.
	ConstDataBufferPtr _roundness; // Array of Vector2

	/// The radius value to be used if no per-particle radii have been specified.
	FloatType _uniformParticleRadius = 0;

	/// The color to be used if no per-particle colors have been specified.
	Color _uniformParticleColor{1,1,1};

	/// The color used for rendering all selected particles.
	Color _selectionParticleColor{1,0,0};
};

}	// End of namespace

Q_DECLARE_METATYPE(Ovito::ParticlePrimitive::ShadingMode);
Q_DECLARE_METATYPE(Ovito::ParticlePrimitive::RenderingQuality);
Q_DECLARE_METATYPE(Ovito::ParticlePrimitive::ParticleShape);
Q_DECLARE_TYPEINFO(Ovito::ParticlePrimitive::ShadingMode, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ParticlePrimitive::RenderingQuality, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::ParticlePrimitive::ParticleShape, Q_PRIMITIVE_TYPE);
