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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/utilities/concurrent/Future.h>
#include <ovito/core/app/Application.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(ModifierApplication);
DEFINE_REFERENCE_FIELD(ModifierApplication, modifier);
DEFINE_REFERENCE_FIELD(ModifierApplication, input);
SET_PROPERTY_FIELD_LABEL(ModifierApplication, modifier, "Modifier");
SET_PROPERTY_FIELD_LABEL(ModifierApplication, input, "Input");
SET_PROPERTY_FIELD_CHANGE_EVENT(ModifierApplication, input, ReferenceEvent::PipelineChanged);

/******************************************************************************
* Returns the global class registry, which allows looking up the
* ModifierApplication subclass for a Modifier subclass.
******************************************************************************/
ModifierApplication::Registry& ModifierApplication::registry()
{
	static Registry singleton;
	return singleton;
}

/******************************************************************************
* Constructor.
******************************************************************************/
ModifierApplication::ModifierApplication(DataSet* dataset) : CachingPipelineObject(dataset)
{
}

/******************************************************************************
* Determines the time interval over which a computed pipeline state will remain valid.
******************************************************************************/
TimeInterval ModifierApplication::validityInterval(const PipelineEvaluationRequest& request) const
{
	TimeInterval iv = CachingPipelineObject::validityInterval(request);

	// Take into account the validity interval of the input state.
	if(input())
		iv.intersect(input()->validityInterval(request));

	// Let the modifier determine the local validity interval.
	if(modifier() && modifier()->isEnabled()) 
		iv.intersect(modifier()->validityInterval(request, this));

	return iv;
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool ModifierApplication::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetEnabledOrDisabled && source == modifier()) {
		// If modifier provides animation frames, the animation interval might change when the
		// modifier gets enabled/disabled.
		if(!isBeingLoaded())
			notifyDependents(ReferenceEvent::AnimationFramesChanged);

		if(!modifier()->isEnabled()) {
			// Ignore modifier's status if it is currently disabled.
			setStatus(PipelineStatus(PipelineStatus::Success, tr("Modifier is currently turned off.")));
			// Also clear pipeline cache in order to reduce memory footprint when modifier is disabled.
			pipelineCache().invalidate(TimeInterval::empty(), true);
		}
		// Propagate enabled/disabled notification events from the modifier.
		return true;
	}
	else if(event.type() == ReferenceEvent::TitleChanged && source == modifier()) {
		return true;
	}
	else if(event.type() == ReferenceEvent::PipelineChanged && source == input()) {
		// Propagate pipeline changed events and updates to the preliminary state from upstream.
		return true;
	}
	else if(event.type() == ReferenceEvent::AnimationFramesChanged && (source == input() || source == modifier()) && !isBeingLoaded()) {
		// Propagate animation interval events from the modifier or the upstream pipeline.
		return true;
	}
	else if(event.type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached results when the modifier or the upstream pipeline change.
		TimeInterval validityInterval = static_cast<const TargetChangedEvent&>(event).unchangedInterval();

		// Let the modifier reduce the remaining validity interval if the modifier depends on other animation times.
		if(modifier() && source == input())
			modifier()->restrictInputValidityInterval(validityInterval);

		// Propagate change event to upstream pipeline.
		// Note that this will invoke ModifierApplication::notifyDependentsImpl(), which
		// takes care of invalidating the pipeline cache.
		notifyTargetChangedOutsideInterval(validityInterval);

		// Trigger a preliminary viewport update if desired by the modifier.
		if(source == modifier() && modifier()->performPreliminaryUpdateAfterChange()) {
			notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
		}

		return false;
	}
	else if(event.type() == ReferenceEvent::PreliminaryStateAvailable && source == input()) {
		pipelineCache().invalidateSynchronousState();
		// Inform modifier that the input state has changed.
		if(modifier())
			modifier()->notifyDependents(ReferenceEvent::ModifierInputChanged);
	}
	return CachingPipelineObject::referenceEvent(source, event);
}

/******************************************************************************
* Gets called when the data object of the node has been replaced.
******************************************************************************/
void ModifierApplication::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(modifier)) {
		// Reset all caches when the modifier is replaced.
		pipelineCache().invalidate(TimeInterval::empty(), true);

		// Update the status of the Modifier when it is detached from the ModifierApplication.
		if(Modifier* oldMod = static_object_cast<Modifier>(oldTarget)) {
			oldMod->notifyDependents(ReferenceEvent::ObjectStatusChanged);
			oldMod->notifyDependents(ReferenceEvent::ModifierInputChanged);
		}
		if(Modifier* newMod = static_object_cast<Modifier>(newTarget)) {
			newMod->notifyDependents(ReferenceEvent::ObjectStatusChanged);
			newMod->notifyDependents(ReferenceEvent::ModifierInputChanged);
		}
		notifyDependents(ReferenceEvent::TargetEnabledOrDisabled);

		// The animation length might have changed when the modifier has changed.
		if(!isBeingLoaded())
			notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}
	else if(field == PROPERTY_FIELD(input) && !isBeingLoaded()) {
		// Reset all caches when the data input is replaced.
		pipelineCache().invalidate(TimeInterval::empty(), true);
		// Update the status of the Modifier when ModifierApplication is inserted/removed into pipeline.
		if(modifier())
			modifier()->notifyDependents(ReferenceEvent::ModifierInputChanged);
		// The animation length might have changed when the pipeline has changed.
		notifyDependents(ReferenceEvent::AnimationFramesChanged);
	}

	CachingPipelineObject::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Sends an event to all dependents of this RefTarget.
