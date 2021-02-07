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
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/data/DataObjectAccess.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/rendering/CylinderPrimitive.h>
#include "ParticlesVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticlesVis);
IMPLEMENT_OVITO_CLASS(ParticlePickInfo);
DEFINE_PROPERTY_FIELD(ParticlesVis, defaultParticleRadius);
DEFINE_PROPERTY_FIELD(ParticlesVis, radiusScaleFactor);
DEFINE_PROPERTY_FIELD(ParticlesVis, renderingQuality);
DEFINE_PROPERTY_FIELD(ParticlesVis, particleShape);
SET_PROPERTY_FIELD_LABEL(ParticlesVis, defaultParticleRadius, "Standard radius");
SET_PROPERTY_FIELD_LABEL(ParticlesVis, radiusScaleFactor, "Radius scaling factor");
SET_PROPERTY_FIELD_LABEL(ParticlesVis, renderingQuality, "Rendering quality");
SET_PROPERTY_FIELD_LABEL(ParticlesVis, particleShape, "Standard shape");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticlesVis, defaultParticleRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticlesVis, radiusScaleFactor, PercentParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
ParticlesVis::ParticlesVis(DataSet* dataset) : DataVis(dataset),
	_defaultParticleRadius(1.2),
	_radiusScaleFactor(1.0),
	_renderingQuality(ParticlePrimitive::AutoQuality),
	_particleShape(Sphere)
{
}

/******************************************************************************
* Computes the bounding box of the visual element.
******************************************************************************/
Box3 ParticlesVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack.back());
	if(!particles) return {};
	particles->verifyIntegrity();
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* shapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);

	// The key type used for caching the computed bounding box:
	using CacheKey = std::tuple<
		ConstDataObjectRef,	// Position property
		ConstDataObjectRef,	// Radius property
		ConstDataObjectRef,	// Type property
		ConstDataObjectRef,	// Aspherical shape property
		FloatType 			// Default particle radius
	>;

	// Look up the bounding box in the vis cache.
	auto& bbox = dataset()->visCache().get<Box3>(CacheKey(
			positionProperty,
			radiusProperty,
			typeProperty,
			shapeProperty,
			defaultParticleRadius()));

	// Check if the cached bounding box information is still up to date.
	if(bbox.isEmpty()) {
		// If not, recompute bounding box from particle data.
		bbox = particleBoundingBox(positionProperty, typeProperty, radiusProperty, shapeProperty, true);
	}
	return bbox;
}

/******************************************************************************
* Computes the bounding box of the particles.
******************************************************************************/
Box3 ParticlesVis::particleBoundingBox(ConstPropertyAccess<Point3> positionProperty, const PropertyObject* typeProperty, ConstPropertyAccess<FloatType> radiusProperty, ConstPropertyAccess<Vector3> shapeProperty, bool includeParticleRadius) const
{
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticlesObject::TypeProperty);
	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder)
		shapeProperty = nullptr;

	Box3 bbox;
	if(positionProperty) {
		bbox.addPoints(positionProperty);
	}
	if(!includeParticleRadius)
		return bbox;

	// Check if any of the particle types have a user-defined mesh geometry assigned.
	std::vector<std::pair<int,FloatType>> userShapeParticleTypes;
	if(typeProperty) {
		for(const ElementType* etype : typeProperty->elementTypes()) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
				if(ptype->shapeMesh() && ptype->shapeMesh()->mesh() && ptype->shapeMesh()->mesh()->faceCount() != 0) {
					// Compute the maximum extent of the user-defined shape mesh.
					const Box3& bbox = ptype->shapeMesh()->mesh()->boundingBox();
					FloatType extent = std::max((bbox.minc - Point3::Origin()).length(), (bbox.maxc - Point3::Origin()).length());
					userShapeParticleTypes.emplace_back(ptype->numericId(), extent);
				}
			}
		}
	}

	// Extend box to account for radii/shape of particles.
	FloatType maxAtomRadius = 0;

	if(userShapeParticleTypes.empty()) {
		// Standard case - no user-defined particle shapes assigned:
		if(typeProperty) {
			for(const auto& it : ParticleType::typeRadiusMap(typeProperty)) {
				maxAtomRadius = std::max(maxAtomRadius, (it.second != 0) ? it.second : defaultParticleRadius());
			}
		}
		if(maxAtomRadius == 0)
			maxAtomRadius = defaultParticleRadius();
		if(shapeProperty) {
			for(const Vector3& s : shapeProperty)
				maxAtomRadius = std::max(maxAtomRadius, std::max(s.x(), std::max(s.y(), s.z())));
			if(particleShape() == Spherocylinder)
				maxAtomRadius *= 2;
		}
		if(radiusProperty && radiusProperty.size() != 0) {
			auto minmax = std::minmax_element(radiusProperty.cbegin(), radiusProperty.cend());
			if(*minmax.first <= 0)
				maxAtomRadius = std::max(maxAtomRadius, *minmax.second);
			else
				maxAtomRadius = *minmax.second;
		}
	}
	else {
		// Non-standard case - at least one user-defined particle shape assigned:
		std::map<int,FloatType> typeRadiusMap = ParticleType::typeRadiusMap(typeProperty);
		if(radiusProperty && radiusProperty.size() == typeProperty->size()) {
			const FloatType* r = radiusProperty.cbegin();
			ConstPropertyAccess<int> typeData(typeProperty);
			for(int t : typeData) {
				// Determine effective radius of the current particle.
				FloatType radius = *r++;
				if(radius <= 0) radius = typeRadiusMap[t];
				if(radius <= 0) radius = defaultParticleRadius();
				// Effective radius is multiplied with the extent of the user-defined shape mesh.
				bool foundMeshExtent = false;
				for(const auto& entry : userShapeParticleTypes) {
					if(entry.first == t) {
						maxAtomRadius = std::max(maxAtomRadius, radius * entry.second);
						foundMeshExtent = true;
						break;
					}
				}
				// If this particle type has no user-defined shape assigned, simply use radius.
				if(!foundMeshExtent)
					maxAtomRadius = std::max(maxAtomRadius, radius);
			}
		}
		else {
			for(const auto& it : typeRadiusMap) {
				FloatType typeRadius = (it.second != 0) ? it.second : defaultParticleRadius();
				bool foundMeshExtent = false;
				for(const auto& entry : userShapeParticleTypes) {
					if(entry.first == it.first) {
						maxAtomRadius = std::max(maxAtomRadius, typeRadius * entry.second);
						foundMeshExtent = true;
						break;
					}
				}
				// If this particle type has no user-defined shape assigned, simply use radius.
				if(!foundMeshExtent)
					maxAtomRadius = std::max(maxAtomRadius, typeRadius);
			}
		}
	}

	// Extend the bounding box by the largest particle radius.
	return bbox.padBox(std::max(radiusScaleFactor() * maxAtomRadius * sqrt(FloatType(3)), FloatType(0)));
}

/******************************************************************************
* Returns the typed particle property used to determine the rendering colors 
* of particles (if no per-particle colors are defined).
******************************************************************************/
const PropertyObject* ParticlesVis::getParticleTypeColorProperty(const ParticlesObject* particles) const
{
	return particles->getProperty(ParticlesObject::TypeProperty);
}

