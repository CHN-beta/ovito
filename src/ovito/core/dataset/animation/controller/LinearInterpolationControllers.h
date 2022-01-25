////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
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
#include "KeyframeController.h"

namespace Ovito {

/**
 * \brief Implementation of the key interpolator concept that performs linear interpolation.
 *
 * This class is used with the linear interpolation controllers.
 */
template<typename KeyType>
struct LinearKeyInterpolator {
	typename KeyType::value_type operator()(TimePoint time, KeyType* key0, KeyType* key1, KeyType* key2, KeyType* key3) const {
		OVITO_ASSERT(key2->time() > key1->time());
		FloatType t = (FloatType)(time - key1->time()) / (key2->time() - key1->time());
		LinearValueInterpolator<typename KeyType::value_type> valueInterpolator;
		return valueInterpolator(t, key1->value(), key2->value());
	}
};

/**
 * \brief A keyframe controller that interpolates between float values using a linear interpolation scheme.
 */
class OVITO_CORE_EXPORT LinearFloatController
	: public KeyframeControllerTemplate<FloatAnimationKey,
	  	  	  	  	  	  	  	  	  	LinearKeyInterpolator<FloatAnimationKey>,
	  	  	  	  	  	  	  	  	  	Controller::ControllerTypeFloat>
{
	OVITO_CLASS(LinearFloatController)

public:

	/// Constructor.
	Q_INVOKABLE LinearFloatController(ObjectCreationParams params)
		: KeyframeControllerTemplate<FloatAnimationKey, LinearKeyInterpolator<FloatAnimationKey>, Controller::ControllerTypeFloat>(params) {}

	/// \brief Gets the controller's value at a certain animation time.
	virtual FloatType getFloatValue(TimePoint time, TimeInterval& validityInterval) override {
		FloatType val;
		getInterpolatedValue(time, val, validityInterval);
		return val;
	}

	/// \brief Sets the controller's value at the given animation time.
	virtual void setFloatValue(TimePoint time, FloatType newValue) override {
		setAbsoluteValue(time, newValue);
	}
};

/**
 * \brief A keyframe controller that interpolates between integer values using a linear interpolation scheme.
 */
class OVITO_CORE_EXPORT LinearIntegerController
	: public KeyframeControllerTemplate<IntegerAnimationKey,
	  	  	  	  	  	  	  	  	  	LinearKeyInterpolator<IntegerAnimationKey>,
	  	  	  	  	  	  	  	  	  	Controller::ControllerTypeInt>
{
	OVITO_CLASS(LinearIntegerController)

public:

	/// Constructor.
	Q_INVOKABLE LinearIntegerController(ObjectCreationParams params)
		: KeyframeControllerTemplate<IntegerAnimationKey, LinearKeyInterpolator<IntegerAnimationKey>, Controller::ControllerTypeInt>(params) {}

	/// \brief Gets the controller's value at a certain animation time.
	virtual int getIntValue(TimePoint time, TimeInterval& validityInterval) override {
		int val;
		getInterpolatedValue(time, val, validityInterval);
		return val;
	}

	/// \brief Sets the controller's value at the given animation time.
	virtual void setIntValue(TimePoint time, int newValue) override {
		setAbsoluteValue(time, newValue);
	}
};

/**
 * \brief A keyframe controller that interpolates between Vector3 values using a linear interpolation scheme.
 */
class OVITO_CORE_EXPORT LinearVectorController
	: public KeyframeControllerTemplate<Vector3AnimationKey,
	  	  	  	  	  	  	  	  	  	LinearKeyInterpolator<Vector3AnimationKey>,
	  	  	  	  	  	  	  	  	  	Controller::ControllerTypeVector3>
{
	OVITO_CLASS(LinearVectorController)

public:

	/// Constructor.
	Q_INVOKABLE LinearVectorController(ObjectCreationParams params)
		: KeyframeControllerTemplate<Vector3AnimationKey, LinearKeyInterpolator<Vector3AnimationKey>, Controller::ControllerTypeVector3>(params) {}

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getVector3Value(TimePoint time, Vector3& value, TimeInterval& validityInterval) override {
		getInterpolatedValue(time, value, validityInterval);
	}

	/// \brief Sets the controller's value at the given animation time.
	virtual void setVector3Value(TimePoint time, const Vector3& newValue) override {
		setAbsoluteValue(time, newValue);
	}
};

/**
 * \brief A keyframe controller that interpolates between position values using a linear interpolation scheme.
 */
class OVITO_CORE_EXPORT LinearPositionController
	: public KeyframeControllerTemplate<PositionAnimationKey,
	  	  	  	  	  	  	  	  	  	LinearKeyInterpolator<PositionAnimationKey>,
	  	  	  	  	  	  	  	  	  	Controller::ControllerTypePosition>
{
	OVITO_CLASS(LinearPositionController)

public:

	/// Constructor.
	Q_INVOKABLE LinearPositionController(ObjectCreationParams params)
		: KeyframeControllerTemplate<PositionAnimationKey, LinearKeyInterpolator<PositionAnimationKey>, Controller::ControllerTypePosition>(params) {}

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getPositionValue(TimePoint time, Vector3& value, TimeInterval& validityInterval) override {
		getInterpolatedValue(time, value, validityInterval);
	}

	/// \brief Sets the controller's value at the given animation time.
	virtual void setPositionValue(TimePoint time, const Vector3& newValue, bool isAbsolute) override {
		if(isAbsolute)
			setAbsoluteValue(time, newValue);
		else
			setRelativeValue(time, newValue);
	}
};

/**
 * \brief A keyframe controller that interpolates between rotation values using a linear interpolation scheme.
 */
class OVITO_CORE_EXPORT LinearRotationController
	: public KeyframeControllerTemplate<RotationAnimationKey,
	  	  	  	  	  	  	  	  	  	LinearKeyInterpolator<RotationAnimationKey>,
	  	  	  	  	  	  	  	  	  	Controller::ControllerTypeRotation>
{
	OVITO_CLASS(LinearRotationController)

public:

	/// Constructor.
	Q_INVOKABLE LinearRotationController(ObjectCreationParams params)
		: KeyframeControllerTemplate<RotationAnimationKey, LinearKeyInterpolator<RotationAnimationKey>, Controller::ControllerTypeRotation>(params) {}

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getRotationValue(TimePoint time, Rotation& value, TimeInterval& validityInterval) override {
		getInterpolatedValue(time, value, validityInterval);
	}

	/// \brief Sets the controller's value at the given animation time.
	virtual void setRotationValue(TimePoint time, const Rotation& newValue, bool isAbsolute) override {
		if(isAbsolute)
			setAbsoluteValue(time, newValue);
		else
			setRelativeValue(time, newValue);
	}
};

/**
 * \brief A keyframe controller that interpolates between scaling values using a linear interpolation scheme.
 */
class OVITO_CORE_EXPORT LinearScalingController
	: public KeyframeControllerTemplate<ScalingAnimationKey,
	  	  	  	  	  	  	  	  	  	LinearKeyInterpolator<ScalingAnimationKey>,
	  	  	  	  	  	  	  	  	  	Controller::ControllerTypeScaling>
{
	OVITO_CLASS(LinearScalingController)

public:

	/// Constructor.
	Q_INVOKABLE LinearScalingController(ObjectCreationParams params)
		: KeyframeControllerTemplate<ScalingAnimationKey, LinearKeyInterpolator<ScalingAnimationKey>, Controller::ControllerTypeScaling>(params) {}

	/// \brief Gets the controller's value at a certain animation time.
	virtual void getScalingValue(TimePoint time, Scaling& value, TimeInterval& validityInterval) override {
		getInterpolatedValue(time, value, validityInterval);
	}

	/// \brief Sets the controller's value at the given animation time.
	virtual void setScalingValue(TimePoint time, const Scaling& newValue, bool isAbsolute) override {
		if(isAbsolute)
			setAbsoluteValue(time, newValue);
		else
			setRelativeValue(time, newValue);
	}
};

}	// End of namespace