******************************************************************************/
void ModifierApplication::notifyDependentsImpl(const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged) {
		// Invalidate cached results when this modifier application or the modifier changes.
		pipelineCache().invalidate(static_cast<const TargetChangedEvent&>(event).unchangedInterval());
	}
	CachingPipelineObject::notifyDependentsImpl(event);
}

/******************************************************************************
* Asks the object for the result of the upstream data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> ModifierApplication::evaluateInput(const PipelineEvaluationRequest& request)
{
	// Without a data source, this ModifierApplication doesn't produce any data.
	if(!input())
		return PipelineFlowState();

	// Request the input data.
	return input()->evaluate(request);
}

/******************************************************************************
*  Asks the object for the result of the upstream data pipeline at several animation times.
******************************************************************************/
Future<std::vector<PipelineFlowState>> ModifierApplication::evaluateInputMultiple(const PipelineEvaluationRequest& request, std::vector<TimePoint> times)
{
	// Without a data source, this ModifierApplication doesn't produce any data.
	if(!input())
		return std::vector<PipelineFlowState>(times.size(), PipelineFlowState());

	// Request the input data.
	return input()->evaluateMultiple(request, std::move(times));
}

/******************************************************************************
* Returns the results of an immediate and preliminary evaluation of the data pipeline.
******************************************************************************/
PipelineFlowState ModifierApplication::evaluateSynchronous(TimePoint time)
{
	// If modifier is disabled, bypass cache and forward results of upstream pipeline.
	if(input() && (!modifier() || modifier()->isEnabled() == false))
		return input()->evaluateSynchronous(time);

	return CachingPipelineObject::evaluateSynchronous(time);
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
SharedFuture<PipelineFlowState> ModifierApplication::evaluate(const PipelineEvaluationRequest& request)
{
	// If modifier is disabled, bypass cache and forward results of upstream pipeline.
	if(input() && (!modifier() || modifier()->isEnabled() == false))
		return input()->evaluate(request);
	
	// Otherwise, let the base class call our evaluateInternal() method.
	return CachingPipelineObject::evaluate(request);
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> ModifierApplication::evaluateInternal(const PipelineEvaluationRequest& request)
{
	// Set up the evaluation request for the upstream pipeline.
	PipelineEvaluationRequest upstreamRequest = request;

	// Ask the modifier for the set of animation time intervals that should be cached by the upstream pipeline.
	if(modifier() && modifier()->isEnabled())
		modifier()->inputCachingHints(upstreamRequest.modifiableCachingIntervals(), this);

	// Obtain input data and pass it on to the modifier.
	return evaluateInput(upstreamRequest)
		.then(executor(), [this, upstreamRequest](PipelineFlowState inputData) -> Future<PipelineFlowState> {

			// Clear the status of the input unless it is an error.
			if(inputData.status().type() != PipelineStatus::Error) {
				inputData.setStatus(PipelineStatus());
			}
			else if(upstreamRequest.breakOnError()) {
				// Skip all following modifiers once an error has occured along the pipeline.
				return inputData;
			}

			// Without a modifier, this ModifierApplication becomes a no-op.
			// The same is true when the Modifier is disabled or if the input data is invalid.
			if(!modifier() || !modifier()->isEnabled() || !inputData)
				return inputData;

			Future<PipelineFlowState> future;
			try {
				// Let the modifier do its job.
				future = modifier()->evaluate(upstreamRequest, this, inputData);
				// Register the task with this pipeline stage.
				registerActiveFuture(future);
			}
			catch(...) {
				future = Future<PipelineFlowState>::createFailed(std::current_exception());
			}

			// Post-process the modifier results before returning them to the caller.
			// Turn any exception that was thrown during modifier evaluation into a
			// valid pipeline state with an error code.
			return future.then_future(executor(), [this, time = upstreamRequest.time(), inputData = std::move(inputData)](Future<PipelineFlowState> future) mutable {
				OVITO_ASSERT(future.isFinished());
				OVITO_ASSERT(!future.isCanceled());
				try {
					try {
						PipelineFlowState state = future.result();
						if(inputData.status().type() != PipelineStatus::Error || state.status().type() == PipelineStatus::Success)
							setStatus(state.status());
						else
							setStatus(PipelineStatus());
						return state;
					}
					catch(const Exception&) {
						throw;
					}
					catch(const std::bad_alloc&) {
						throwException(tr("Not enough memory."));
					}
					catch(const std::exception& ex) {
						qWarning() << "WARNING: Modifier" << modifier() << "has thrown a non-standard exception:" << ex.what();
						OVITO_ASSERT(false);
						throwException(tr("Exception: %1").arg(QString::fromLatin1(ex.what())));
					}
				}
				catch(Exception& ex) {
					setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar('\n'))));
					ex.prependGeneralMessage(tr("Modifier '%1' reported:").arg(modifier()->objectTitle()));
					inputData.setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar(' '))));
					return std::move(inputData);
				}
				catch(...) {
					OVITO_ASSERT_MSG(false, "ModifierApplication::evaluate()", "Caught an unexpected exception type during modifier evaluation.");
					PipelineStatus status(PipelineStatus::Error, tr("Unknown exception caught during evaluation of modifier '%1'.").arg(modifier()->objectTitle()));
					setStatus(status);
					inputData.setStatus(status);
					return std::move(inputData);
				}
			});
		});
}