/******************************************************************************
* Determines the display particle colors.
******************************************************************************/
ConstPropertyPtr ParticlesVis::particleColors(const ParticlesObject* particles, bool highlightSelection) const
{
	OVITO_ASSERT(particles);
	particles->verifyIntegrity();

	// Take particle colors directly from the 'Color' property if available.
	DataObjectAccess<DataOORef, PropertyObject> output = particles->getProperty(ParticlesObject::ColorProperty);
	if(!output) {
		// Allocate new output color array.
		output.reset(ParticlesObject::OOClass().createStandardProperty(dataset(), particles->elementCount(), ParticlesObject::ColorProperty, false, ExecutionContext::Scripting));

		const Color defaultColor = defaultParticleColor();
		if(const PropertyObject* typeProperty = getParticleTypeColorProperty(particles)) {
			OVITO_ASSERT(typeProperty->size() == output->size());
			// Assign colors based on particle types.
			// Generate a lookup map for particle type colors.
			const std::map<int,Color> colorMap = typeProperty->typeColorMap();
			std::array<Color,16> colorArray;
			// Check if all type IDs are within a small, non-negative range.
			// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
			if(boost::algorithm::all_of(colorMap, [&colorArray](const std::map<int,Color>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
				colorArray.fill(defaultColor);
				for(const auto& entry : colorMap)
					colorArray[entry.first] = entry.second;
				// Fill color array.
				ConstPropertyAccess<int> typeData(typeProperty);
				const int* t = typeData.cbegin();
				for(Color& c : PropertyAccess<Color>(output.makeMutable())) {
					if(*t >= 0 && *t < (int)colorArray.size())
						c = colorArray[*t];
					else
						c = defaultColor;
					++t;
				}
			}
			else {
				// Fill color array.
				ConstPropertyAccess<int> typeData(typeProperty);
				const int* t = typeData.cbegin();
				for(Color& c : PropertyAccess<Color>(output.makeMutable())) {
					auto it = colorMap.find(*t);
					if(it != colorMap.end())
						c = it->second;
					else
						c = defaultColor;
					++t;
				}
			}
		}
		else {
			// Assign a uniform color to all particles.
			output.makeMutable()->fill(defaultColor);
		}
	}

	// Highlight selected particles with a special color.
	if(const PropertyObject* selectionProperty = highlightSelection ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr)
		output.makeMutable()->fillSelected(selectionParticleColor(), *selectionProperty);

	return output.take();
}

/******************************************************************************
* Returns the typed particle property used to determine the rendering radii 
* of particles (if no per-particle radii are defined).
******************************************************************************/
const PropertyObject* ParticlesVis::getParticleTypeRadiusProperty(const ParticlesObject* particles) const
{
	return particles->getProperty(ParticlesObject::TypeProperty);
}

/******************************************************************************
* Determines the display particle radii.
******************************************************************************/
ConstPropertyPtr ParticlesVis::particleRadii(const ParticlesObject* particles, bool includeGlobalScaleFactor) const
{
	particles->verifyIntegrity();
	FloatType defaultRadius = defaultParticleRadius();
	if(includeGlobalScaleFactor)
		defaultRadius *= radiusScaleFactor();

	// Take particle radii directly from the 'Radius' property if available.
	DataObjectAccess<DataOORef, PropertyObject> output = particles->getProperty(ParticlesObject::RadiusProperty);
	if(output) {
		// Check if the radius array contains any zero entries.
		ConstPropertyAccess<FloatType> radiusArray(output);
		if(boost::find(radiusArray, FloatType(0)) != radiusArray.end()) {	
			radiusArray.reset();
			// Replace zero entries in the per-particle array with the uniform default radius.
			boost::replace(PropertyAccess<FloatType>(output.makeMutable()), FloatType(0), defaultRadius);
		}
		// Apply global scaling factor.
		if(includeGlobalScaleFactor && radiusScaleFactor() != 1.0) {
			for(FloatType& r : PropertyAccess<FloatType>(output.makeMutable()))
				r *= radiusScaleFactor();
		}
	}
	else {
		// Allocate output array.
		output.reset(ParticlesObject::OOClass().createStandardProperty(dataset(), particles->elementCount(), ParticlesObject::RadiusProperty, false, ExecutionContext::Scripting));

		if(const PropertyObject* typeProperty = getParticleTypeRadiusProperty(particles)) {
			OVITO_ASSERT(typeProperty->size() == output->size());

			// Assign radii based on particle types.
			// Build a lookup map for particle type radii.
			std::map<int,FloatType> radiusMap = ParticleType::typeRadiusMap(typeProperty);
			// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
			if(boost::algorithm::any_of(radiusMap, [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
				// Apply global scaling factor.
				if(includeGlobalScaleFactor && radiusScaleFactor() != 1.0) {
					for(auto& p : radiusMap)
						p.second *= radiusScaleFactor();
				}
				// Fill radius array.
				ConstPropertyAccess<int> typeData(typeProperty);
				PropertyAccess<FloatType> radiusArray(output.makeMutable());
				boost::transform(typeData, radiusArray.begin(), [&](int t) {
					auto it = radiusMap.find(t);
					// Set particle radius only if the type's radius is non-zero.
					if(it != radiusMap.end() && it->second != 0)
						return it->second;
					else
						return defaultRadius;
				});
			}
			else {
				// Assign the uniform default radius to all particles.
				output.makeMutable()->fill(defaultRadius);
			}
		}
		else {
			// Assign the uniform default radius to all particles.
			output.makeMutable()->fill(defaultRadius);
		}
	}

	return output.take();
}

/******************************************************************************
* Determines the display radius of a single particle.
******************************************************************************/
FloatType ParticlesVis::particleRadius(size_t particleIndex, ConstPropertyAccess<FloatType> radiusProperty, const PropertyObject* typeProperty) const
{
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticlesObject::TypeProperty);

	if(radiusProperty && radiusProperty.size() > particleIndex) {
		// Take particle radius directly from the radius property.
		FloatType r = radiusProperty[particleIndex];
		if(r > 0) return r;
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Assign radius based on particle types.
		ConstPropertyAccess<int> typeData(typeProperty);
		const ParticleType* ptype = static_object_cast<ParticleType>(typeProperty->elementType(typeData[particleIndex]));
		if(ptype && ptype->radius() > 0)
			return ptype->radius();
	}

	return defaultParticleRadius();
}

/******************************************************************************
* Determines the display color of a single particle.
******************************************************************************/
Color ParticlesVis::particleColor(size_t particleIndex, ConstPropertyAccess<Color> colorProperty, const PropertyObject* typeProperty, ConstPropertyAccess<int> selectionProperty) const
{
	// Check if particle is selected.
	if(selectionProperty && selectionProperty.size() > particleIndex) {
		if(selectionProperty[particleIndex])
			return selectionParticleColor();
	}

	Color c = defaultParticleColor();
	if(colorProperty && colorProperty.size() > particleIndex) {
		// Take particle color directly from the color property.
		c = colorProperty[particleIndex];
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Return color based on particle types.
		ConstPropertyAccess<int> typeData(typeProperty);
		const ElementType* ptype = typeProperty->elementType(typeData[particleIndex]);
		if(ptype)
			c = ptype->color();
	}

	return c;
}

/******************************************************************************
* Returns the actual rendering quality used to render the particles.
******************************************************************************/
ParticlePrimitive::RenderingQuality ParticlesVis::effectiveRenderingQuality(SceneRenderer* renderer, const ParticlesObject* particles) const
{
	ParticlePrimitive::RenderingQuality renderQuality = renderingQuality();
	if(renderQuality == ParticlePrimitive::AutoQuality) {
		if(!particles) return ParticlePrimitive::HighQuality;
		size_t particleCount = particles->elementCount();
		if(particleCount < 4000 || renderer->isInteractive() == false)
			renderQuality = ParticlePrimitive::HighQuality;
		else if(particleCount < 400000)
			renderQuality = ParticlePrimitive::MediumQuality;
		else
			renderQuality = ParticlePrimitive::LowQuality;
	}
	return renderQuality;
}

/******************************************************************************
* Returns the effective primtive shape for rendering the particles.
******************************************************************************/
ParticlePrimitive::ParticleShape ParticlesVis::effectiveParticleShape(ParticleShape shape, const PropertyObject* shapeProperty, const PropertyObject* orientationProperty, const PropertyObject* roundnessProperty)
{
	if(shape == Sphere) {
		if(roundnessProperty != nullptr) return ParticlePrimitive::SuperquadricShape;
		if(shapeProperty != nullptr) return ParticlePrimitive::EllipsoidShape;
		else return ParticlePrimitive::SphericalShape;
	}
	else if(shape == Box) {
		if(shapeProperty != nullptr || orientationProperty != nullptr) return ParticlePrimitive::BoxShape;
		else return ParticlePrimitive::SquareCubicShape;
	}
	else if(shape == Circle) {
		return ParticlePrimitive::SphericalShape;
	}
	else if(shape == Square) {
		return ParticlePrimitive::SquareCubicShape;
	}
	else {
		OVITO_ASSERT(false);
		return ParticlePrimitive::SphericalShape;
	}
}

/******************************************************************************
* Lets the visualization element render the data object.
******************************************************************************/
void ParticlesVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Handle bounding-box computation in a separate method.
	if(renderer->isBoundingBoxPass()) {
		TimeInterval validityInterval;
		renderer->addToLocalBoundingBox(boundingBox(time, objectStack, contextNode, flowState, validityInterval));
		return;
	}

	// Get input particle data.
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectStack.back());
	if(!particles) return;
	particles->verifyIntegrity();

	// Make sure we don't exceed the internal limits. Rendering of more than 2 billion particles is not yet supported by OVITO.
	size_t particleCount = particles->elementCount();
	if(particleCount > (size_t)std::numeric_limits<int>::max()) {
		qWarning() << "WARNING: This version of OVITO doesn't support rendering more than" << std::numeric_limits<int>::max() << "particles.";
		return;
	}

	// Render all mesh-based particle types.
	renderMeshBasedParticles(particles, renderer, contextNode);

	// Render all primitive particle types.
	renderPrimitiveParticles(particles, renderer, contextNode);

	// Render all (sphero-)cylindric particle types.
	renderCylindricParticles(particles, renderer, contextNode);
}

/******************************************************************************
* Renders particle types that have a mesh-based shape assigned.
******************************************************************************/
void ParticlesVis::renderMeshBasedParticles(const ParticlesObject* particles, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Get input particle data.
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* selectionProperty = renderer->isInteractive() ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);
	const PropertyObject* orientationProperty = particles->getProperty(ParticlesObject::OrientationProperty);
	if(!typeProperty)
		return;

	// Compile list of particle types that have a mesh geometry assigned.
	QVarLengthArray<int, 10> shapeMeshParticleTypes;
	for(const ElementType* etype : typeProperty->elementTypes()) {
		if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
			if(ptype->shape() == ParticlesVis::ParticleShape::Mesh && ptype->shapeMesh() && ptype->shapeMesh()->mesh() && ptype->shapeMesh()->mesh()->faceCount() != 0) {
				shapeMeshParticleTypes.push_back(ptype->numericId());
			}
		}
	}
	if(shapeMeshParticleTypes.empty())
		return;

	// The type of lookup key used for caching the mesh rendering primitives:
	using ShapeMeshCacheKey = std::tuple<
		CompatibleRendererGroup,	// The scene renderer
		QPointer<PipelineSceneNode>,// The pipeline scene node
		ConstDataObjectRef,			// Particle type property
		FloatType,					// Default particle radius
		FloatType,					// Global radius scaling factor
		ConstDataObjectRef,			// Position property
		ConstDataObjectRef,			// Orientation property
		ConstDataObjectRef,			// Color property
		ConstDataObjectRef,			// Selection property
		ConstDataObjectRef,			// Transparency property
		ConstDataObjectRef			// Radius property
	>;
	// The data structure created for each mesh-based particle type.
	struct MeshParticleType {
		std::shared_ptr<MeshPrimitive> meshPrimitive;
		OORef<ObjectPickInfo> pickInfo;
		bool useMeshColors; ///< Controls the use of the original face colors from the mesh instead of the per-particle colors.
	};
	// The data structure stored in the vis cache for the mesh-based particle shapes.
	using ShapeMeshCacheValue = std::vector<MeshParticleType>;

	// Look up the rendering primitives for mesh-based particle types in the vis cache.
	ShapeMeshCacheValue& meshVisCache = dataset()->visCache().get<ShapeMeshCacheValue>(ShapeMeshCacheKey(
		renderer,
		const_cast<PipelineSceneNode*>(contextNode),
		typeProperty,
		defaultParticleRadius(),
		radiusScaleFactor(),
		positionProperty,
		orientationProperty,
		colorProperty,
		selectionProperty,
		transparencyProperty,
		radiusProperty));

	// Check if we already have valid rendering primitives that are up to date.
	if(meshVisCache.empty()) {
		meshVisCache.clear();

		// This data structure stores temporary per-particle instance data, separated by mesh-based particle type.
		struct MeshTypePerInstanceData {
			MeshTypePerInstanceData(DataBufferPtr tm, DataBufferPtr colors, DataBufferPtr indices) : 
				particleTMs(std::move(tm)), particleColors(std::move(colors)), particleIndices(std::move(indices)) {}
			DataBufferAccessAndRef<AffineTransformation> particleTMs; 	/// AffineTransformation of each particle to be rendered.
			DataBufferAccessAndRef<ColorA> particleColors;	/// Color of each particle to be rendered.				
			DataBufferAccessAndRef<int> particleIndices;	/// Index of each particle to be rendered in the original particles list.
		};
		std::vector<MeshTypePerInstanceData> perInstanceData;

		meshVisCache.reserve(shapeMeshParticleTypes.size());
		perInstanceData.reserve(shapeMeshParticleTypes.size());

		// Create one instanced mesh primitive for each mesh-based particle type. 
		for(int typeId : shapeMeshParticleTypes) {
			// Create a new instanced mesh primitive for the particle type.
			const ParticleType* ptype = static_object_cast<ParticleType>(typeProperty->elementType(typeId));
			OVITO_ASSERT(ptype->shapeMesh() && ptype->shapeMesh()->mesh());
			MeshParticleType meshType;
			meshType.meshPrimitive = renderer->createMeshPrimitive();
			meshType.meshPrimitive->setEmphasizeEdges(ptype->highlightShapeEdges());
			meshType.meshPrimitive->setCullFaces(ptype->shapeBackfaceCullingEnabled());
			meshType.meshPrimitive->setMesh(*ptype->shapeMesh()->mesh());
			meshType.useMeshColors = ptype->shapeUseMeshColor();
			meshVisCache.push_back(std::move(meshType));

			perInstanceData.emplace_back(
				DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 0, DataBuffer::Float, 12, 0, false),	// <AffineTransformation> (particle orientations)
				DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 0, DataBuffer::Float,  4, 0, false),	// <ColorA> (particle colors)
				DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 0, DataBuffer::Int, 1, 0, false)		// <int> (particle indices)
			);
		}

		// Compile the per-instance particle data (positions, orientations, colors, etc) for each mesh-based particle type.
		ConstPropertyAccessAndRef<Color> colors = particleColors(particles, renderer->isInteractive());
		ConstPropertyAccessAndRef<FloatType> radii = particleRadii(particles, true);
		ConstPropertyAccess<int> types(typeProperty);
		ConstPropertyAccess<Point3> positions(positionProperty);
		ConstPropertyAccess<Quaternion> orientations(orientationProperty);
		ConstPropertyAccess<FloatType> transparencies(transparencyProperty);
		size_t particleCount = particles->elementCount();
		for(size_t i = 0; i < particleCount; i++) {
			if(radii[i] <= 0) continue;
			auto iter = boost::find(shapeMeshParticleTypes, types[i]);
			if(iter == shapeMeshParticleTypes.end()) continue;
			size_t typeIndex = std::distance(shapeMeshParticleTypes.begin(), iter);
			AffineTransformation tm = AffineTransformation::scaling(radii[i]);
			if(positions)
				tm.translation() = positions[i] - Point3::Origin();
			if(orientations) {
				Quaternion quat = orientations[i];
				// Normalize quaternion.
				FloatType c = sqrt(quat.dot(quat));
				if(c <= FLOATTYPE_EPSILON)
					quat.setIdentity();
				else
					quat /= c;
				tm = tm * Matrix3::rotation(quat);
			}
			perInstanceData[typeIndex].particleTMs.push_back(tm);
			perInstanceData[typeIndex].particleColors.push_back(ColorA(colors[i], transparencies ? qBound(0.0, 1.0 - transparencies[i], 1.0) : 1.0));
			perInstanceData[typeIndex].particleIndices.push_back(i);
		}

		// Store the per-particle data into the mesh rendering primitives.
		for(size_t typeIndex = 0; typeIndex < meshVisCache.size(); typeIndex++) {
			if(meshVisCache[typeIndex].useMeshColors)
				perInstanceData[typeIndex].particleColors.reset();
			meshVisCache[typeIndex].meshPrimitive->setInstancedRendering(
				perInstanceData[typeIndex].particleTMs.take(), 
				perInstanceData[typeIndex].particleColors.take());
			// Create a picking structure for this set of particles.
			meshVisCache[typeIndex].pickInfo = new ParticlePickInfo(this, particles, perInstanceData[typeIndex].particleIndices.take());
		}
	}
	OVITO_ASSERT(meshVisCache.size() == shapeMeshParticleTypes.size());

	// Render the instanced mesh primitives, one for each particle type with a mesh-based shape.
	for(MeshParticleType& t : meshVisCache) {
		if(renderer->isPicking())
			renderer->beginPickObject(contextNode, t.pickInfo);
		renderer->renderMesh(t.meshPrimitive);
		if(renderer->isPicking())
			renderer->endPickObject();
	}
}

