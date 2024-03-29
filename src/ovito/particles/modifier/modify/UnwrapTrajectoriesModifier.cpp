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

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/utilities/concurrent/TaskManager.h>
#include <ovito/core/utilities/concurrent/ForEach.h>
#include "UnwrapTrajectoriesModifier.h"

#include <boost/range/irange.hpp>

namespace Ovito::Particles {

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifier);

IMPLEMENT_OVITO_CLASS(UnwrapTrajectoriesModifierApplication);
SET_MODIFIER_APPLICATION_TYPE(UnwrapTrajectoriesModifier, UnwrapTrajectoriesModifierApplication);

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool UnwrapTrajectoriesModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Modifies the input data.
******************************************************************************/
Future<PipelineFlowState> UnwrapTrajectoriesModifier::evaluate(const ModifierEvaluationRequest& request, const PipelineFlowState& input)
{
	if(input) {
		if(UnwrapTrajectoriesModifierApplication* unwrapModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(request.modApp())) {

			// If the periodic image flags property is present, use it to unwrap particle positions.
			const ParticlesObject* inputParticles = input.expectObject<ParticlesObject>();
			if(inputParticles->getProperty(ParticlesObject::PeriodicImageProperty)) {
				PipelineFlowState output = input;
				unwrapModApp->unwrapParticleCoordinates(request, output);
				return output;
			}

			// Without the periodic image flags information, we have to scan the particle trajectories
			// from beginning to end before making them continuous.
			return unwrapModApp->detectPeriodicCrossings(request).then(unwrapModApp->executor(), [state = input, request]() mutable {
				static_object_cast<UnwrapTrajectoriesModifierApplication>(request.modApp())->unwrapParticleCoordinates(request, state);
				return std::move(state);
			});
		}
	}
	return input;
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void UnwrapTrajectoriesModifier::evaluateSynchronous(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	if(!state) return;

	// The pipeline system may call evaluateSynchronous() with an outdated trajectory frame, which doesn't match the current
	// animation time. This would lead to artifacts, because particles might get unwrapped even though they haven't crossed
	// a periodic cell boundary yet. To avoid this from happening, we try to determine the true animation time of the
	// current input data collection and use it for looking up the unwrap information.
	TimePoint time = request.time();
	int sourceFrame = state.data()->sourceFrame();
	if(sourceFrame != -1)
		time = request.modApp()->sourceFrameToAnimationTime(sourceFrame);

	if(UnwrapTrajectoriesModifierApplication* unwrapModApp = dynamic_object_cast<UnwrapTrajectoriesModifierApplication>(request.modApp())) {
		unwrapModApp->unwrapParticleCoordinates(request, state);
	}
}

/******************************************************************************
* Processes all frames of the input trajectory to detect periodic crossings 
* of the particles.
******************************************************************************/
SharedFuture<> UnwrapTrajectoriesModifierApplication::detectPeriodicCrossings(const ModifierEvaluationRequest& request)
{
	if(_unwrapOperation.isValid() == false || _unwrapOperation.isCanceled()) {

		// Determine the range of animation frames to be processed.
		int startFrame = 0;
		if(unwrappedUpToTime() != TimeNegativeInfinity())
			startFrame = animationTimeToSourceFrame(unwrappedUpToTime());
		int endFrame = std::max(numberOfSourceFrames(), startFrame);
		auto inputFrameRange = boost::irange(startFrame, endFrame);

		// Iterate over all frames of the input range in sequential order.
		_unwrapOperation = for_each_sequential(
			std::move(inputFrameRange), 
			executor(true), // require deferred execution of each frame
			// Requests the next frame from the upstream pipeline.
			[this](int frame) {
				return evaluateInput(PipelineEvaluationRequest(sourceFrameToAnimationTime(frame)));
			},
			// This object processes each frame's data.
			WorkingData{this});

		// Display progress in the UI.
		OVITO_ASSERT(_unwrapOperation.task()->isProgressingTask());
		static_cast<ProgressingTask*>(_unwrapOperation.task().get())->setProgressText(tr("Unwrapping particle trajectories"));
		taskManager().registerFuture(_unwrapOperation);
		registerActiveFuture(_unwrapOperation);
	}
	return _unwrapOperation;
}

/******************************************************************************
* Throws away the precomputed unwrapping information and interrupts
* any computation currently in progress.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::invalidateUnwrapData()
{
	_unwrappedUpToTime = TimeNegativeInfinity();
	_unwrapRecords.clear();
	_unflipRecords.clear();
	_unwrapOperation.reset();
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool UnwrapTrajectoriesModifierApplication::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged && source == input()) {
		invalidateUnwrapData();
	}
	return ModifierApplication::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex)
{
	if(field == PROPERTY_FIELD(input)) {
		invalidateUnwrapData();
	}
	ModifierApplication::referenceReplaced(field, oldTarget, newTarget, listIndex);
}

/******************************************************************************
* Rescales the times of all animation keys from the old animation interval to the new interval.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval)
{
	ModifierApplication::rescaleTime(oldAnimationInterval, newAnimationInterval);
	invalidateUnwrapData();
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::unwrapParticleCoordinates(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	const ParticlesObject* inputParticles = state.expectObject<ParticlesObject>();
	inputParticles->verifyIntegrity();

	// If the periodic image flags particle property is present, use it to unwrap particle positions.
	if(ConstPropertyAccess<Vector3I> particlePeriodicImageProperty = inputParticles->getProperty(ParticlesObject::PeriodicImageProperty)) {
		// Get current simulation cell.
		const SimulationCellObject* cell = state.expectObject<SimulationCellObject>();

		// Make a modifiable copy of the particles object.
		ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

		// Make a modifiable copy of the particle position property.
		PropertyAccess<Point3> posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty);
		const Vector3I* pbcShift = particlePeriodicImageProperty.cbegin();
		for(Point3& p : posProperty) {
			p += cell->cellMatrix() * (*pbcShift++).toDataType<FloatType>();
		}

		// Unwrap bonds by adjusting their PBC shift vectors.
		if(outputParticles->bonds()) {
			if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = outputParticles->bonds()->getProperty(BondsObject::TopologyProperty)) {
				PropertyAccess<Vector3I> periodicImageProperty = outputParticles->makeBondsMutable()->createProperty(BondsObject::PeriodicImageProperty, DataBuffer::InitializeMemory);
				for(size_t bondIndex = 0; bondIndex < topologyProperty.size(); bondIndex++) {
					size_t particleIndex1 = topologyProperty[bondIndex][0];
					size_t particleIndex2 = topologyProperty[bondIndex][1];
					if(particleIndex1 >= particlePeriodicImageProperty.size() || particleIndex2 >= particlePeriodicImageProperty.size())
						continue;
					const Vector3I& particleShift1 = particlePeriodicImageProperty[particleIndex1];
					const Vector3I& particleShift2 = particlePeriodicImageProperty[particleIndex2];
					periodicImageProperty[bondIndex] += particleShift1 - particleShift2;
				}
			}
		}

		// After unwrapping the particles, the PBC image flags are obsolete.
		// It's time to remove the particle property.
		outputParticles->removeProperty(outputParticles->getProperty(ParticlesObject::PeriodicImageProperty));

		state.setStatus(tr("Unwrapping particle positions using stored PBC image information."));

		return;
	}

	// Check if periodic cell boundary crossing have been precomputed or not.
	if(request.time() > unwrappedUpToTime()) {
		if(ExecutionContext::isInteractive())
			state.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Particle crossings of periodic cell boundaries have not been determined yet.")));
		else
			throwException(tr("Particle crossings of periodic cell boundaries have not been determined yet. Cannot unwrap trajectories. Did you forget to call UnwrapTrajectoriesModifier.update()?"));
		return;
	}

	// Reverse any cell shear flips made by LAMMPS.
	if(!unflipRecords().empty() && request.time() >= unflipRecords().front().first) {
		auto iter = unflipRecords().rbegin();
		while(iter->first > request.time()) {
			++iter;
			OVITO_ASSERT(iter != unflipRecords().rend());
		}
		const std::array<int,3>& flipState = iter->second;
		SimulationCellObject* simCellObj = state.expectMutableObject<SimulationCellObject>();
		AffineTransformation cell = simCellObj->cellMatrix();
		cell.column(2) += cell.column(0) * flipState[1] + cell.column(1) * flipState[2];
		cell.column(1) += cell.column(0) * flipState[0];
		simCellObj->setCellMatrix(cell);
	}

	if(unwrapRecords().empty())
		return;

	// Get current simulation cell.
	const SimulationCellObject* simCell = state.expectObject<SimulationCellObject>();

	// Make a modifiable copy of the particles object.
	ParticlesObject* outputParticles = state.expectMutableObject<ParticlesObject>();

	// Make a modifiable copy of the particle position property.
	PropertyAccess<Point3> posProperty = outputParticles->expectMutableProperty(ParticlesObject::PositionProperty);

	// Get particle identifiers.
	ConstPropertyAccess<qlonglong> identifierProperty = outputParticles->getProperty(ParticlesObject::IdentifierProperty);
	if(identifierProperty && identifierProperty.size() != posProperty.size())
		identifierProperty.reset();

	// Compute unwrapped particle coordinates.
	qlonglong index = 0;
	for(Point3& p : posProperty) {
		auto range = unwrapRecords().equal_range(identifierProperty ? identifierProperty[index] : index);
		bool shifted = false;
		Vector3 pbcShift = Vector3::Zero();
		for(auto iter = range.first; iter != range.second; ++iter) {
			if(std::get<0>(iter->second) <= request.time()) {
				pbcShift[std::get<1>(iter->second)] += std::get<2>(iter->second);
				shifted = true;
			}
		}
		if(shifted) {
			p += simCell->matrix() * pbcShift;
		}
		index++;
	}

	// Unwrap bonds by adjusting their PBC shift vectors.
	if(outputParticles->bonds()) {
		if(ConstPropertyAccess<ParticleIndexPair> topologyProperty = outputParticles->bonds()->getProperty(BondsObject::TopologyProperty)) {
			PropertyAccess<Vector3I> periodicImageProperty = outputParticles->makeBondsMutable()->createProperty(BondsObject::PeriodicImageProperty, DataBuffer::InitializeMemory);
			for(size_t bondIndex = 0; bondIndex < topologyProperty.size(); bondIndex++) {
				size_t particleIndex1 = topologyProperty[bondIndex][0];
				size_t particleIndex2 = topologyProperty[bondIndex][1];
				if(particleIndex1 >= posProperty.size() || particleIndex2 >= posProperty.size())
					continue;

				Vector3I& pbcShift = periodicImageProperty[bondIndex];
				auto range1 = unwrapRecords().equal_range(identifierProperty ? identifierProperty[particleIndex1] : particleIndex1);
				auto range2 = unwrapRecords().equal_range(identifierProperty ? identifierProperty[particleIndex2] : particleIndex2);
				for(auto iter = range1.first; iter != range1.second; ++iter) {
					if(std::get<0>(iter->second) <= request.time()) {
						pbcShift[std::get<1>(iter->second)] += std::get<2>(iter->second);
					}
				}
				for(auto iter = range2.first; iter != range2.second; ++iter) {
					if(std::get<0>(iter->second) <= request.time()) {
						pbcShift[std::get<1>(iter->second)] -= std::get<2>(iter->second);
					}
				}
			}
		}
	}
}

/******************************************************************************
* Calculates the information that is needed to unwrap particle coordinates.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::WorkingData::operator()(int frame, const PipelineFlowState& state)
{
	TimePoint time = _modApp->sourceFrameToAnimationTime(frame);

	// Get simulation cell geometry and boundary conditions.
	const SimulationCellObject* cell = state.getObject<SimulationCellObject>();
	if(!cell)
		_modApp->throwException(tr("Input data contains no simulation cell information at frame %1.").arg(frame));
	if(!cell->hasPbcCorrected())
		_modApp->throwException(tr("No periodic boundary conditions set for the simulation cell."));
	AffineTransformation reciprocalCellMatrix = cell->inverseMatrix();

	const ParticlesObject* particles = state.getObject<ParticlesObject>();
	if(!particles)
		_modApp->throwException(tr("Input data contains no particles at frame %1.").arg(frame));
	ConstPropertyAccess<Point3> posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<qlonglong> identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	if(identifierProperty && identifierProperty.size() != posProperty.size())
		identifierProperty.reset();

	// Special handling of cell flips in LAMMPS, which occur whenever a tilt factor exceeds +/-50%.
	if(cell->matrix()(1,0) == 0 && cell->matrix()(2,0) == 0 && cell->matrix()(2,1) == 0 && cell->matrix()(0,0) > 0 && cell->matrix()(1,1) > 0) {
		if(_previousCell) {
			std::array<int,3> flipState = _currentFlipState;
			// Detect discontinuities in the three tilt factors of the cell.
			if(cell->hasPbc(0)) {
				FloatType xy1 = _previousCell->matrix()(0,1) / _previousCell->matrix()(0,0);
				FloatType xy2 = cell->matrix()(0,1) / cell->matrix()(0,0);
				if(int flip_xy = (int)std::round(xy2 - xy1))
					flipState[0] -= flip_xy;
				if(!cell->is2D()) {
					FloatType xz1 = _previousCell->matrix()(0,2) / _previousCell->matrix()(0,0);
					FloatType xz2 = cell->matrix()(0,2) / cell->matrix()(0,0);
					if(int flip_xz = (int)std::round(xz2 - xz1))
						flipState[1] -= flip_xz;
				}
			}
			if(cell->hasPbc(1) && !cell->is2D()) {
				FloatType yz1 = _previousCell->matrix()(1,2) / _previousCell->matrix()(1,1);
				FloatType yz2 = cell->matrix()(1,2) / cell->matrix()(1,1);
				if(int flip_yz = (int)std::round(yz2 - yz1))
					flipState[2] -= flip_yz;
			}
			// Emit a timeline record whever a flipping occurred.
			if(flipState != _currentFlipState)
				_modApp->_unflipRecords.emplace_back(time, flipState);
			_currentFlipState = flipState;
		}
		_previousCell = cell;
		// Unflip current simulation cell.
		if(_currentFlipState != std::array<int,3>{{0,0,0}}) {
			AffineTransformation newCellMatrix = cell->matrix();
			newCellMatrix(0,1) += cell->matrix()(0,0) * _currentFlipState[0];
			newCellMatrix(0,2) += cell->matrix()(0,0) * _currentFlipState[1];
			newCellMatrix(1,2) += cell->matrix()(1,1) * _currentFlipState[2];
			reciprocalCellMatrix = newCellMatrix.inverse();
		}
	}

	qlonglong index = 0;
	for(const Point3& p : posProperty) {
		Point3 rp = reciprocalCellMatrix * p;
		// Try to insert new position of particle into map.
		// If an old position already exists, insertion will fail and we can
		// test whether the particle did cross a periodic cell boundary.
		auto result = _previousPositions.insert(std::make_pair(identifierProperty ? identifierProperty[index] : index, rp));
		if(!result.second) {
			Vector3 delta = result.first->second - rp;
			for(size_t dim = 0; dim < 3; dim++) {
				if(cell->hasPbcCorrected(dim)) {
					int shift = (int)std::round(delta[dim]);
					if(shift != 0) {
						// Create a new record when particle has crossed a periodic cell boundary.
						_modApp->_unwrapRecords.emplace(result.first->first, std::make_tuple(time, (qint8)dim, (qint16)shift));
					}
				}
			}
			result.first->second = rp;
		}
		index++;
	}

	_modApp->_unwrappedUpToTime = time;
	_modApp->setStatus(tr("Processed input trajectory frame %1 of %2.").arg(frame).arg(_modApp->numberOfSourceFrames()));
}

/******************************************************************************
* Saves the class' contents to an output stream.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const
{
	ModifierApplication::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	stream << unwrappedUpToTime();
	stream.endChunk();
	stream.beginChunk(0x02);
	stream.writeSizeT(unwrapRecords().size());
	for(const auto& item : unwrapRecords()) {
		OVITO_STATIC_ASSERT((std::is_same<qlonglong, qint64>::value));
		stream << item.first;
		stream << std::get<0>(item.second);
		stream << std::get<1>(item.second);
		stream << std::get<2>(item.second);
	}
	stream.writeSizeT(unflipRecords().size());
	for(const auto& item : unflipRecords()) {
		stream << item.first;
		stream << std::get<0>(item.second);
		stream << std::get<1>(item.second);
		stream << std::get<2>(item.second);
	}
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from an input stream.
******************************************************************************/
void UnwrapTrajectoriesModifierApplication::loadFromStream(ObjectLoadStream& stream)
{
	ModifierApplication::loadFromStream(stream);
	stream.expectChunk(0x02);
	stream >> _unwrappedUpToTime;
	stream.closeChunk();
	int version = stream.expectChunkRange(0x01, 1);
	size_t numItems = stream.readSizeT();
	_unwrapRecords.reserve(numItems);
	for(size_t i = 0; i < numItems; i++) {
		UnwrapData::key_type particleId;
		std::tuple_element_t<0, UnwrapData::mapped_type> time;
		std::tuple_element_t<1, UnwrapData::mapped_type> dim;
		std::tuple_element_t<2, UnwrapData::mapped_type> direction;
		stream >> particleId >> time >> dim >> direction;
		_unwrapRecords.emplace(particleId, std::make_tuple(time, dim, direction));
	}
	if(version >= 1) {
		stream.readSizeT(numItems);
		_unflipRecords.reserve(numItems);
		for(size_t i = 0; i < numItems; i++) {
			UnflipData::value_type item;
			stream >> item.first >> item.second[0] >> item.second[1] >> item.second[2];
			_unflipRecords.push_back(item);
		}
	}
	stream.closeChunk();
}

}	// End of namespace
