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


#include <ovito/core/Core.h>
#include <ovito/core/rendering/ColorCodingGradient.h>

namespace Ovito {

/**
 * \brief Transfer function that defines the mapping from scalar pseudo-color values to RGB values.
 */
class OVITO_CORE_EXPORT PseudoColorMapping
{
public:

	/// Default constructor.
	PseudoColorMapping() = default;

	/// Constructor.
	PseudoColorMapping(FloatType minValue, FloatType maxValue, OORef<ColorCodingGradient> gradient) : _minValue(minValue), _maxValue(maxValue), _gradient(std::move(gradient)) {
		OVITO_ASSERT(_gradient);
	}

	/// Returns true if this is not the null mapping.
	bool isValid() const { return (bool)_gradient && std::isfinite(_minValue) && std::isfinite(_maxValue); }

	/// Returns the lower bound of the mapping interval.
	FloatType minValue() const { return _minValue; }

	/// Returns the upper bound of the mapping interval.
	FloatType maxValue() const { return _maxValue; }

	/// Returns the color gradient.
	const OORef<ColorCodingGradient>& gradient() const { return _gradient; }

	/// Converts a scalar value to an RGB color value.
	Color valueToColor(FloatType v) const {
		OVITO_ASSERT(isValid());
		OVITO_ASSERT(std::isfinite(v));
		// Handle a degenerate mapping interval.
		if(_maxValue == _minValue) {
			if(v == _maxValue) return gradient()->valueToColor(FloatType(0.5));
			else if(v < _maxValue) return gradient()->valueToColor(FloatType(0));
			else return gradient()->valueToColor(FloatType(1));
		}
		// Compute linear interpolation.
		FloatType t = (v - _minValue) / (_maxValue - _minValue);
		// Clamp values.
		if(std::isnan(t)) t = FloatType(0);
		else if(t ==  std::numeric_limits<FloatType>::infinity()) t = FloatType(1);
		else if(t == -std::numeric_limits<FloatType>::infinity()) t = FloatType(0);
		else if(t < FloatType(0)) t = FloatType(0);
		else if(t > FloatType(1)) t = FloatType(1);
		return gradient()->valueToColor(t);
	}

private:

	/// The lower bound of the mapping interval.
	FloatType _minValue = 0;

	/// The upper bound of the mapping interval.
	FloatType _maxValue = 0;

	/// The color gradient.
	OORef<ColorCodingGradient> _gradient;
};

}	// End of namespace
