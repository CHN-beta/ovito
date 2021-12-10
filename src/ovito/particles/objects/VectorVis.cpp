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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/CylinderPrimitive.h>
#include "VectorVis.h"
#include "ParticlesVis.h"

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(VectorVis);
IMPLEMENT_OVITO_CLASS(VectorPickInfo);
DEFINE_REFERENCE_FIELD(VectorVis, transparencyController);
DEFINE_REFERENCE_FIELD(VectorVis, colorMapping);
SET_PROPERTY_FIELD_LABEL(VectorVis, arrowColor, "Arrow color");
SET_PROPERTY_FIELD_LABEL(VectorVis, arrowWidth, "Arrow width");
SET_PROPERTY_FIELD_LABEL(VectorVis, scalingFactor, "Scaling factor");
SET_PROPERTY_FIELD_LABEL(VectorVis, reverseArrowDirection, "Reverse direction");
SET_PROPERTY_FIELD_LABEL(VectorVis, arrowPosition, "Position");
SET_PROPERTY_FIELD_LABEL(VectorVis, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(VectorVis, renderingQuality, "RenderingQuality");
SET_PROPERTY_FIELD_LABEL(VectorVis, transparencyController, "Transparency");
SET_PROPERTY_FIELD_LABEL(VectorVis, offset, "Offset");
SET_PROPERTY_FIELD_LABEL(VectorVis, coloringMode, "Coloring mode");
SET_PROPERTY_FIELD_LABEL(VectorVis, colorMapping, "Color mapping");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VectorVis, arrowWidth, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VectorVis, scalingFactor, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(VectorVis, transparencyController, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS(VectorVis, offset, WorldParameterUnit);

/******************************************************************************
* Constructor.
******************************************************************************/
VectorVis::VectorVis(DataSet* dataset) : DataVis(dataset),
	_reverseArrowDirection(false),
	_arrowPosition(Base),
	_arrowColor(1, 1, 0),
	_arrowWidth(0.5),
	_scalingFactor(1),
	_shadingMode(FlatShading),
	_renderingQuality(CylinderPrimitive::LowQuality),
	_offset(Vector3::Zero()),
	_coloringMode(UniformColoring)
{
}

/******************************************************************************
* Initializes the object's parameter fields with default values and loads 
* user-defined default values from the application's settings store (GUI only).
******************************************************************************/
void VectorVis::initializeObject(ObjectInitializationHints hints)
{
	// Create animation controller for the transparency parameter.
	if(!transparencyController())
		setTransparencyController(ControllerManager::createFloatController(dataset(), hints));

	// Create a color mapping object for pseudo-color visualization of a particle property.
	if(!colorMapping())
		setColorMapping(OORef<PropertyColorMapping>::create(dataset(), hints));

	DataVis::initializeObject(hints);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 VectorVis::boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	if(path.size() < 2) return {};
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(path[path.size()-2]);
	if(!particles) return {};
	const PropertyObject* vectorProperty = dynamic_object_cast<PropertyObject>(path.back());
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	if(vectorProperty && (vectorProperty->dataType() != PropertyObject::Float || vectorProperty->componentCount() != 3))
		vectorProperty = nullptr;

	// The key type used for caching the computed bounding box:
	using CacheKey = RendererResourceKey<struct VectorVisBoundingBoxCache,
		ConstDataObjectRef,		// Vector property
		ConstDataObjectRef,		// Particle position property
		FloatType,				// Scaling factor
		FloatType,				// Arrow width
		Vector3					// Offset
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			vectorProperty,
			positionProperty,
			scalingFactor(),
			arrowWidth(),
			offset()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {
		// If not, recompute bounding box from particle data.
		bbox = arrowBoundingBox(vectorProperty, positionProperty);
	}
	return bbox;
}

/******************************************************************************
* Computes the bounding box of the arrows.
******************************************************************************/
Box3 VectorVis::arrowBoundingBox(const PropertyObject* vectorProperty, const PropertyObject* positionProperty) const
{
	if(!positionProperty || !vectorProperty)
		return Box3();

	OVITO_ASSERT(positionProperty->type() == ParticlesObject::PositionProperty);
	OVITO_ASSERT(vectorProperty->dataType() == PropertyObject::Float);
	OVITO_ASSERT(vectorProperty->componentCount() == 3);

	// Compute bounding box of particle positions (only those with non-zero vector).
	Box3 bbox;
	ConstPropertyAccess<Point3> positions(positionProperty);
	ConstPropertyAccess<Vector3> vectorData(vectorProperty);
	const Point3* p = positions.cbegin();
	for(const Vector3& v : vectorData) {
		if(v != Vector3::Zero())
			bbox.addPoint(*p);
		++p;
	}

	// Find largest vector magnitude.
	FloatType maxMagnitude = 0;
	for(const Vector3& v : vectorData) {
		FloatType m = v.squaredLength();
		if(m > maxMagnitude) maxMagnitude = m;
	}

	// Apply displacement offset.
	bbox.minc += offset();
	bbox.maxc += offset();

	// Enlarge the bounding box by the largest vector magnitude + padding.
	return bbox.padBox((sqrt(maxMagnitude) * std::abs(scalingFactor())) + arrowWidth());
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
PipelineStatus VectorVis::render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	PipelineStatus status;

	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, path, contextNode, flowState, validityInterval));
		return status;
	}

	// Get input data.
	if(path.size() < 2) return {};
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(path[path.size()-2]);
	if(!particles) return {};
	particles->verifyIntegrity();
	const PropertyObject* vectorProperty = dynamic_object_cast<PropertyObject>(path.back());
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	if(vectorProperty && (vectorProperty->dataType() != PropertyObject::Float || vectorProperty->componentCount() != 3))
		vectorProperty = nullptr;
	const PropertyObject* vectorColorProperty = particles->getProperty(ParticlesObject::VectorColorProperty);

	// Make sure we don't exceed our internal limits.
	if(vectorProperty && vectorProperty->size() > (size_t)std::numeric_limits<int>::max()) {
		throw RenderException(tr("This version of OVITO cannot render more than %1 vector arrows.").arg(std::numeric_limits<int>::max()));
	}

	// Look for selected pseudo-coloring property.
	const PropertyObject* pseudoColorProperty = nullptr;
	int pseudoColorPropertyComponent = 0;
	PseudoColorMapping pseudoColorMapping;
	if(coloringMode() == PseudoColoring && colorMapping() && colorMapping()->sourceProperty() && !vectorColorProperty) {
		pseudoColorProperty = colorMapping()->sourceProperty().findInContainer(particles);
		if(!pseudoColorProperty) {
			status = PipelineStatus(PipelineStatus::Error, tr("The particle property with the name '%1' does not exist.").arg(colorMapping()->sourceProperty().name()));
		}
		else {
			if(colorMapping()->sourceProperty().vectorComponent() >= (int)pseudoColorProperty->componentCount()) {
				status = PipelineStatus(PipelineStatus::Error, tr("The vector component is out of range. The particle property '%1' has only %2 values per data element.").arg(colorMapping()->sourceProperty().name()).arg(pseudoColorProperty->componentCount()));
				pseudoColorProperty = nullptr;
			}
			pseudoColorPropertyComponent = std::max(0, colorMapping()->sourceProperty().vectorComponent());
			pseudoColorMapping = colorMapping()->pseudoColorMapping();
		}
	}

	// The key type used for caching the rendering primitive:
	using CacheKey = RendererResourceKey<struct VectorVisCache,
		ConstDataObjectRef,		// Vector property
		ConstDataObjectRef,		// Particle position property
		ShadingMode,			// Arrow shading mode
		CylinderPrimitive::RenderingQuality,	// Arrow rendering quality
		FloatType,				// Scaling factor
		FloatType,				// Arrow width
		Color,					// Arrow color
		FloatType,				// Arrow transparency
		bool,					// Reverse arrow direction
		ArrowPosition,			// Arrow position
		ConstDataObjectRef,		// Vector color property
		ConstDataObjectRef,		// Pseudo-color property
		int,					// Pseudo-color vector component
		PseudoColorMapping		// Pseudo-color mapping
	>;

	// Determine effective color including alpha value.
	FloatType transparency = 0;
	TimeInterval iv;
	if(transparencyController()) 
		transparency = transparencyController()->getFloatValue(time, iv);

	// Lookup the rendering primitive in the vis cache.
	auto& arrows = dataset()->visCache().get<CylinderPrimitive>(CacheKey(
			vectorProperty,
			positionProperty,
			shadingMode(),
			renderingQuality(),
			scalingFactor(),
			arrowWidth(),
			arrowColor(),
			transparency,
			reverseArrowDirection(),
			arrowPosition(),
			vectorColorProperty,
			pseudoColorProperty, 
			pseudoColorPropertyComponent,
			pseudoColorMapping));

	// Check if we already have a valid rendering primitive that is up to date.
	if(!arrows.basePositions()) {

		// Determine number of non-zero vectors.
		int vectorCount = 0;
		ConstPropertyAccess<Vector3> vectorData(vectorProperty);
		if(vectorProperty && positionProperty) {
			for(const Vector3& v : vectorData) {
				if(v != Vector3::Zero())
					vectorCount++;
			}
		}

		// Allocate data buffers.
		DataBufferAccessAndRef<Point3> arrowBasePositions = DataBufferPtr::create(dataset(), vectorCount, DataBuffer::Float, 3, 0, false);
		DataBufferAccessAndRef<Point3> arrowHeadPositions = DataBufferPtr::create(dataset(), vectorCount, DataBuffer::Float, 3, 0, false);
		DataBufferAccessAndRef<Color> arrowColors = (vectorColorProperty || pseudoColorProperty) ? DataBufferPtr::create(dataset(), vectorCount, DataBuffer::Float, 3, 0, false) : nullptr;

		// Fill data buffers.
		if(vectorCount) {
			FloatType scalingFac = scalingFactor();
			if(reverseArrowDirection())
				scalingFac = -scalingFac;
			ConstPropertyAccess<Point3> particlePositions(positionProperty);
			ConstPropertyAccess<Color> vectorColorData(vectorColorProperty);
			ConstPropertyAccess<void,true> vectorPseudoColorData(pseudoColorProperty);
			size_t inIndex = 0;
			size_t outIndex = 0;
			for(size_t inIndex = 0; inIndex < particlePositions.size(); inIndex++) {
				const Vector3& vec = vectorData[inIndex];
				if(vec != Vector3::Zero()) {
					Vector3 v = vec * scalingFac;
					Point3 base = particlePositions[inIndex];
					if(arrowPosition() == Head)
						base -= v;
					else if(arrowPosition() == Center)
						base -= v * FloatType(0.5);
					arrowBasePositions[outIndex] = base;
					arrowHeadPositions[outIndex] = base + v;
					if(vectorColorProperty)
						arrowColors[outIndex] = vectorColorData[inIndex];
					else if(pseudoColorProperty)
						arrowColors[outIndex] = pseudoColorMapping.valueToColor(vectorPseudoColorData.get<FloatType>(inIndex, pseudoColorPropertyComponent));
					outIndex++;
				}
			}
			OVITO_ASSERT(outIndex == vectorCount);
		}

		// Create arrow rendering primitive.
		arrows.setShape(CylinderPrimitive::ArrowShape);
		arrows.setShadingMode(static_cast<CylinderPrimitive::ShadingMode>(shadingMode()));
		arrows.setRenderingQuality(renderingQuality());
		arrows.setUniformRadius(arrowWidth());
		arrows.setUniformColor(arrowColor());
		arrows.setPositions(arrowBasePositions.take(), arrowHeadPositions.take());
		arrows.setColors(arrowColors.take());
		if(transparency > 0.0) {
			DataBufferPtr transparencyBuffer = DataBufferPtr::create(dataset(), vectorCount, DataBuffer::Float, 1, 0, false);
			transparencyBuffer->fill(transparency);
			arrows.setTransparencies(std::move(transparencyBuffer));
		}
	}

	if(renderer->isPicking()) {
		OORef<VectorPickInfo> pickInfo(new VectorPickInfo(this, particles, vectorProperty));
		renderer->beginPickObject(contextNode, pickInfo);
	}
	AffineTransformation oldTM = renderer->worldTransform();
	renderer->setWorldTransform(AffineTransformation::translation(offset()) * oldTM);
	renderer->renderCylinders(arrows);
	renderer->setWorldTransform(oldTM);
	if(renderer->isPicking()) {
		renderer->endPickObject();
	}

	return status;
}

/******************************************************************************
* Given an sub-object ID returned by the Viewport::pick() method, looks up the
* corresponding particle index.
******************************************************************************/
size_t VectorPickInfo::particleIndexFromSubObjectID(quint32 subobjID) const
{
	if(_vectorProperty) {
		size_t particleIndex = 0;
		ConstPropertyAccess<Vector3> vectorData(_vectorProperty);
		for(const Vector3& v : vectorData) {
			if(v != Vector3::Zero()) {
				if(subobjID == 0) return particleIndex;
				subobjID--;
			}
			particleIndex++;
		}
	}
	return std::numeric_limits<size_t>::max();
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString VectorPickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	size_t particleIndex = particleIndexFromSubObjectID(subobjectId);
	if(particleIndex == std::numeric_limits<size_t>::max()) 
		return QString();
	return ParticlePickInfo::particleInfoString(*particles(), particleIndex);
}

}	// End of namespace
