///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/util/ParticleOrderingFingerprint.h>
#include <ovito/stdobj/properties/PropertyStorage.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the atomic volume and the Voronoi indices of particles.
 */
class OVITO_PARTICLES_EXPORT VoronoiAnalysisModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class VoronoiAnalysisModifierClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(VoronoiAnalysisModifier, VoronoiAnalysisModifierClass)

	Q_CLASSINFO("DisplayName", "Voronoi analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE VoronoiAnalysisModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

	/// Computes the modifier's results.
	class VoronoiAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		VoronoiAnalysisEngine(const TimeInterval& validityInterval, ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, ConstPropertyPtr selection, std::vector<FloatType> radii,
							const SimulationCell& simCell,
							bool computeIndices, bool computeBonds, FloatType edgeThreshold, FloatType faceThreshold, FloatType relativeFaceThreshold) :
			ComputeEngine(validityInterval),
			_positions(positions),
			_selection(std::move(selection)),
			_radii(std::move(radii)),
			_simCell(simCell),
			_edgeThreshold(edgeThreshold),
			_faceThreshold(faceThreshold),
			_relativeFaceThreshold(relativeFaceThreshold),
			_computeBonds(computeBonds),
			_coordinationNumbers(ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::CoordinationProperty, true)),
			_atomicVolumes(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, QStringLiteral("Atomic Volume"), true)),
			_maxFaceOrders(computeIndices ? std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Int, 1, 0, QStringLiteral("Max Face Order"), true) : nullptr),
			_inputFingerprint(std::move(fingerprint)) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_positions.reset();
			_selection.reset();
			decltype(_radii){}.swap(_radii);
			ComputeEngine::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the property storage that contains the computed coordination numbers.
		const PropertyPtr& coordinationNumbers() const { return _coordinationNumbers; }

		/// Returns the property storage that contains the computed atomic volumes.
		const PropertyPtr& atomicVolumes() const { return _atomicVolumes; }

		/// Returns the property storage that contains the computed Voronoi indices.
		const PropertyPtr& voronoiIndices() const { return _voronoiIndices; }

		/// Returns the property storage that contains the maximum face order for each particle.
		const PropertyPtr& maxFaceOrders() const { return _maxFaceOrders; }

		/// Returns the volume sum of all Voronoi cells computed by the modifier.
		std::atomic<double>& voronoiVolumeSum() { return _voronoiVolumeSum; }

		/// Returns the maximum number of edges of any Voronoi face.
		std::atomic<int>& maxFaceOrder() { return _maxFaceOrder; }

		/// Returns the generated nearest neighbor bonds.
		std::vector<Bond>& bonds() { return _bonds; }

		const SimulationCell& simCell() const { return _simCell; }
		const ConstPropertyPtr& positions() const { return _positions; }
		const ConstPropertyPtr selection() const { return _selection; }

	private:

		const FloatType _edgeThreshold;
		const FloatType _faceThreshold;
		const FloatType _relativeFaceThreshold;
		const SimulationCell _simCell;
		std::vector<FloatType> _radii;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _selection;
		bool _computeBonds;

		const PropertyPtr _coordinationNumbers;
		const PropertyPtr _atomicVolumes;
		PropertyPtr _voronoiIndices;
		const PropertyPtr _maxFaceOrders;
		std::vector<Bond> _bonds;
		ParticleOrderingFingerprint _inputFingerprint;

		/// The volume sum of all Voronoi cells.
		std::atomic<double> _voronoiVolumeSum{0.0};

		/// The maximum number of edges of a Voronoi face.
		std::atomic<int> _maxFaceOrder{0};

		/// Maximum length of Voronoi index vectors produced by this modifier.
		constexpr static int FaceOrderStorageLimit = 32;
	};

	/// Controls whether the modifier takes into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);

	/// Controls whether the modifier takes into account particle radii.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useRadii, setUseRadii);

	/// Controls whether the modifier computes Voronoi indices.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeIndices, setComputeIndices);

	/// The minimum length for an edge to be counted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, edgeThreshold, setEdgeThreshold);

	/// The minimum area for a face to be counted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, faceThreshold, setFaceThreshold);

	/// The minimum area for a face to be counted relative to the total polyhedron surface.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, relativeFaceThreshold, setRelativeFaceThreshold);

	/// Controls whether the modifier output nearest neighbor bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeBonds, setComputeBonds);

	/// The vis element for rendering the bonds.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(BondsVis, bondsVis, setBondsVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace