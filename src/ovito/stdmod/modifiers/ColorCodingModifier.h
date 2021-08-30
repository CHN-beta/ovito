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


#include <ovito/stdmod/StdMod.h>
#include <ovito/stdobj/properties/PropertyReference.h>
#include <ovito/stdobj/properties/PropertyContainer.h>
#include <ovito/core/dataset/pipeline/DelegatingModifier.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "ColormapsData.h"

namespace Ovito { namespace StdMod {

/**
 * \brief Abstract base class for color gradients that can be used with a ColorCodingModifier.
 *
 * Implementations of this class convert a scalar value in the range [0,1] to a color value.
 */
class OVITO_STDMOD_EXPORT ColorCodingGradient : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ColorCodingGradient)

protected:

	/// Constructor.
	using RefTarget::RefTarget;

public:

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	Q_INVOKABLE virtual Color valueToColor(FloatType t) = 0;
};

/**
 * \brief Converts a scalar value to a color using the HSV color system.
 */
class OVITO_STDMOD_EXPORT ColorCodingHSVGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingHSVGradient)
	Q_CLASSINFO("DisplayName", "Rainbow");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingHSVGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color::fromHSV((FloatType(1) - t) * FloatType(0.7), 1, 1); }
};

/**
 * \brief Converts a scalar value to a color using a gray-scale ramp.
 */
class OVITO_STDMOD_EXPORT ColorCodingGrayscaleGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingGrayscaleGradient)
	Q_CLASSINFO("DisplayName", "Grayscale");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingGrayscaleGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color(t, t, t); }
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingHotGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingHotGradient)
	Q_CLASSINFO("DisplayName", "Hot");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingHotGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		// Interpolation black->red->yellow->white.
		OVITO_ASSERT(t >= 0 && t <= 1);
		return Color(std::min(t / FloatType(0.375), FloatType(1)), std::max(FloatType(0), std::min((t-FloatType(0.375))/FloatType(0.375), FloatType(1))), std::max(FloatType(0), t*4 - FloatType(3)));
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingJetGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingJetGradient)
	Q_CLASSINFO("DisplayName", "Jet");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingJetGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
	    if(t < FloatType(0.125)) return Color(0, 0, FloatType(0.5) + FloatType(0.5) * t / FloatType(0.125));
	    else if(t < FloatType(0.125) + FloatType(0.25)) return Color(0, (t - FloatType(0.125)) / FloatType(0.25), 1);
	    else if(t < FloatType(0.125) + FloatType(0.25) + FloatType(0.25)) return Color((t - FloatType(0.375)) / FloatType(0.25), 1, FloatType(1) - (t - FloatType(0.375)) / FloatType(0.25));
	    else if(t < FloatType(0.125) + FloatType(0.25) + FloatType(0.25) + FloatType(0.25)) return Color(1, FloatType(1) - (t - FloatType(0.625)) / FloatType(0.25), 0);
	    else return Color(FloatType(1) - FloatType(0.5) * (t - FloatType(0.875)) / FloatType(0.125), 0, 0);
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingBlueWhiteRedGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingBlueWhiteRedGradient)
	Q_CLASSINFO("DisplayName", "Blue-White-Red");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingBlueWhiteRedGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		if(t <= FloatType(0.5))
			return Color(t * 2, t * 2, 1);
		else
			return Color(1, (FloatType(1)-t) * 2, (FloatType(1)-t) * 2);
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingViridisGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingViridisGradient)
	Q_CLASSINFO("DisplayName", "Viridis");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingViridisGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		int index = t * (sizeof(colormap_viridis_data)/sizeof(colormap_viridis_data[0]) - 1);
		OVITO_ASSERT(t >= 0 && t < sizeof(colormap_viridis_data)/sizeof(colormap_viridis_data[0]));
		return Color(colormap_viridis_data[index][0], colormap_viridis_data[index][1], colormap_viridis_data[index][2]);
	}
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingMagmaGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingMagmaGradient)
	Q_CLASSINFO("DisplayName", "Magma");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingMagmaGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		int index = t * (sizeof(colormap_magma_data)/sizeof(colormap_magma_data[0]) - 1);
		OVITO_ASSERT(t >= 0 && t < sizeof(colormap_magma_data)/sizeof(colormap_magma_data[0]));
		return Color(colormap_magma_data[index][0], colormap_magma_data[index][1], colormap_magma_data[index][2]);
	}
};

/**
 * \brief Uses a color table to convert scalar values to a color.
 */
class OVITO_STDMOD_EXPORT ColorCodingTableGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingTableGradient)
	Q_CLASSINFO("DisplayName", "User table");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingTableGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override;

private:

	/// The user-defined color table.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(std::vector<Color>, table, setTable);
};

/**
 * \brief Converts a scalar value to a color based on a user-defined image.
 */
class OVITO_STDMOD_EXPORT ColorCodingImageGradient : public ColorCodingGradient
{
	OVITO_CLASS(ColorCodingImageGradient)
	Q_CLASSINFO("DisplayName", "User image");
	Q_OBJECT

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingImageGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override;

	/// Loads the given image file from disk.
	void loadImage(const QString& filename);

private:

	/// The user-defined color map image.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QImage, image, setImage);
};

/**
 * \brief Base class for ColorCodingModifier delegates that operate on different kinds of data.
 */
