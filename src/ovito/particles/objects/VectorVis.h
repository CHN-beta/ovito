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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/properties/PropertyColorMapping.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/dataset/animation/controller/Controller.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/rendering/CylinderPrimitive.h>
#include <ovito/core/rendering/SceneRenderer.h>

namespace Ovito::Particles {

/**
 * \brief A visualization element for rendering per-particle vector arrows.
 */
class OVITO_PARTICLES_EXPORT VectorVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(VectorVis)
	Q_CLASSINFO("DisplayName", "Vectors");

public:

	/// The shading modes supported by the vector vis element.
	enum ShadingMode {
		NormalShading = CylinderPrimitive::ShadingMode::NormalShading,
		FlatShading = CylinderPrimitive::ShadingMode::FlatShading
	};
	Q_ENUM(ShadingMode);

	/// The position mode for the arrows.
	enum ArrowPosition {
		Base,
		Center,
		Head
	};
	Q_ENUM(ArrowPosition);

	/// The coloring modes supported by the vis element.
	enum ColoringMode {
		UniformColoring,
		PseudoColoring,
	};
	Q_ENUMS(ColoringMode);	

public:

	/// \brief Constructor.
	Q_INVOKABLE VectorVis(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ObjectInitializationHints hints) override;	

	/// \brief Lets the visualization element render the data object.
	virtual PipelineStatus render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the transparency parameter.
	FloatType transparency() const { return transparencyController()->currentFloatValue(); }

	/// Sets the transparency parameter.
	void setTransparency(FloatType t) { transparencyController()->setCurrentFloatValue(t); }

public:

    Q_PROPERTY(Ovito::Particles::VectorVis::ShadingMode shadingMode READ shadingMode WRITE setShadingMode)
    Q_PROPERTY(Ovito::CylinderPrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality)

protected:

	/// Computes the bounding box of the arrows.
	Box3 arrowBoundingBox(const PropertyObject* vectorProperty, const PropertyObject* positionProperty) const;

protected:

	/// Reverses of the arrow pointing direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, reverseArrowDirection, setReverseArrowDirection);
	DECLARE_SHADOW_PROPERTY_FIELD(reverseArrowDirection);

	/// Controls how the arrows are positioned relative to the particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(ArrowPosition, arrowPosition, setArrowPosition, PROPERTY_FIELD_MEMORIZE);
	DECLARE_SHADOW_PROPERTY_FIELD(arrowPosition);

	/// The uniform display color of the arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Color, arrowColor, setArrowColor, PROPERTY_FIELD_MEMORIZE);
	DECLARE_SHADOW_PROPERTY_FIELD(arrowColor);

	/// The width of the arrows in world units.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, arrowWidth, setArrowWidth, PROPERTY_FIELD_MEMORIZE);
	DECLARE_SHADOW_PROPERTY_FIELD(arrowWidth);

	/// The scaling factor applied to the vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, scalingFactor, setScalingFactor, PROPERTY_FIELD_MEMORIZE);
	DECLARE_SHADOW_PROPERTY_FIELD(scalingFactor);

	/// The shading mode for arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(VectorVis::ShadingMode, shadingMode, setShadingMode, PROPERTY_FIELD_MEMORIZE);
	DECLARE_SHADOW_PROPERTY_FIELD(shadingMode);

	/// The rendering quality mode for arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(CylinderPrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// The transparency value of the arrows.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<Controller>, transparencyController, setTransparencyController);

	/// Displacement offset to be applied to all arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, offset, setOffset);

	/// Determines how the arrows are colored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(VectorVis::ColoringMode, coloringMode, setColoringMode);

	/// Transfer function for pseudo-color visualization of a particle property.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(OORef<PropertyColorMapping>, colorMapping, setColorMapping);
};

/**
 * \brief This information record is attached to the arrows by the VectorVis when rendering
 * them in the viewports. It facilitates the picking of arrows with the mouse.
 */
class OVITO_PARTICLES_EXPORT VectorPickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(VectorPickInfo)

public:

	/// Constructor.
	VectorPickInfo(VectorVis* visElement, DataOORef<const ParticlesObject> particles, ConstPropertyPtr vectorProperty) :
		_visElement(visElement), _particles(std::move(particles)), _vectorProperty(std::move(vectorProperty)) {}

	/// Returns the particles object.
	const DataOORef<const ParticlesObject>& particles() const { OVITO_ASSERT(_particles); return _particles; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

	/// Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding particle index.
	size_t particleIndexFromSubObjectID(quint32 subobjID) const;

private:

	/// The vis element that rendered the arrows.
	OORef<VectorVis> _visElement;

	/// The particles object.
	DataOORef<const ParticlesObject> _particles;

	/// The vector property.
	ConstPropertyPtr _vectorProperty;
};

}	// End of namespace