/******************************************************************************
* Lets the pipeline stage compute a preliminary result in a synchronous fashion.
******************************************************************************/
PipelineFlowState ModifierApplication::evaluateInternalSynchronous(TimePoint time)
{
	PipelineFlowState state;
	
	if(input()) {
		UndoSuspender noUndo(this);
		// First get the preliminary results from the upstream pipeline.
		state = input()->evaluateSynchronous(time);
		try {
			if(!state)
				throwException(tr("Modifier input is empty."));

			// Apply modifier:
			if(modifier() && modifier()->isEnabled())
				modifier()->evaluateSynchronous(time, this, state);
		}
		catch(const Exception& ex) {
			// Turn exceptions thrown during modifier evaluation into an error pipeline state.
			state.setStatus(PipelineStatus(PipelineStatus::Error, ex.messages().join(QStringLiteral(": "))));
		}
		catch(const std::bad_alloc&) {
			// Turn exceptions thrown during modifier evaluation into an error pipeline state.
			state.setStatus(PipelineStatus(PipelineStatus::Error, tr("Not enough memory.")));
		}
		catch(const std::exception& ex) {
			qWarning() << "WARNING: Modifier" << modifier() << "has thrown a non-standard exception:" << ex.what();
			OVITO_ASSERT(false);
			state.setStatus(PipelineStatus(PipelineStatus::Error, tr("Exception: %1").arg(QString::fromLatin1(ex.what()))));
		}
		catch(...) {
			OVITO_ASSERT_MSG(false, "ModifierApplication::evaluateSynchronous()", "Caught an unexpected exception type during preliminary modifier evaluation.");
			// Turn exceptions thrown during modifier evaluation into an error pipeline state.
			state.setStatus(PipelineStatus(PipelineStatus::Error, tr("Unknown exception caught during evaluation of modifier '%1'.").arg(modifier()->objectTitle())));
		}
	}

	return state;
}

/******************************************************************************
* Returns the number of animation frames this pipeline object can provide.
******************************************************************************/
int ModifierApplication::numberOfSourceFrames() const
{
	int n = input() ? input()->numberOfSourceFrames() : CachingPipelineObject::numberOfSourceFrames();
	if(modifier() && modifier()->isEnabled())
		n = modifier()->numberOfSourceFrames(n);
	return n;
}

/******************************************************************************
* Given an animation time, computes the source frame to show.
******************************************************************************/
int ModifierApplication::animationTimeToSourceFrame(TimePoint time) const
{
	int frame = input() ? input()->animationTimeToSourceFrame(time) : CachingPipelineObject::animationTimeToSourceFrame(time);
	if(modifier() && modifier()->isEnabled())
		frame = modifier()->animationTimeToSourceFrame(time, frame);
	return frame;
}

/******************************************************************************
* Given a source frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint ModifierApplication::sourceFrameToAnimationTime(int frame) const
{
	TimePoint time = input() ? input()->sourceFrameToAnimationTime(frame) : CachingPipelineObject::sourceFrameToAnimationTime(frame);
	if(modifier() && modifier()->isEnabled())
		time = modifier()->sourceFrameToAnimationTime(frame, time);
	return time;
}

/******************************************************************************
* Returns the human-readable labels associated with the animation frames.
******************************************************************************/
QMap<int, QString> ModifierApplication::animationFrameLabels() const
{
	QMap<int, QString> labels = input() ? input()->animationFrameLabels() : CachingPipelineObject::animationFrameLabels();
	if(modifier() && modifier()->isEnabled())
		return modifier()->animationFrameLabels(std::move(labels));
	return labels;
}

/******************************************************************************
* Traverses the pipeline from this modifier application up to the source and
* returns the source object that generates the input data for the pipeline.
******************************************************************************/
PipelineObject* ModifierApplication::pipelineSource() const
{
	PipelineObject* obj = input();
	while(obj) {
		if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(obj))
			obj = modApp->input();
		else
			break;
	}
	return obj;
}

}	// End of namespace
