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

#include <ovito/particles/Particles.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include <ptm/ptm_polar.h>
#include "AtomicStrainModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(AtomicStrainModifier);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, cutoff);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateDeformationGradients);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateStrainTensors);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateNonaffineSquaredDisplacements);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, selectInvalidParticles);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateStretchTensors);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateRotations);
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateNonaffineSquaredDisplacements, "Output non-affine squared displacements");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, selectInvalidParticles, "Select invalid particles");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateStretchTensors, "Output stretch tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateRotations, "Output rotations");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(AtomicStrainModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AtomicStrainModifier::AtomicStrainModifier(DataSet* dataset) : ReferenceConfigurationModifier(dataset),
    _cutoff(3),
	_calculateDeformationGradients(false),
	_calculateStrainTensors(false),
	_calculateNonaffineSquaredDisplacements(false),
	_calculateStretchTensors(false),
	_calculateRotations(false),
    _selectInvalidParticles(true)
{
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::EnginePtr> AtomicStrainModifier::createEngineInternal(const PipelineEvaluationRequest& request, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, Application::ExecutionContext executionContext, TimeInterval validityInterval)
{
	// Get the current particle positions.
	const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Get the reference particle position.
	const ParticlesObject* refParticles = referenceState.getObject<ParticlesObject>();
	if(!refParticles)
		throwException(tr("Reference configuration does not contain particle positions."));
	refParticles->verifyIntegrity();
	const PropertyObject* refPosProperty = refParticles->expectProperty(ParticlesObject::PositionProperty);

	// Get the simulation cells.
	const SimulationCellObject* inputCell = input.expectObject<SimulationCellObject>();
	const SimulationCellObject* refCell = referenceState.getObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

	// Vaildate the simulation cells.
	if((!inputCell->is2D() && inputCell->volume3D() < FLOATTYPE_EPSILON) || (inputCell->is2D() && inputCell->volume2D() < FLOATTYPE_EPSILON))
		throwException(tr("Simulation cell is degenerate in the deformed configuration."));
	if((!inputCell->is2D() && refCell->volume3D() < FLOATTYPE_EPSILON) || (inputCell->is2D() && refCell->volume2D() < FLOATTYPE_EPSILON))
		throwException(tr("Simulation cell is degenerate in the reference configuration."));

	// Get particle identifiers.
	const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	const PropertyObject* refIdentifierProperty = refParticles->getProperty(ParticlesObject::IdentifierProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<AtomicStrainEngine>(executionContext, dataset(), validityInterval, particles, posProperty, inputCell, refPosProperty, refCell,
			identifierProperty, refIdentifierProperty,
			cutoff(), affineMapping(), useMinimumImageConvention(), calculateDeformationGradients(), calculateStrainTensors(),
			calculateNonaffineSquaredDisplacements(), calculateRotations(), calculateStretchTensors(), selectInvalidParticles());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AtomicStrainModifier::AtomicStrainEngine::perform()
{
	setProgressText(tr("Computing atomic displacements"));

	// First determine the mapping from particles of the reference config to particles
	// of the current config.
	if(!buildParticleMapping(false, false))
		return;

	// Compute displacement vectors of particles in the reference configuration.
	PropertyAccess<Vector3> displacementsArray(displacements());
	ConstPropertyAccess<Point3> positionsArray(positions());
	ConstPropertyAccess<Point3> refPositionsArray(refPositions());
	parallelForChunks(displacements()->size(), *this, [&](size_t startIndex, size_t count, Task& task) {
		Vector3* u = displacementsArray.begin() + startIndex;
		const Point3* p0 = refPositionsArray.cbegin() + startIndex;
		auto index = refToCurrentIndexMap().cbegin() + startIndex;
		for(; count; --count, ++u, ++p0, ++index) {
			if(task.isCanceled()) return;
			if(*index == std::numeric_limits<size_t>::max()) {
				u->setZero();
				continue;
			}
			Point3 reduced_reference_pos = refCell()->inverseMatrix() * (*p0);
			Point3 reduced_current_pos = cell()->inverseMatrix() * positionsArray[*index];
			Vector3 delta = reduced_current_pos - reduced_reference_pos;
			if(useMinimumImageConvention()) {
				for(size_t k = 0; k < 3; k++) {
					if(refCell()->hasPbc(k))
						delta[k] -= std::floor(delta[k] + FloatType(0.5));
				}
			}
			*u = refCell()->matrix() * delta;
		}
	});
	if(isCanceled())
		return;

	setProgressText(tr("Computing atomic strain tensors"));

	// Prepare the neighbor list for the reference configuration.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_cutoff, refPositions(), refCell(), {}, this))
		return;

	// Prepare the output data arrays.
	PropertyAccess<int> invalidParticlesArray(invalidParticles());
	PropertyAccess<Matrix3> deformationGradientsArray(deformationGradients());
	PropertyAccess<SymmetricTensor2> strainTensorsArray(strainTensors());
	PropertyAccess<FloatType> shearStrainsArray(shearStrains());
	PropertyAccess<FloatType> volumetricStrainsArray(volumetricStrains());
	PropertyAccess<FloatType> nonaffineSquaredDisplacementsArray(nonaffineSquaredDisplacements());
	PropertyAccess<Quaternion> rotationsArray(rotations());
	PropertyAccess<SymmetricTensor2> stretchTensorsArray(stretchTensors());

	// Perform individual strain calculation for each particle.
	parallelFor(positions()->size(), *this, [&](size_t particleIndex) {

		// Note: We do the following calculations using double precision numbers to
		// minimize numerical errors. Final results will be converted back to
		// standard precision.

		Matrix_3<double> V = Matrix_3<double>::Zero();
		Matrix_3<double> W = Matrix_3<double>::Zero();
		int numNeighbors = 0;

		// Iterate over neighbors of central particle.
		size_t particleIndexReference = currentToRefIndexMap()[particleIndex];
		FloatType sumSquaredDistance = 0;
		if(particleIndexReference != std::numeric_limits<size_t>::max()) {
			const Vector3& center_displacement = displacementsArray[particleIndexReference];
			for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndexReference); !neighQuery.atEnd(); neighQuery.next()) {
				size_t neighborIndexCurrent = refToCurrentIndexMap()[neighQuery.current()];
				if(neighborIndexCurrent == std::numeric_limits<size_t>::max()) continue;
				const Vector3& neigh_displacement = displacementsArray[neighQuery.current()];
				Vector3 delta_ref = neighQuery.delta();
				Vector3 delta_cur = delta_ref + neigh_displacement - center_displacement;
				if(affineMapping() == TO_CURRENT_CELL) {
					delta_ref = refToCurTM() * delta_ref;
					delta_cur = refToCurTM() * delta_cur;
				}
				else if(affineMapping() != TO_REFERENCE_CELL) {
					delta_cur = refToCurTM() * delta_cur;
				}
				for(size_t i = 0; i < 3; i++) {
					for(size_t j = 0; j < 3; j++) {
						V(i,j) += delta_ref[j] * delta_ref[i];
						W(i,j) += delta_ref[j] * delta_cur[i];
					}
				}
				sumSquaredDistance += delta_ref.squaredLength();
				numNeighbors++;
			}
		}

		// Special handling for 2D systems.
		if(cell()->is2D()) {
			// Assume plane strain.
			V(2,2) = W(2,2) = 1;
			V(0,2) = V(1,2) = V(2,0) = V(2,1) = 0;
			W(0,2) = W(1,2) = W(2,0) = W(2,1) = 0;
		}

		// Check if matrix can be inverted.
		Matrix_3<double> inverseV;
		double detThreshold = (double)sumSquaredDistance * 1e-12;
		if(numNeighbors < 2 || (!cell()->is2D() && numNeighbors < 3) || !V.inverse(inverseV, detThreshold) || std::abs(W.determinant()) <= detThreshold) {
			if(invalidParticlesArray)
				invalidParticlesArray[particleIndex] = 1;
			if(deformationGradientsArray)
				deformationGradientsArray[particleIndex].setZero();
			if(strainTensorsArray)
				strainTensorsArray[particleIndex] = SymmetricTensor2::Zero();
			if(nonaffineSquaredDisplacementsArray)
				nonaffineSquaredDisplacementsArray[particleIndex] = 0;
			shearStrainsArray[particleIndex] = 0;
			volumetricStrainsArray[particleIndex] = 0;
			if(rotationsArray)
				rotationsArray[particleIndex] = Quaternion(0,0,0,0);
			if(stretchTensorsArray)
				stretchTensorsArray[particleIndex] = SymmetricTensor2::Zero();
			addInvalidParticle();
			return;
		}

		// Calculate deformation gradient tensor F.
		Matrix_3<double> F = W * inverseV;
		if(deformationGradientsArray) {
			for(Matrix_3<double>::size_type col = 0; col < 3; col++) {
				for(Matrix_3<double>::size_type row = 0; row < 3; row++) {
					deformationGradientsArray[particleIndex](row, col) = (FloatType)F(row, col);
				}
			}
		}

		// Polar decomposition F=RU.
		if(rotationsArray || stretchTensorsArray) {
			Matrix_3<double> R, U;
			ptm::polar_decomposition_3x3(F.elements(), false, R.elements(), U.elements());
			if(rotationsArray) {
				// If F contains a reflection, R will not be a pure rotation matrix and the
				// conversion to a quaternion below will fail with an assertion error.
				// Thus, in the rather unlikely case that F contains a reflection, we simply flip the
				// R matrix to make it a pure rotation.
				if(R.determinant() < 0) {
					for(size_t i = 0; i < 3; i++)
						for(size_t j = 0; j < 3; j++)
							R(i,j) = -R(i,j);
				}
				rotationsArray[particleIndex] = (Quaternion)QuaternionT<double>(R);
			}
			if(stretchTensorsArray) {
				stretchTensorsArray[particleIndex] = SymmetricTensor2(U(0,0), U(1,1), U(2,2), U(0,1), U(0,2), U(1,2));
			}
		}

		// Calculate strain tensor.
		SymmetricTensor2T<double> strain = (Product_AtA(F) - SymmetricTensor2T<double>::Identity()) * 0.5;
		if(strainTensorsArray)
			strainTensorsArray[particleIndex] = (SymmetricTensor2)strain;

		// Calculate nonaffine displacement.
		if(nonaffineSquaredDisplacementsArray) {
			FloatType D2min = 0;
			Matrix3 Fftype = static_cast<Matrix3>(F);

			// Again iterate over neighbor vectors of central particle.
			numNeighbors = 0;
			const Vector3& center_displacement = displacementsArray[particleIndexReference];
			for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndexReference); !neighQuery.atEnd(); neighQuery.next()) {
				size_t neighborIndexCurrent = refToCurrentIndexMap()[neighQuery.current()];
				if(neighborIndexCurrent == std::numeric_limits<size_t>::max()) continue;
				const Vector3& neigh_displacement = displacementsArray[neighQuery.current()];
				Vector3 delta_ref = neighQuery.delta();
				Vector3 delta_cur = delta_ref + neigh_displacement - center_displacement;
				if(affineMapping() == TO_CURRENT_CELL) {
					delta_ref = refToCurTM() * delta_ref;
					delta_cur = refToCurTM() * delta_cur;
				}
				else if(affineMapping() != TO_REFERENCE_CELL) {
					delta_cur = refToCurTM() * delta_cur;
				}
				D2min += (Fftype * delta_ref - delta_cur).squaredLength();
			}

			nonaffineSquaredDisplacementsArray[particleIndex] = D2min;
		}

		// Calculate von Mises shear strain.
		double xydiff = strain.xx() - strain.yy();
		double shearStrain;
		if(!cell()->is2D()) {
			double xzdiff = strain.xx() - strain.zz();
			double yzdiff = strain.yy() - strain.zz();
			shearStrain = sqrt(strain.xy()*strain.xy() + strain.xz()*strain.xz() + strain.yz()*strain.yz() +
					(xydiff*xydiff + xzdiff*xzdiff + yzdiff*yzdiff) / 6.0);
		}
		else {
			shearStrain = sqrt(strain.xy()*strain.xy() + (xydiff*xydiff) / 2.0);
		}
		OVITO_ASSERT(std::isfinite(shearStrain));
		shearStrainsArray[particleIndex] = (FloatType)shearStrain;

		// Calculate volumetric component.
		double volumetricStrain;
		if(!cell()->is2D()) {
			volumetricStrain = (strain(0,0) + strain(1,1) + strain(2,2)) / 3.0;
		}
		else {
			volumetricStrain = (strain(0,0) + strain(1,1)) / 2.0;
		}
		OVITO_ASSERT(std::isfinite(volumetricStrain));
		volumetricStrainsArray[particleIndex] = (FloatType)volumetricStrain;

		if(invalidParticlesArray)
			invalidParticlesArray[particleIndex] = 0;
	});

	// Release data that is no longer needed.
	releaseWorkingData();
	_displacements.reset();
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
void AtomicStrainModifier::AtomicStrainEngine::applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

	if(_inputFingerprint.hasChanged(particles))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	OVITO_ASSERT(shearStrains());
	OVITO_ASSERT(shearStrains()->size() == particles->elementCount());

	if(invalidParticles())
		particles->createProperty(invalidParticles());

	if(strainTensors())
		particles->createProperty(strainTensors());

	if(deformationGradients())
		particles->createProperty(deformationGradients());

	if(nonaffineSquaredDisplacements())
		particles->createProperty(nonaffineSquaredDisplacements());

	if(volumetricStrains())
		particles->createProperty(volumetricStrains());

	if(shearStrains())
		particles->createProperty(shearStrains());

	if(rotations())
		particles->createProperty(rotations());

	if(stretchTensors())
		particles->createProperty(stretchTensors());

	state.addAttribute(QStringLiteral("AtomicStrain.invalid_particle_count"), QVariant::fromValue(numInvalidParticles()), modApp);

	if(numInvalidParticles() != 0)
		state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Could not compute local deformation for %1 particles because of too few neighbors. Increase cutoff radius to include more neighbors.").arg(numInvalidParticles())));
}

}	// End of namespace
}	// End of namespace
