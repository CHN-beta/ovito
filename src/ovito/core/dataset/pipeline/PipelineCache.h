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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/pipeline/PipelineEvaluation.h>

namespace Ovito {

/**
 * \brief A data cache for PipelineFlowState objects, which is used in the implementation of the PipelineSceneNode 
 *        and the CachingPipelineObject class.
 */
class OVITO_CORE_EXPORT PipelineCache
{
public:

	/// Constructor.
	PipelineCache(RefTarget* owner, bool includeVisElements);

	/// Destructor.
	~PipelineCache();

	/// Starts a pipeline evaluation or returns a reference to an existing evaluation that is currently in progress. 
	SharedFuture<PipelineFlowState> evaluatePipeline(const PipelineEvaluationRequest& request);

	/// Performs a synchronous pipeline evaluation.
	const PipelineFlowState& evaluatePipelineSynchronous(const PipelineEvaluationRequest& request);

	/// Performs a synchronous evaluation of a pipeline stage.
	const PipelineFlowState& evaluatePipelineStageSynchronous(const PipelineEvaluationRequest& request);

	/// Looks up the pipeline state for the given animation time.
	const PipelineFlowState& getAt(TimePoint time) const;

	/// Returns the cached results from the last synchronous pipeline evaluation, which is used for interactive viewport rendering.
	const PipelineFlowState& synchronousState() const { return _synchronousState; }

	/// Invalidates the cached results from a synchronous pipeline evaluation.
	void invalidateSynchronousState() { _synchronousState.setStateValidity(TimeInterval::empty()); }

	/// Marks the contents of the cache as outdated and throws away data that is no longer needed.
	void invalidate(TimeInterval keepInterval = TimeInterval::empty(), bool resetSynchronousCache = false);

	/// Special method used by the FileSource class to replace the contents of the pipeline
	/// cache with a data collection modified by the user.
	void overrideCache(const DataCollection* dataCollection, const TimeInterval& keepInterval);

	/// Enables or disables the precomputation and caching of all frames of the animation.
	void setPrecomputeAllFrames(bool enable);
	
private:

	/// Describes a pipeline evaluation that is currently in progress. 
	struct EvaluationInProgress {
		TimeInterval validityInterval;
		WeakSharedFuture<PipelineFlowState> future;
	};

	/// Returns a pointer to the pipeline object that owns this cache.
	RefTarget* ownerObject() const { return _ownerObject; }

	/// Inserts (or may reject) a pipeline state into the cache. 
	void insertState(const PipelineFlowState& state);

	/// Populates the internal cache with transformed data objects generated by transforming visual elements.
	void cacheTransformedDataObjects(const PipelineFlowState& state);

	/// Removes an evaluation record from the list of evaluations currently in progress.
	void cleanupEvaluation(std::forward_list<EvaluationInProgress>::iterator evaluation);

	/// Starts the process of caching the pipeline results for all animation frames.
	void startFramePrecomputation(const PipelineEvaluationRequest& request);

	/// Requests the next frame from the pipeline that needs to be precomputed.
	void precomputeNextAnimationFrame();

	/// The contents of the cache.
	std::vector<PipelineFlowState> _cachedStates;

	/// Results from the last synchronous pipeline evaluation, which is used for interactive viewport rendering.
	PipelineFlowState _synchronousState;

	/// The union of time intervals for which this cache should maintain pipeline states.
	TimeIntervalUnion _requestedIntervals;

	/// The set of activate pipeline evaluations.
	std::forward_list<EvaluationInProgress> _evaluationsInProgress;

	/// A cache with the transformed data objects generated during the last pipeline evaluation.
	std::vector<OORef<TransformedDataObject>> _cachedTransformedDataObjects;

	/// The object this cache belongs to (either a PipelineSceneNode or a CachingPipelineObject).
	RefTarget* _ownerObject;

	/// Include the effect of visual elements in the pipeline evaluation.
	bool _includeVisElements = false;

	/// Enables the precomputation of the pipeline output for all animation frames.
	bool _precomputeAllFrames = false;

	/// Indicates that all frames of the trahectory have been precomputed.
	bool _allFramesPrecomputed = false;

	/// The asynchronous task that precomputes the pipeline output for all animation frames.
	Promise<> _precomputeFramesOperation;

	/// The future for the next precompute frame. 
	SharedFuture<PipelineFlowState> _precomputeFrameFuture;

#ifdef OVITO_DEBUG
	/// While this flag is set, the cache may not be invalidated.
	bool _preparingEvaluation = false; 
#endif
};

}	// End of namespace