/******************************************************************************
* Renders all particles with a primitive shape (spherical, box, (super)quadrics).
******************************************************************************/
void ParticlesVis::renderPrimitiveParticles(const ParticlesObject* particles, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Determine whether all particle types use the same uniform shape or not.
	ParticlesVis::ParticleShape uniformShape = particleShape();
	OVITO_ASSERT(uniformShape != ParticleShape::Default);
	if(uniformShape == ParticleShape::Default)
		return;
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	if(typeProperty) {
		for(const ElementType* etype : typeProperty->elementTypes()) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
				ParticleShape ptypeShape = ptype->shape();
				if(ptypeShape == ParticleShape::Default)
					ptypeShape = particleShape();
				if(ptypeShape != uniformShape) {
					uniformShape = ParticleShape::Default; // This value indicates that particles do NOT all use one uniform shape.
					break;
				}
			}
		}
	}

	// Quit early if all particles have a shape not handled by this method.
	if(uniformShape != ParticleShape::Default) {
		if(uniformShape != ParticleShape::Sphere && 
			uniformShape != ParticleShape::Box && 
			uniformShape != ParticleShape::Circle && 
			uniformShape != ParticleShape::Square)
			return;
	}

	// Get input particle data.
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* typeColorProperty = getParticleTypeColorProperty(particles);
	const PropertyObject* typeRadiusProperty = getParticleTypeRadiusProperty(particles);
	const PropertyObject* selectionProperty = renderer->isInteractive() ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);
	const PropertyObject* asphericalShapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);
	const PropertyObject* orientationProperty = particles->getProperty(ParticlesObject::OrientationProperty);
	const PropertyObject* roundnessProperty = particles->getProperty(ParticlesObject::SuperquadricRoundnessProperty);

	// Pick render quality level adaptively based on current number of particles.
	ParticlePrimitive::RenderingQuality primitiveRenderQuality = effectiveRenderingQuality(renderer, particles);

	ConstPropertyPtr colorBuffer;
	ConstPropertyPtr radiusBuffer;

	/// Create separate rendering primitives for the different shapes supported by the method.
	for(ParticlesVis::ParticleShape shape : {ParticleShape::Sphere, ParticleShape::Box, ParticleShape::Circle, ParticleShape::Square}) {

		// Skip this shape if all particles are known to have a different shape.
		if(uniformShape != ParticleShape::Default && uniformShape != shape) 
			continue;

		// The lookup key for the cached rendering primitive:
		using ParticleCacheKey = std::tuple<
			CompatibleRendererGroup,			// Sscene renderer
			QPointer<PipelineSceneNode>,		// Pipeline scene node
			ParticlePrimitive::ShadingMode,		// Effective particle shading mode
			ParticlePrimitive::RenderingQuality,// Effective particle rendering quality
			ParticlePrimitive::ParticleShape,	// Effective particle shape
			ConstDataObjectRef,					// Particle type property
			size_t,								// Total particle count
			ParticlesVis::ParticleShape			// Global particle shape
		>;
		// The data structure stored in the vis cache.
		struct ParticleCacheValue {
			std::shared_ptr<ParticlePrimitive> primitive;
			OORef<ParticlePickInfo> pickInfo;
		};

		// Determine effective primitive shape and shading mode.
		ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(shape, asphericalShapeProperty, orientationProperty, roundnessProperty);
		ParticlePrimitive::ShadingMode primitiveShadingMode = (shape == Circle || shape == Square) ? ParticlePrimitive::FlatShading : ParticlePrimitive::NormalShading;

		// Look up the rendering primitive in the vis cache.
		auto& visCache = dataset()->visCache().get<ParticleCacheValue>(ParticleCacheKey(
			renderer,
			const_cast<PipelineSceneNode*>(contextNode),
			primitiveShadingMode,
			primitiveRenderQuality,
			primitiveParticleShape,
			typeProperty,
			particles->elementCount(),
			particleShape()));

		// Check if the rendering primitive needs to be recreated from scratch.
		if(!visCache.primitive) {

			// Determine the set of particles to be rendered using the current primitive shape.
			DataBufferAccessAndRef<int> activeParticleIndices;
			if(uniformShape != shape) {
				OVITO_ASSERT(typeProperty);

				// Build list of type IDs that use the current shape.
				std::vector<int> activeParticleTypes;
				for(const ElementType* etype : typeProperty->elementTypes()) {
					if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
						if(ptype->shape() == shape || (ptype->shape() == ParticleShape::Default && shape == particleShape()) || (ptype->shape() == ParticleShape::Mesh && !ptype->shapeMesh() && shape == ParticleShape::Box))
							activeParticleTypes.push_back(ptype->numericId());
					}
				}

				// Collect indices of all particles that have an active type.
				activeParticleIndices = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 0, DataBuffer::Int, 1, 0, false);
				size_t index = 0;
				for(int t : ConstPropertyAccess<int>(typeProperty)) {
					if(boost::find(activeParticleTypes, t) != activeParticleTypes.cend())
						activeParticleIndices.push_back(index);
					index++;
				}

				if(activeParticleIndices.size() == 0) {
					visCache.primitive.reset();
					visCache.pickInfo.reset();
					continue;	// No particles to be rendered using the current primitive shape.
				}
			}
			// Create the rendering primitive.
			visCache.primitive = renderer->createParticlePrimitive(primitiveShadingMode, primitiveRenderQuality, primitiveParticleShape);
			// Enable/disable indexed rendering of particle primitives.
			visCache.primitive->setIndices(activeParticleIndices.take());
			// Also create the corresponding picking record.
			visCache.pickInfo = new ParticlePickInfo(this, particles);
		}

		// Fill rendering primitive with particle properties.
		visCache.primitive->setPositions(positionProperty);
		visCache.primitive->setTransparencies(transparencyProperty);
		visCache.primitive->setSelection(selectionProperty);
		visCache.primitive->setAsphericalShapes(asphericalShapeProperty);
		visCache.primitive->setOrientations(orientationProperty);
		visCache.primitive->setRoundness(roundnessProperty);
		visCache.primitive->setSelectionColor(selectionParticleColor());

		// The type of lookup key used for caching the particle radii:
		using RadiiCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			FloatType,							// Default particle radius
			FloatType,							// Global radius scaling factor
			ConstDataObjectRef,					// Radius property
			ConstDataObjectRef					// Type property
		>;
		bool& radiiUpToDate = dataset()->visCache().get<bool>(RadiiCacheKey(
			visCache.primitive,
			defaultParticleRadius(),
			radiusScaleFactor(),
			radiusProperty,
			typeRadiusProperty));
		if(!radiiUpToDate) {
			radiiUpToDate = true;
			if(!radiusBuffer)
				radiusBuffer = particleRadii(particles, true);
			visCache.primitive->setRadii(radiusBuffer);
		}

		// The type of lookup key used for caching the particle colors:
		using ColorCacheKey = std::tuple<
			std::shared_ptr<ParticlePrimitive>,	// The rendering primitive
			ConstDataObjectRef,					// Type property
			ConstDataObjectRef					// Color property
		>;
		bool& colorsUpToDate = dataset()->visCache().get<bool>(ColorCacheKey(
			visCache.primitive,
			typeProperty,
			colorProperty));
		if(!colorsUpToDate) {
			colorsUpToDate = true;
			if(!colorBuffer)
				colorBuffer = particleColors(particles, false);
			visCache.primitive->setColors(colorBuffer);
		}

		// Render the particle primitive.
		if(renderer->isPicking()) renderer->beginPickObject(contextNode, visCache.pickInfo);
		renderer->renderParticles(visCache.primitive);
		if(renderer->isPicking()) renderer->endPickObject();
	}
}

