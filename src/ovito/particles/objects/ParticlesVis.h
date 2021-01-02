////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/rendering/SceneRenderer.h>
#include <ovito/core/rendering/ParticlePrimitive.h>
#include <ovito/core/dataset/data/DataVis.h>

namespace Ovito { namespace Particles {

/**
 * \brief A visualization element for rendering particles.
 */
class OVITO_PARTICLES_EXPORT ParticlesVis : public DataVis
{
	Q_OBJECT
	OVITO_CLASS(ParticlesVis)
	Q_CLASSINFO("DisplayName", "Particles");

public:

	/// The standard shapes supported by the particles visualization element.
	enum ParticleShape {
		Sphere, 			// Includes ellipsoids and superquadrics
		Box,				// Includes cubes and non-cubic boxes
		Circle,
		Square,
		Cylinder,
		Spherocylinder,
		Mesh,
		Default
	};
	Q_ENUM(ParticleShape);

public:

	/// Constructor.
	Q_INVOKABLE ParticlesVis(DataSet* dataset);

	/// Renders the visual element.
	virtual void render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// Computes the bounding box of the visual element.
	virtual Box3 boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;

	/// Returns the default display color for particles.
	Color defaultParticleColor() const { return Color(1,1,1); }

	/// Returns the display color used for selected particles.
	Color selectionParticleColor() const { return Color(1,0,0); }

	/// Returns the actual particle shape used to render the particles.
	static ParticlePrimitive::ParticleShape effectiveParticleShape(ParticleShape shape, const PropertyObject* shapeProperty, const PropertyObject* orientationProperty, const PropertyObject* roundnessProperty);

	/// Returns the actual rendering quality used to render the particles.
	ParticlePrimitive::RenderingQuality effectiveRenderingQuality(SceneRenderer* renderer, const ParticlesObject* particles) const;

	/// Determines the color of each particle to be used for rendering.
	ConstPropertyPtr particleColors(const ParticlesObject* particles, bool highlightSelection) const;

	/// Determines the particle radii used for rendering.
	ConstPropertyPtr particleRadii(const ParticlesObject* particles) const;

	/// Determines the display radius of a single particle.
	FloatType particleRadius(size_t particleIndex, ConstPropertyAccess<FloatType> radiusProperty, const PropertyObject* typeProperty) const;

	/// s the display color of a single particle.
	ColorA particleColor(size_t particleIndex, ConstPropertyAccess<Color> colorProperty, const PropertyObject* typeProperty, ConstPropertyAccess<int> selectionProperty, ConstPropertyAccess<FloatType> transparencyProperty) const;

	/// Computes the bounding box of the particles.
	Box3 particleBoundingBox(ConstPropertyAccess<Point3> positionProperty, const PropertyObject* typeProperty, ConstPropertyAccess<FloatType> radiusProperty, ConstPropertyAccess<Vector3> shapeProperty, bool includeParticleRadius) const;

	/// Render a marker around a particle to highlight it in the viewports.
	void highlightParticle(size_t particleIndex, const ParticlesObject* particles, SceneRenderer* renderer) const;

	/// Returns the typed particle property used to determine the rendering colors of particles (if no per-particle colors are defined).
	virtual const PropertyObject* getParticleTypeColorProperty(const ParticlesObject* particles) const;

	/// Returns the typed particle property used to determine the rendering radii of particles (if no per-particle radii are defined).
	virtual const PropertyObject* getParticleTypeRadiusProperty(const ParticlesObject* particles) const;

public:

    Q_PROPERTY(Ovito::ParticlePrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);
    Q_PROPERTY(Ovito::Particles::ParticlesVis::ParticleShape particleShape READ particleShape WRITE setParticleShape);

private:

	/// Renders particle types that have a mesh-based shape assigned.
	void renderMeshBasedParticles(const ParticlesObject* particles, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

	/// Renders all particles with a primitive shape (spherical, box, (super)quadrics).
	void renderPrimitiveParticles(const ParticlesObject* particles, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

	/// Renders all particles with a (sphero-)cylindrical shape.
	void renderCylindricParticles(const ParticlesObject* particles, SceneRenderer* renderer, const PipelineSceneNode* contextNode);

private:

	/// Controls the default display radius of atomic particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, defaultParticleRadius, setDefaultParticleRadius, PROPERTY_FIELD_MEMORIZE);

	/// Controls the rendering quality mode for particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// Controls the display shape of particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticleShape, particleShape, setParticleShape);
};

/**
 * \brief This information record is attached to the particles by the ParticlesVis when rendering
 * them in the viewports. It facilitates the picking of particles with the mouse.
 */
class OVITO_PARTICLES_EXPORT ParticlePickInfo : public ObjectPickInfo
{
	Q_OBJECT
	OVITO_CLASS(ParticlePickInfo)

public:

	/// Constructor.
	ParticlePickInfo(ParticlesVis* visElement, DataOORef<const ParticlesObject> particles, ConstDataBufferPtr subobjectToParticleMapping = {}) :
		_visElement(visElement), _particles(std::move(particles)), _subobjectToParticleMapping(std::move(subobjectToParticleMapping)) {}

	/// Returns the particles object.
	const DataOORef<const ParticlesObject>& particles() const { OVITO_ASSERT(_particles); return _particles; }

	/// Updates the reference to the particles object.
	void setParticles(DataOORef<const ParticlesObject> particles) { _particles = std::move(particles); }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(PipelineSceneNode* objectNode, quint32 subobjectId) override;

	/// Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding particle index.
	size_t particleIndexFromSubObjectID(quint32 subobjID) const;

	/// Builds the info string for a particle to be displayed in the status bar.
	static QString particleInfoString(const ParticlesObject& particles, size_t particleIndex);

private:

	/// The vis element that rendered the particles.
	OORef<ParticlesVis> _visElement;

	/// The particles object.
	DataOORef<const ParticlesObject> _particles;

	/// Stores the indices of the particles associated with the rendering primitives.
	ConstDataBufferPtr _subobjectToParticleMapping;
};

}	// End of namespace
}	// End of namespace
