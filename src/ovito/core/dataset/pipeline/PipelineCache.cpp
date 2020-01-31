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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/PipelineCache.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/utilities/concurrent/Future.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Determines whether the cache contains a cached pipeline state for the
* given animation time.
******************************************************************************/
bool PipelineCache::contains(TimePoint time, bool onlyCurrentAnimTime) const
{
	if(onlyCurrentAnimTime)
		return _currentAnimState.stateValidity().contains(time);
	else
		return _mostRecentState.stateValidity().contains(time) || _currentAnimState.stateValidity().contains(time);
}

/******************************************************************************
* Returns a state from this cache that is valid at the given animation time.
* If the cache contains no state for the given animation time, then an empty
* pipeline state is returned.
******************************************************************************/
const PipelineFlowState& PipelineCache::getAt(TimePoint time) const
{
	if(_mostRecentState.stateValidity().contains(time)) {
		return _mostRecentState;
	}
	else if(_currentAnimState.stateValidity().contains(time)) {
		return _currentAnimState;
	}
	else {
		const static PipelineFlowState emptyState;
		return emptyState;
	}
}

/******************************************************************************
* Puts the given pipeline state into the cache for later retrieval.
* The cache may decide not to cache the state, in which case the method returns
* false.
******************************************************************************/
bool PipelineCache::insert(PipelineFlowState state, const RefTarget* ownerObject)
{
	state.intersectStateValidity(_restrictedValidityOfNextInsertedState);
	if(state.stateValidity().isEmpty()) {
		qDebug() << "PipelineCache::insert: rejecting state";
		return false;
	}

	OVITO_ASSERT(ownerObject);
	if(state.stateValidity().contains(ownerObject->dataset()->animationSettings()->time()))
		_currentAnimState = state;
	_mostRecentState = std::move(state);
	ownerObject->notifyDependents(ReferenceEvent::PipelineCacheUpdated);
	return true;
}

/******************************************************************************
* Puts the given pipeline state into the cache when it comes available.
* Depending on the given state validity interval, the cache may decide not to
* cache the state.
******************************************************************************/
void PipelineCache::insert(Future<PipelineFlowState>& stateFuture, const TimeInterval& validityInterval, const RefTarget* ownerObject)
{
	// Wait for computation to complete, then cache the results.
	stateFuture = stateFuture.then(ownerObject->executor(), [this, ownerObject](PipelineFlowState&& state) {
		insert(state, ownerObject);
		return std::move(state);
	});
}

/******************************************************************************
* Marks the cached state as stale, but holds on to it.
******************************************************************************/
void PipelineCache::invalidate(bool keepStaleContents, TimeInterval keepInterval)
{
	// Reduce the cache validity to the interval to be kept.
	_mostRecentState.intersectStateValidity(keepInterval);
	_currentAnimState.intersectStateValidity(keepInterval);

	// If the remaining validity interval is empty, we can clear the caches.
	if(_mostRecentState.stateValidity().isEmpty())
		_mostRecentState.reset();
	if(_currentAnimState.stateValidity().isEmpty() && !keepStaleContents)
		_currentAnimState.reset();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