class OVITO_STDMOD_EXPORT ColorCodingModifierDelegate : public ModifierDelegate
{
	OVITO_CLASS(ColorCodingModifierDelegate)
	Q_OBJECT

#ifdef OVITO_QML_GUI
	Q_PROPERTY(Ovito::DataObjectReference inputContainerRef READ inputContainerRef NOTIFY propertyValueChangedSignal)
#endif

public:

	/// Applies the modifier operation to the data in a pipeline flow state.
	virtual PipelineStatus apply(Modifier* modifier, PipelineFlowState& state, TimePoint time, ModifierApplication* modApp, const std::vector<std::reference_wrapper<const PipelineFlowState>>& additionalInputs) override;

	/// Returns the type of input property container that this delegate can process.
	PropertyContainerClassPtr inputContainerClass() const {
		return static_class_cast<PropertyContainer>(&getOOMetaClass().getApplicableObjectClass());
	}

	/// Returns the reference to the selected input property container for this delegate.
	PropertyContainerReference inputContainerRef() const {
		return PropertyContainerReference(inputContainerClass(), inputDataObject().dataPath(), inputDataObject().dataTitle());
	}

protected:

	/// Abstract class constructor.
	using ModifierDelegate::ModifierDelegate;

	/// Returns the ID of the standard property that will receive the computed colors.
	virtual int outputColorPropertyId() const = 0;
};

/**
 * \brief This modifier assigns a colors to data elements based on the value of a property.
 */
class OVITO_STDMOD_EXPORT ColorCodingModifier : public DelegatingModifier
{
public:

	/// Give this modifier class its own metaclass.
	class ColorCodingModifierClass : public DelegatingModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base class.
		using DelegatingModifier::OOMetaClass::OOMetaClass;

		/// Return the metaclass of delegates for this modifier type.
		virtual const ModifierDelegate::OOMetaClass& delegateMetaclass() const override { return ColorCodingModifierDelegate::OOClass(); }
	};

	OVITO_CLASS_META(ColorCodingModifier, ColorCodingModifierClass)
	Q_CLASSINFO("DisplayName", "Color coding");
	Q_CLASSINFO("Description", "Colors elements based on property values.");
	Q_CLASSINFO("ModifierCategory", "Coloring");
	Q_OBJECT

#ifdef OVITO_QML_GUI
	Q_PROPERTY(Ovito::StdMod::ColorCodingGradient* colorGradient READ colorGradient WRITE setColorGradient NOTIFY referenceReplacedSignal)
	Q_PROPERTY(QString colorGradientType READ colorGradientType WRITE setColorGradientType NOTIFY referenceReplacedSignal)
#endif

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingModifier(DataSet* dataset);

	/// Loads the user-defined default values of this object's parameter fields from the
	/// application's settings store.
	virtual void initializeObject(ExecutionContext executionContext) override;

	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const PipelineEvaluationRequest& request, const ModifierApplication* modApp) const override;

	/// Returns the range start value.
	FloatType startValue() const { return startValueController() ? startValueController()->currentFloatValue() : 0; }

	/// Sets the range start value.
	void setStartValue(FloatType value) { if(startValueController()) startValueController()->setCurrentFloatValue(value); }

	/// Returns the range end value.
	FloatType endValue() const { return endValueController() ? endValueController()->currentFloatValue() : 0; }

	/// Sets the range end value.
	void setEndValue(FloatType value) { if(endValueController()) endValueController()->setCurrentFloatValue(value); }

	/// Sets the start and end value to the minimum and maximum value of the selected input property
	/// determined over the entire animation sequence.
	bool adjustRangeGlobal(Promise<>&& operation);

	/// Returns the current delegate of this modifier.
	ColorCodingModifierDelegate* delegate() const {
		return static_object_cast<ColorCodingModifierDelegate>(DelegatingModifier::delegate());
	}

#ifdef OVITO_QML_GUI
	/// Returns the class name of the selected color gradient.
	QString colorGradientType() const;

	/// Assigns a new color gradient based on its class name.
	void setColorGradientType(const QString& typeName, ExecutionContext executionContext = ExecutionContext::Interactive);
#endif

public Q_SLOTS:

	/// Sets the start and end value to the minimum and maximum value of the selected input property.
	/// Returns true if successful.
	bool adjustRange();

	/// Swaps the minimum and maximum values to reverse the color scale.
	void reverseRange();

protected:

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(TimePoint time, ModifierApplication* modApp, ExecutionContext executionContext) override;

	/// Determines the range of values in the input data for the selected property.
	bool determinePropertyValueRange(const PipelineFlowState& state, FloatType& min, FloatType& max) const;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex) override;

private:

	/// This controller stores the start value of the color scale.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<Controller>, startValueController, setStartValueController);

	/// This controller stores the end value of the color scale.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<Controller>, endValueController, setEndValueController);

	/// This object converts property values to colors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<ColorCodingGradient>, colorGradient, setColorGradient);

	/// The input property that is used as data source for the coloring.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(PropertyReference, sourceProperty, setSourceProperty);

	/// Controls whether the modifier assigns a color only to selected elements.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, colorOnlySelected, setColorOnlySelected);

	/// Controls whether the input selection is preserved. If false, the selection is cleared by the modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, keepSelection, setKeepSelection);

	/// Controls whether the value range of the color map is automatically adjusted to the range of input values.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, autoAdjustRange, setAutoAdjustRange);

	friend class ColorCodingModifierDelegate;
};

}	// End of namespace
}	// End of namespace