/******************************************************************************
* Renders all particles with a (sphero-)cylindrical shape.
******************************************************************************/
void ParticlesVis::renderCylindricParticles(const ParticlesObject* particles, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Determine whether all particle types use the same uniform shape or not.
	ParticlesVis::ParticleShape uniformShape = particleShape();
	OVITO_ASSERT(uniformShape != ParticleShape::Default);
	if(uniformShape == ParticleShape::Default)
		return;
	const PropertyObject* typeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	if(typeProperty) {
		for(const ElementType* etype : typeProperty->elementTypes()) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
				ParticleShape ptypeShape = ptype->shape();
				if(ptypeShape == ParticleShape::Default)
					ptypeShape = particleShape();
				if(ptypeShape != uniformShape) {
					uniformShape = ParticleShape::Default; // This value indicates that particles do NOT all use one uniform shape.
					break;
				}
			}
		}
	}

	// Quit early if all particles have a shape not handled by this method.
	if(uniformShape != ParticleShape::Default) {
		if(uniformShape != ParticleShape::Cylinder && 
			uniformShape != ParticleShape::Spherocylinder)
			return;
	}

	// Get input particle data.
	const PropertyObject* positionProperty = particles->getProperty(ParticlesObject::PositionProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* colorProperty = particles->getProperty(ParticlesObject::ColorProperty);
	const PropertyObject* selectionProperty = renderer->isInteractive() ? particles->getProperty(ParticlesObject::SelectionProperty) : nullptr;
	const PropertyObject* transparencyProperty = particles->getProperty(ParticlesObject::TransparencyProperty);
	const PropertyObject* asphericalShapeProperty = particles->getProperty(ParticlesObject::AsphericalShapeProperty);
	const PropertyObject* orientationProperty = particles->getProperty(ParticlesObject::OrientationProperty);

	ConstPropertyPtr colorBuffer;
	ConstPropertyPtr radiusBuffer;

	/// Create separate rendering primitives for the different shapes supported by the method.
	for(ParticlesVis::ParticleShape shape : {ParticleShape::Cylinder, ParticleShape::Spherocylinder}) {

		// Skip this shape if all particles are known to have a different shape.
		if(uniformShape != ParticleShape::Default && uniformShape != shape) 
			continue;

		// The lookup key for the cached rendering primitive:
		using ParticleCacheKey = std::tuple<
			CompatibleRendererGroup,			// Scene renderer
			QPointer<PipelineSceneNode>,		// Pipeline scene node
			ConstDataObjectRef,					// Position property
			ConstDataObjectRef,					// Type property
			ConstDataObjectRef,					// Selection property
			ConstDataObjectRef,					// Color property
			ConstDataObjectRef,					// Transparency property
			ConstDataObjectRef,					// Apherical shape property
			ConstDataObjectRef,					// Orientation property
			ConstDataObjectRef,					// Radius property
			FloatType,							// Default particle radius
			FloatType,							// Global radius scaling factor
			ParticlesVis::ParticleShape,		// Global particle shape
			ParticlesVis::ParticleShape			// Local particle shape
		>;
		// The data structure stored in the vis cache.
		struct ParticleCacheValue {
			std::shared_ptr<CylinderPrimitive> cylinderPrimitive;
			std::shared_ptr<ParticlePrimitive> spheresPrimitives[2];
			OORef<ParticlePickInfo> pickInfo;
		};

		// Look up the rendering primitive in the vis cache.
		auto& visCache = dataset()->visCache().get<ParticleCacheValue>(ParticleCacheKey(
			renderer,
			const_cast<PipelineSceneNode*>(contextNode),
			positionProperty,
			typeProperty,
			selectionProperty,
			colorProperty,
			transparencyProperty,
			asphericalShapeProperty,
			orientationProperty,
			radiusProperty,
			defaultParticleRadius(),
			radiusScaleFactor(),
			particleShape(),
			shape));

		// Check if the rendering primitive needs to be recreated from scratch.
		if(!visCache.cylinderPrimitive) {

			// Determine the set of particles to be rendered using the current shape.
			DataBufferAccessAndRef<int> activeParticleIndices;
			if(uniformShape != shape) {
				OVITO_ASSERT(typeProperty);

				// Build list of type IDs that use the current shape.
				std::vector<int> activeParticleTypes;
				for(const ElementType* etype : typeProperty->elementTypes()) {
					if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(etype)) {
						if(ptype->shape() == shape || (ptype->shape() == ParticleShape::Default && shape == particleShape()))
							activeParticleTypes.push_back(ptype->numericId());
					}
				}

				// Collect indices of all particles that have an active type.
				activeParticleIndices = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 0, DataBuffer::Int, 1, 0, false);
				size_t index = 0;
				for(int t : ConstPropertyAccess<int>(typeProperty)) {
					if(boost::find(activeParticleTypes, t) != activeParticleTypes.cend())
						activeParticleIndices.push_back(index);
					index++;
				}

				if(activeParticleIndices.size() == 0) {
					visCache.cylinderPrimitive.reset();
					visCache.spheresPrimitives[0].reset();
					visCache.spheresPrimitives[1].reset();
					visCache.pickInfo.reset();
					continue;	// No particles to be rendered using the current primitive shape.
				}
			}
			int effectiveParticleCount = activeParticleIndices ? activeParticleIndices.size() : particles->elementCount();

			// Create the rendering primitive for the cylinders.
			visCache.cylinderPrimitive = renderer->createCylinderPrimitive(CylinderPrimitive::CylinderShape, CylinderPrimitive::NormalShading, CylinderPrimitive::HighQuality);

			// Determine cylinder colors.
			if(!colorBuffer)
				colorBuffer = particleColors(particles, renderer->isInteractive());
			
			// Determine cylinder radii (only needed if aspherical shape property is not present).
			if(!radiusBuffer && !asphericalShapeProperty)
				radiusBuffer = particleRadii(particles, true);

			// Allocate cylinder data buffers.
			DataBufferAccessAndRef<Point3> cylinderBasePositions = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, effectiveParticleCount, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Point3> cylinderHeadPositions = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, effectiveParticleCount, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<FloatType> cylinderRadii = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, effectiveParticleCount, DataBuffer::Float, 1, 0, false);
			DataBufferAccessAndRef<Color> cylinderColors = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, effectiveParticleCount, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<FloatType> cylinderTransparencies = transparencyProperty ? DataBufferPtr::create(dataset(), ExecutionContext::Scripting, effectiveParticleCount, DataBuffer::Float, 1, 0, false) : nullptr;

			// Fill data buffers.
			ConstPropertyAccess<Point3> positionArray(positionProperty);
			ConstPropertyAccess<Vector3> asphericalShapeArray(asphericalShapeProperty);
			ConstPropertyAccess<Quaternion> orientationArray(orientationProperty);
			ConstPropertyAccess<Color> colorsArray(colorBuffer);
			ConstPropertyAccess<FloatType> radiiArray(radiusBuffer);
			ConstPropertyAccess<FloatType> transparencies(transparencyProperty);
			for(int index = 0; index < effectiveParticleCount; index++) {
				int effectiveParticleIndex = activeParticleIndices ? activeParticleIndices[index] : index;
				const Point3& center = positionArray[effectiveParticleIndex];
				FloatType radius, length;
				if(asphericalShapeArray) {
					radius = std::abs(asphericalShapeArray[effectiveParticleIndex].x());
					length = asphericalShapeArray[effectiveParticleIndex].z();
				}
				else {
					radius = radiiArray[effectiveParticleIndex];
					length = radius * 2;
				}
				Vector3 dir = Vector3(0, 0, length);
				if(orientationArray) {
					dir = orientationArray[effectiveParticleIndex] * dir;
				}
				Point3 p = center - (dir * FloatType(0.5));
				cylinderBasePositions[index] = p;
				cylinderHeadPositions[index] = p + dir;
				cylinderRadii[index] = radius;
				cylinderColors[index] = colorsArray[effectiveParticleIndex];
				if(cylinderTransparencies)
					cylinderTransparencies[index] = transparencies[effectiveParticleIndex];
			}
			visCache.cylinderPrimitive->setPositions(cylinderBasePositions.take(), cylinderHeadPositions.take());
			visCache.cylinderPrimitive->setRadii(cylinderRadii.take());
			visCache.cylinderPrimitive->setColors(cylinderColors.take());
			visCache.cylinderPrimitive->setTransparencies(cylinderTransparencies.take());

			// Create the rendering primitive for the spheres.
			if(shape == ParticleShape::Spherocylinder) {
				visCache.spheresPrimitives[0] = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape);
				visCache.spheresPrimitives[0]->setPositions(visCache.cylinderPrimitive->basePositions());
				visCache.spheresPrimitives[0]->setRadii(visCache.cylinderPrimitive->radii());
				visCache.spheresPrimitives[0]->setColors(visCache.cylinderPrimitive->colors());
				visCache.spheresPrimitives[0]->setTransparencies(visCache.cylinderPrimitive->transparencies());
				visCache.spheresPrimitives[1] = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape);
				visCache.spheresPrimitives[1]->setPositions(visCache.cylinderPrimitive->headPositions());
				visCache.spheresPrimitives[1]->setRadii(visCache.cylinderPrimitive->radii());
				visCache.spheresPrimitives[1]->setColors(visCache.cylinderPrimitive->colors());
				visCache.spheresPrimitives[1]->setTransparencies(visCache.cylinderPrimitive->transparencies());
			}

			// Also create the corresponding picking record.
			visCache.pickInfo = new ParticlePickInfo(this, particles, activeParticleIndices.take());
		}

		// Render the particle primitive.
		if(renderer->isPicking()) renderer->beginPickObject(contextNode, visCache.pickInfo);
		renderer->renderCylinders(visCache.cylinderPrimitive);
		if(renderer->isPicking()) renderer->endPickObject();
		if(visCache.spheresPrimitives[0]) {
			if(renderer->isPicking()) renderer->beginPickObject(contextNode, visCache.pickInfo);
			renderer->renderParticles(visCache.spheresPrimitives[0]);
			if(renderer->isPicking()) renderer->endPickObject();
			if(renderer->isPicking()) renderer->beginPickObject(contextNode, visCache.pickInfo);
			renderer->renderParticles(visCache.spheresPrimitives[1]);
			if(renderer->isPicking()) renderer->endPickObject();
		}
	}
}

