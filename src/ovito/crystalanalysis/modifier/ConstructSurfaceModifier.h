///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include <ovito/mesh/surface/SurfaceMeshData.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Constructs a surface mesh from a particle system.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ConstructSurfaceModifier : public AsynchronousModifier
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

	/// Decides whether a preliminary viewport update is performed after the modifier has been
	/// evaluated but before the entire pipeline evaluation is complete.
	/// We suppress such preliminary updates for this modifier, because it produces a surface mesh,
	/// which requires further asynchronous processing before a viewport update makes sense.
	virtual bool performPreliminaryUpdateAfterEvaluation() override { return false; }

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Abstract base class for computation engines that build the surface mesh.
	class ConstructSurfaceEngineBase : public ComputeEngine
	{
	public:

		/// Constructor.
		ConstructSurfaceEngineBase(ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCell& simCell, FloatType radius) :
			_positions(positions),
			_selection(std::move(selection)),
			_mesh(simCell),
			_radius(radius) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_selection.reset();
			ComputeEngine::cleanup();
		}

		/// Returns the generated surface mesh.
		const SurfaceMeshData& mesh() const { return _mesh; }

		/// Returns a mutable reference to the surface mesh structure.
		SurfaceMeshData& mesh() { return _mesh; }

		/// Returns the computed surface area.
		FloatType surfaceArea() const { return (FloatType)_surfaceArea; }

		/// Sums a contribution to the total surface area.
		void addSurfaceArea(FloatType a) { _surfaceArea += a; }

		/// Returns the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the input particle selection.
		const ConstPropertyPtr& selection() const { return _selection; }

		/// Returns the evalue of the probe sphere radius parameter.
		FloatType probeSphereRadius() const { return _radius; }

	private:

		const FloatType _radius;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _selection;

		/// The generated surface mesh.
		SurfaceMeshData _mesh;

		/// The computed surface area.
		double _surfaceArea = 0;
	};

	/// Computation engine that builds the surface mesh.
	class AlphaShapeEngine : public ConstructSurfaceEngineBase
	{
	public:

		/// Constructor.
		AlphaShapeEngine(ConstPropertyPtr positions, ConstPropertyPtr selection, const SimulationCell& simCell, FloatType radius, int smoothingLevel, bool selectSurfaceParticles) :
			ConstructSurfaceEngineBase(std::move(positions), std::move(selection), simCell, radius),
			_smoothingLevel(smoothingLevel),
			_totalVolume(std::abs(simCell.matrix().determinant())),
			_surfaceParticleSelection(selectSurfaceParticles ? ParticlesObject::OOClass().createStandardStorage(positions->size(), ParticlesObject::SelectionProperty, true) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the computed solid volume.
		FloatType solidVolume() const { return (FloatType)_solidVolume; }

		/// Sums a contribution to the total solid volume.
		void addSolidVolume(FloatType v) { _solidVolume += v; }

		/// Returns the computed total volume.
		FloatType totalVolume() const { return (FloatType)_totalVolume; }

		/// Returns the selection set containing the particles at the constructed surfaces.
		const PropertyPtr& surfaceParticleSelection() const { return _surfaceParticleSelection; }

	private:

		/// The number of iterations of the smoothing algorithm to apply to the surface mesh.
		const int _smoothingLevel;

		/// The computed solid volume.
		double _solidVolume = 0;

		/// The computed total volume.
		double _totalVolume = 0;

		/// The selection set containing the particles right on the constructed surfaces.
		PropertyPtr _surfaceParticleSelection;
	};	

	/// Controls the radius of the probe sphere.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, probeSphereRadius, setProbeSphereRadius, PROPERTY_FIELD_MEMORIZE);

	/// Controls the amount of smoothing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, smoothingLevel, setSmoothingLevel, PROPERTY_FIELD_MEMORIZE);

	/// Controls whether only selected particles should be taken into account.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the modifier should select surface particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectSurfaceParticles, setSelectSurfaceParticles);

	/// The vis element for rendering the surface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshVis, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);

	/// Surface construction method
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(SurfaceMethod, method, setMethod, PROPERTY_FIELD_MEMORIZE);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Plugins::CrystalAnalysis::ConstructSurfaceModifier::SurfaceMethod);
Q_DECLARE_TYPEINFO(Ovito::Plugins::CrystalAnalysis::ConstructSurfaceModifier::SurfaceMethod, Q_PRIMITIVE_TYPE);
