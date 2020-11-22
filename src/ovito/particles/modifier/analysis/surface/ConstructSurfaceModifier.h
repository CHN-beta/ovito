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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/mesh/surface/SurfaceMeshAccess.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles {

/*
 * Constructs a surface mesh from a particle system.
 */
class OVITO_PARTICLES_EXPORT ConstructSurfaceModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(ConstructSurfaceModifier, OOMetaClass)

	Q_CLASSINFO("DisplayName", "Construct surface mesh");
	Q_CLASSINFO("Description", "Build triangle mesh represention and compute volume and surface area of voids.");
	Q_CLASSINFO("ModifierCategory", "Visualization");

public:

	/// The different methods supported by this modifier for constructing the surface.
    enum SurfaceMethod {
		AlphaShape,
		GaussianDensity,
	};
    Q_ENUMS(SurfaceMethod);

public:

	/// Constructor.
	Q_INVOKABLE ConstructSurfaceModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;	
	
	/// Decides whether a preliminary viewport update is performed after the modifier has been
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a surface mesh,
	/// which requires further asynchronous processing before a viewport update makes sense.
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, ExecutionContext executionContext) override;

private:

	/// Abstract base class for computation engines that build the surface mesh.
	class ConstructSurfaceEngineBase : public Engine
	{
	public:

		/// Constructor.
		ConstructSurfaceEngineBase(ExecutionContext executionContext, DataSet* dataset, ConstPropertyPtr positions, ConstPropertyPtr selection, DataOORef<SurfaceMesh> mesh, bool computeSurfaceDistance, std::vector<ConstPropertyPtr> particleProperties) :
			Engine(executionContext),
			_positions(positions),
			_selection(std::move(selection)),
			_mesh(std::move(mesh)),
			_particleProperties(std::move(particleProperties)),
			_surfaceDistances(computeSurfaceDistance ? ParticlesObject::OOClass().createUserProperty(dataset, _positions->size(), PropertyObject::Float, 1, 0, tr("Surface Distance"), false) : nullptr) {}

		/// Returns the computed total surface area.
		FloatType surfaceArea() const { return (FloatType)_totalSurfaceArea; }

		/// Adds a summation contribution to the total surface area.
		void addSurfaceArea(FloatType a) { _totalSurfaceArea += a; }

		/// Returns the generated surface mesh.
		DataOORef<SurfaceMesh>& mesh() { return _mesh; }

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the input particle selection.
		const ConstPropertyPtr& selection() const { return _selection; }

		/// Returns the list of particle properties to copy over to the generated mesh.
		const std::vector<ConstPropertyPtr>& particleProperties() const { return _particleProperties; }

		/// Returns the output surface distance property.
		const PropertyPtr& surfaceDistances() const { return _surfaceDistances; }

	protected:

		/// Releases data that is no longer needed.
		void releaseWorkingData() {
			_positions.reset();
			_selection.reset();
			_particleProperties.clear();
		}

		/// Compute the distance of each input particle from the constructed surface.
		void computeSurfaceDistances(const SurfaceMeshAccess& mesh);

	private:

		/// The input particle coordinates.
		ConstPropertyPtr _positions;

		/// The input particle selection flags.
		ConstPropertyPtr _selection;

		/// The generated surface mesh.
		DataOORef<SurfaceMesh> _mesh;

		/// The computed total surface area.
		double _totalSurfaceArea = 0;

		/// The computed distance of each particle from the constructed surface.
		PropertyPtr _surfaceDistances;

		/// The list of particle properties to copy over to the generated mesh.
		std::vector<ConstPropertyPtr> _particleProperties;
	};

	/// Compute engine building the surface mesh using the alpha shape method.
	class AlphaShapeEngine : public ConstructSurfaceEngineBase
	{
	public:

		/// Constructor.
		AlphaShapeEngine(ExecutionContext executionContext, DataSet* dataset, ConstPropertyPtr positions, ConstPropertyPtr selection, ConstPropertyPtr particleGrains, DataOORef<SurfaceMesh> mesh, FloatType probeSphereRadius, int smoothingLevel, bool selectSurfaceParticles, bool identifyRegions, bool computeSurfaceDistance, std::vector<ConstPropertyPtr> particleProperties) :
			ConstructSurfaceEngineBase(executionContext, dataset, std::move(positions), std::move(selection), std::move(mesh), computeSurfaceDistance, std::move(particleProperties)),
			_particleGrains(std::move(particleGrains)),
			_probeSphereRadius(probeSphereRadius),
			_smoothingLevel(smoothingLevel),
			_identifyRegions(identifyRegions),
			_totalCellVolume(this->mesh()->domain() ? this->mesh()->domain()->volume3D() : 0.0),
			_surfaceParticleSelection(selectSurfaceParticles ? ParticlesObject::OOClass().createStandardProperty(dataset, this->positions()->size(), ParticlesObject::SelectionProperty, true, executionContext) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the input particle grain IDs.
		const ConstPropertyPtr& particleGrains() const { return _particleGrains; }

		/// Returns the selection set containing the particles at the constructed surfaces.
		const PropertyPtr& surfaceParticleSelection() const { return _surfaceParticleSelection; }

		/// Returns the evalue of the probe sphere radius parameter.
		FloatType probeSphereRadius() const { return _probeSphereRadius; }

	private:

		/// The radius of the virtual probe sphere (alpha-shape parameter).
		const FloatType _probeSphereRadius;

		/// The number of iterations of the smoothing algorithm to apply to the surface mesh.
		const int _smoothingLevel;

		/// Controls the identification of disconnected spatial regions (filled and empty).
		const bool _identifyRegions;

		/// The input particle grain property.
		ConstPropertyPtr _particleGrains;

		/// Number of filled regions that have been identified.
		SurfaceMeshAccess::size_type _filledRegionCount = 0;

		/// Number of empty regions that have been identified.
		SurfaceMeshAccess::size_type _emptyRegionCount = 0;

		/// The computed total volume of filled regions.
		double _totalFilledVolume = 0;

		/// The computed total volume of empty regions.
		double _totalEmptyVolume = 0;

		/// The total volume of the simulation cell.
		double _totalCellVolume = 0;

		/// The selection set of particles located right on the constructed surfaces.
		PropertyPtr _surfaceParticleSelection;
	};

	/// Compute engine building the surface mesh using the Gaussian density method.
	class GaussianDensityEngine : public ConstructSurfaceEngineBase
	{
	public:

		/// Constructor.
		GaussianDensityEngine(ExecutionContext executionContext, DataSet* dataset, ConstPropertyPtr positions, ConstPropertyPtr selection, DataOORef<SurfaceMesh> mesh,
				FloatType radiusFactor, FloatType isoLevel, int gridResolution, bool computeSurfaceDistance, std::vector<FloatType> radii, std::vector<ConstPropertyPtr> particleProperties) :
			ConstructSurfaceEngineBase(executionContext, dataset, std::move(positions), std::move(selection), std::move(mesh), computeSurfaceDistance, std::move(particleProperties)),
			_radiusFactor(radiusFactor),
			_isoLevel(isoLevel),
			_gridResolution(gridResolution),
			_particleRadii(std::move(radii)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	private:

		/// Scaling factor applied to atomic radii.
		const FloatType _radiusFactor;

		/// The threshold for constructing the isosurface of the density field.
		const FloatType _isoLevel;

		/// The number of voxels in the density grid.
		const int _gridResolution;

		/// The atomic input radii.
		std::vector<FloatType> _particleRadii;
	};

	/// The vis element for rendering the surface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<SurfaceMeshVis>, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// Surface construction method to use.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(SurfaceMethod, method, setMethod, PROPERTY_FIELD_MEMORIZE);

	/// Controls the radius of the probe sphere (alpha-shape method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, probeSphereRadius, setProbeSphereRadius, PROPERTY_FIELD_MEMORIZE);

	/// Controls the amount of smoothing (alpha-shape method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, smoothingLevel, setSmoothingLevel, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether only selected particles should be taken into account.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the modifier should select surface particles (alpha-shape method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectSurfaceParticles, setSelectSurfaceParticles);

	/// Controls whether the algorithm should identify disconnected spatial regions (alpha-shape method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, identifyRegions, setIdentifyRegions);

	/// Controls whether property values should be copied over from the input particles to the generated surface vertices (alpha-shape method / density field method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, transferParticleProperties, setTransferParticleProperties);

	/// Controls the number of grid cells along the largest cell dimension (density field method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, gridResolution, setGridResolution, PROPERTY_FIELD_MEMORIZE);

	/// The scaling factor applied to atomic radii (density field method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, radiusFactor, setRadiusFactor, PROPERTY_FIELD_MEMORIZE);

	/// The threshold value for constructing the isosurface of the density field (density field method).
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, isoValue, setIsoValue, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether the algorithm should compute the shortest distance of each particle from the constructed surface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeSurfaceDistance, setComputeSurfaceDistance);
};

}	// End of namespace
}	// End of namespace