/******************************************************************************
* Render a marker around a particle to highlight it in the viewports.
******************************************************************************/
void ParticlesVis::highlightParticle(size_t particleIndex, const ParticlesObject* particles, SceneRenderer* renderer) const
{
	if(!renderer->isBoundingBoxPass()) {

		// Fetch properties of selected particle which are needed to render the overlay.
		const PropertyObject* posProperty = nullptr;
		const PropertyObject* radiusProperty = nullptr;
		const PropertyObject* colorProperty = nullptr;
		const PropertyObject* selectionProperty = nullptr;
		const PropertyObject* shapeProperty = nullptr;
		const PropertyObject* orientationProperty = nullptr;
		const PropertyObject* roundnessProperty = nullptr;
		const PropertyObject* typeProperty = nullptr;
		for(const PropertyObject* property : particles->properties()) {
			if(property->type() == ParticlesObject::PositionProperty && property->size() >= particleIndex)
				posProperty = property;
			else if(property->type() == ParticlesObject::RadiusProperty && property->size() >= particleIndex)
				radiusProperty = property;
			else if(property->type() == ParticlesObject::TypeProperty && property->size() >= particleIndex)
				typeProperty = property;
			else if(property->type() == ParticlesObject::ColorProperty && property->size() >= particleIndex)
				colorProperty = property;
			else if(property->type() == ParticlesObject::SelectionProperty && property->size() >= particleIndex)
				selectionProperty = property;
			else if(property->type() == ParticlesObject::AsphericalShapeProperty && property->size() >= particleIndex)
				shapeProperty = property;
			else if(property->type() == ParticlesObject::OrientationProperty && property->size() >= particleIndex)
				orientationProperty = property;
			else if(property->type() == ParticlesObject::SuperquadricRoundnessProperty && property->size() >= particleIndex)
				roundnessProperty = property;
		}
		if(!posProperty || particleIndex >= posProperty->size())
			return;

		// Get the particle type.
		const ParticleType* ptype = nullptr;
		if(typeProperty && particleIndex < typeProperty->size()) {
			ConstPropertyAccess<int> typeArray(typeProperty);
			ptype = dynamic_object_cast<ParticleType>(typeProperty->elementType(typeArray[particleIndex]));
		}

		// Check if the particle must be rendered using a custom shape.
		if(ptype && ptype->shape() == ParticleShape::Mesh && ptype->shapeMesh())
			return;	// Note: Highlighting of particles with user-defined shapes is not implemented yet.

		// The rendering shape of the highlighted particle.
		ParticleShape shape = particleShape();
		if(ptype && ptype->shape() != ParticleShape::Default)
			shape = ptype->shape();

		// Determine position of the selected particle.
		Point3 pos = ConstPropertyAccess<Point3>(posProperty)[particleIndex];

		// Determine radius of selected particle.
		FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);

		// Determine the display color of selected particle.
		Color color = particleColor(particleIndex, colorProperty, typeProperty, selectionProperty);
		Color highlightColor = selectionParticleColor();
		color = color * FloatType(0.5) + highlightColor * FloatType(0.5);

		// Determine rendering quality used to render the particles.
		ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, particles);

		std::shared_ptr<ParticlePrimitive> particleBuffer;
		std::shared_ptr<ParticlePrimitive> highlightParticleBuffer;
		std::shared_ptr<CylinderPrimitive> cylinderBuffer;
		std::shared_ptr<CylinderPrimitive> highlightCylinderBuffer;
		if(shape != Cylinder && shape != Spherocylinder) {
			// Determine effective particle shape and shading mode.
			ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(shape, shapeProperty, orientationProperty, roundnessProperty);
			ParticlePrimitive::ShadingMode primitiveShadingMode = ParticlePrimitive::NormalShading;
			if(shape == ParticlesVis::Circle || shape == ParticlesVis::Square)
				primitiveShadingMode = ParticlePrimitive::FlatShading;

			// Prepare data buffers.
			DataBufferAccessAndRef<Point3> positionBuffer = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 3, 0, false);
			positionBuffer[0] = pos;
			DataBufferAccessAndRef<Vector3> asphericalShapeBuffer;
			DataBufferAccessAndRef<Vector3> asphericalShapeBufferHighlight;
			if(shapeProperty) {
				asphericalShapeBuffer = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 3, 0, false);
				asphericalShapeBufferHighlight = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 3, 0, false);
				asphericalShapeBufferHighlight[0] = asphericalShapeBuffer[0] = ConstPropertyAccess<Vector3>(shapeProperty)[particleIndex];
				asphericalShapeBufferHighlight[0] += Vector3(renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
			}
			DataBufferAccessAndRef<Quaternion> orientationBuffer;
			if(orientationProperty) {
				orientationBuffer = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 4, 0, false);
				orientationBuffer[0] = ConstPropertyAccess<Quaternion>(orientationProperty)[particleIndex];
			}
			DataBufferAccessAndRef<Vector2> roundnessBuffer;
			if(roundnessProperty) {
				roundnessBuffer = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 2, 0, false);
				roundnessBuffer[0] = ConstPropertyAccess<Vector2>(roundnessProperty)[particleIndex];
			}

			particleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape);
			particleBuffer->setUniformColor(color);
			particleBuffer->setPositions(positionBuffer.take());
			particleBuffer->setUniformRadius(radius);
			particleBuffer->setAsphericalShapes(asphericalShapeBuffer.take());
			particleBuffer->setOrientations(orientationBuffer.take());
			particleBuffer->setRoundness(roundnessBuffer.take());

			// Prepare marker geometry buffer.
			highlightParticleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape);
			highlightParticleBuffer->setUniformColor(highlightColor);
			highlightParticleBuffer->setPositions(particleBuffer->positions());
			highlightParticleBuffer->setUniformRadius(radius + renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
			highlightParticleBuffer->setAsphericalShapes(asphericalShapeBufferHighlight.take());
			highlightParticleBuffer->setOrientations(particleBuffer->orientations());
			highlightParticleBuffer->setRoundness(particleBuffer->roundness());
		}
		else if(shape == Cylinder || shape == Spherocylinder) {
			FloatType radius, length;
			if(shapeProperty) {
				Vector3 shape = ConstPropertyAccess<Vector3>(shapeProperty)[particleIndex];
				radius = std::abs(shape.x());
				length = shape.z();
			}
			else {
				radius = defaultParticleRadius();
				length = radius * 2;
			}
			Vector3 dir = Vector3(0, 0, length);
			if(orientationProperty) {
				Quaternion q = ConstPropertyAccess<Quaternion>(orientationProperty)[particleIndex];
				dir = q * dir;
			}
			DataBufferAccessAndRef<Point3> positionBuffer1 = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Point3> positionBuffer2 = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 1, DataBuffer::Float, 3, 0, false);
			DataBufferAccessAndRef<Point3> positionBufferSpheres = DataBufferPtr::create(dataset(), ExecutionContext::Scripting, 2, DataBuffer::Float, 3, 0, false);
			positionBufferSpheres[0] = positionBuffer1[0] = pos - (dir * FloatType(0.5));
			positionBufferSpheres[1] = positionBuffer2[0] = pos + (dir * FloatType(0.5));
			cylinderBuffer = renderer->createCylinderPrimitive(CylinderPrimitive::CylinderShape, CylinderPrimitive::NormalShading, CylinderPrimitive::HighQuality);
			highlightCylinderBuffer = renderer->createCylinderPrimitive(CylinderPrimitive::CylinderShape, CylinderPrimitive::NormalShading, CylinderPrimitive::HighQuality);
			cylinderBuffer->setUniformColor(color);
			cylinderBuffer->setUniformRadius(radius);
			cylinderBuffer->setPositions(positionBuffer1.take(), positionBuffer2.take());
			FloatType padding = renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1);
			highlightCylinderBuffer->setUniformColor(highlightColor);
			highlightCylinderBuffer->setUniformRadius(radius + padding);
			highlightCylinderBuffer->setPositions(cylinderBuffer->basePositions(), cylinderBuffer->headPositions());
			if(shape == Spherocylinder) {
				particleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape);
				highlightParticleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape);
				particleBuffer->setPositions(positionBufferSpheres.take());
				particleBuffer->setUniformRadius(radius);
				particleBuffer->setUniformColor(color);
				highlightParticleBuffer->setPositions(particleBuffer->positions());
				highlightParticleBuffer->setUniformRadius(radius + padding);
				highlightParticleBuffer->setUniformColor(highlightColor);
			}
		}

		renderer->setHighlightMode(1);
		if(particleBuffer)
			renderer->renderParticles(particleBuffer);
		if(cylinderBuffer)
			renderer->renderCylinders(cylinderBuffer);
		renderer->setHighlightMode(2);
		if(highlightParticleBuffer)
			renderer->renderParticles(highlightParticleBuffer);
		if(highlightCylinderBuffer)
			renderer->renderCylinders(highlightCylinderBuffer);
		renderer->setHighlightMode(0);
	}
	else {
		// Fetch properties of selected particle needed to compute the bounding box.
		const PropertyObject* posProperty = nullptr;
		const PropertyObject* radiusProperty = nullptr;
		const PropertyObject* shapeProperty = nullptr;
		const PropertyObject* typeProperty = nullptr;
		for(const PropertyObject* property : particles->properties()) {
			if(property->type() == ParticlesObject::PositionProperty && property->size() >= particleIndex)
				posProperty = property;
			else if(property->type() == ParticlesObject::RadiusProperty && property->size() >= particleIndex)
				radiusProperty = property;
			else if(property->type() == ParticlesObject::AsphericalShapeProperty && property->size() >= particleIndex)
				shapeProperty = property;
			else if(property->type() == ParticlesObject::TypeProperty && property->size() >= particleIndex)
				typeProperty = property;
		}
		if(!posProperty)
			return;

		// Determine position of selected particle.
		Point3 pos = ConstPropertyAccess<Point3>(posProperty)[particleIndex];

		// Determine radius of selected particle.
		FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);
		if(shapeProperty) {
			Vector3 shape = ConstPropertyAccess<Vector3>(shapeProperty)[particleIndex];
			radius = std::max(radius, shape.x());
			radius = std::max(radius, shape.y());
			radius = std::max(radius, shape.z());
			radius *= 2;
		}

		if(radius <= 0 || !renderer->viewport())
			return;

		const AffineTransformation& tm = renderer->worldTransform();
		renderer->addToLocalBoundingBox(Box3(pos, radius + renderer->viewport()->nonScalingSize(tm * pos) * FloatType(1e-1)));
	}
}

/******************************************************************************
* Given an sub-object ID returned by the Viewport::pick() method, looks up the
* corresponding particle index.
******************************************************************************/
size_t ParticlePickInfo::particleIndexFromSubObjectID(quint32 subobjID) const
{
	if(_subobjectToParticleMapping && subobjID < _subobjectToParticleMapping->size())
		return ConstDataBufferAccess<int>(_subobjectToParticleMapping)[subobjID];
	return subobjID;
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString ParticlePickInfo::infoString(PipelineSceneNode* objectNode, quint32 subobjectId)
{
	size_t particleIndex = particleIndexFromSubObjectID(subobjectId);
	return particleInfoString(*particles(), particleIndex);
}

/******************************************************************************
* Builds the info string for a particle to be displayed in the status bar.
******************************************************************************/
QString ParticlePickInfo::particleInfoString(const ParticlesObject& particles, size_t particleIndex)
{
	QString str;
	for(const PropertyObject* property : particles.properties()) {
		if(property->size() <= particleIndex) continue;
		if(property->type() == ParticlesObject::SelectionProperty) continue;
		if(property->type() == ParticlesObject::ColorProperty) continue;
		if(!str.isEmpty()) str += QStringLiteral(" | ");
		str += property->name();
		str += QStringLiteral(" ");
		if(property->dataType() == PropertyObject::Int) {
			ConstPropertyAccess<int, true> data(property);
			for(size_t component = 0; component < data.componentCount(); component++) {
				if(component != 0) str += QStringLiteral(", ");
				str += QString::number(data.get(particleIndex, component));
				if(property->elementTypes().empty() == false) {
					if(const ElementType* ptype = property->elementType(data.get(particleIndex, component))) {
						if(!ptype->name().isEmpty())
							str += QString(" (%1)").arg(ptype->name());
					}
				}
			}
		}
		else if(property->dataType() == PropertyObject::Int64) {
			ConstPropertyAccess<qlonglong, true> data(property);
			for(size_t component = 0; component < property->componentCount(); component++) {
				if(component != 0) str += QStringLiteral(", ");
				str += QString::number(data.get(particleIndex, component));
			}
		}
		else if(property->dataType() == PropertyObject::Float) {
			ConstPropertyAccess<FloatType, true> data(property);
			for(size_t component = 0; component < property->componentCount(); component++) {
				if(component != 0) str += QStringLiteral(", ");
				str += QString::number(data.get(particleIndex, component));
			}
		}
		else {
			str += QStringLiteral("<%1>").arg(getQtTypeNameFromId(property->dataType()) ? getQtTypeNameFromId(property->dataType()) : "unknown");
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
