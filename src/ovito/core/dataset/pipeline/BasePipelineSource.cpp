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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/app/Application.h>
#include "BasePipelineSource.h"

namespace Ovito {

IMPLEMENT_OVITO_CLASS(BasePipelineSource);
DEFINE_REFERENCE_FIELD(BasePipelineSource, dataCollection);
DEFINE_PROPERTY_FIELD(BasePipelineSource, dataCollectionFrame);
SET_PROPERTY_FIELD_LABEL(BasePipelineSource, dataCollection, "Data");
SET_PROPERTY_FIELD_LABEL(BasePipelineSource, dataCollectionFrame, "Active frame index");

/******************************************************************************
* Constructor.
******************************************************************************/
BasePipelineSource::BasePipelineSource(DataSet* dataset) : CachingPipelineObject(dataset),
	_dataCollectionFrame(-1)
{
}

/******************************************************************************
* Post-processes the DataCollection generated by the data source and updates 
* the internal master data collection.
******************************************************************************/
Future<PipelineFlowState> BasePipelineSource::postprocessDataCollection(int animationFrame, TimeInterval frameInterval, Future<PipelineFlowState> future)
{
	return std::move(future).then_future(executor(), [this,animationFrame,frameInterval](Future<PipelineFlowState> future) -> PipelineFlowState {
		try {
			OVITO_ASSERT(future.isFinished() && !future.isCanceled());
			PipelineFlowState state = future.result();
			setStatus(state.status());

			// Check if the generated pipeline state is valid.
			if(state.data() && state.status().type() != PipelineStatus::Error) {

				// Create editable proxy objects for the data objects in the generated collection.
				if(Application::instance()->executionContext() == ExecutionContext::Interactive) {
					_updatingEditableProxies = true;
					ConstDataObjectPath dataPath = { state.data() };
					state.data()->updateEditableProxies(state, dataPath);
					_updatingEditableProxies = false;
				}				

				// Adopt the generated data collection as our internal master data collection (only if it is for the current animation time).
				if(state.stateValidity().contains(dataset()->animationSettings()->time())) {
					setDataCollectionFrame(animationFrame);
					setDataCollection(state.data());
					notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
				}
			}

			return state;
		}
		catch(Exception& ex) {
			ex.setContext(dataset());
			setStatus(ex);
			ex.prependGeneralMessage(tr("Pipeline source reported:"));
			return PipelineFlowState(dataCollection(), PipelineStatus(ex, QChar(' ')), frameInterval);
		}
		catch(const std::bad_alloc&) {
			setStatus(PipelineStatus(PipelineStatus::Error, tr("Not enough memory.")));
			return PipelineFlowState(dataCollection(), status(), frameInterval);
		}
		catch(...) {
			OVITO_ASSERT_MSG(false, "BasePipelineSource::evaluateInternal()", "Caught an unexpected exception type during source function execution.");
			setStatus(PipelineStatus(PipelineStatus::Error, tr("Unknown exception caught during execution of pipeline source function.")));
			return PipelineFlowState(dataCollection(), status(), frameInterval);
		}
	});
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool BasePipelineSource::referenceEvent(RefTarget* source, const ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::TargetChanged && source == dataCollection() && !_updatingEditableProxies && !event.sender()->isBeingLoaded()) {
		if(Application::instance()->executionContext() == ExecutionContext::Interactive) {

			// The user has modified one of the editable proxy objects attached to the data collection.
			// Apply the changes made to the proxy objects to the actual data objects.
			UndoSuspender noUndo(this);
			PipelineFlowState state(dataCollection(), PipelineStatus::Success);
			_updatingEditableProxies = true;
			// Temporarily detach data collection from the BasePipelineSource to ignore change signals sent by the data collection.
			setDataCollection(nullptr);
			ConstDataObjectPath dataPath = { state.data() };
			state.data()->updateEditableProxies(state, dataPath);
			// Re-attach the updated data collection to the pipeline source.
			setDataCollection(state.data());
			_updatingEditableProxies = false;

			// Invalidate pipeline cache, except at the current animation time. 
			// Here we use the updated data collection.
			if(dataCollectionFrame() >= 0) {
				pipelineCache().overrideCache(dataCollection(), frameTimeInterval(dataCollectionFrame()));
			}
			// Let downstream pipeline now that its input has changed.
			notifyDependents(ReferenceEvent::PreliminaryStateAvailable);
		}
		else {
			// When the data collection was change by a script, then we simply invalidate the pipeline cache
			// and inform the scene that the pipeline must be re-evaluated.
			pipelineCache().invalidate();
			notifyTargetChanged();
		}
	}
	return CachingPipelineObject::referenceEvent(source, event);
}

/******************************************************************************
* Computes the time interval covered on the timeline by the given source animation frame.
******************************************************************************/
TimeInterval BasePipelineSource::frameTimeInterval(int frame) const
{
	return TimeInterval(
		sourceFrameToAnimationTime(frame),
		std::max(sourceFrameToAnimationTime(frame + 1) - 1, sourceFrameToAnimationTime(frame)));
}

/******************************************************************************
* Throws away the master data collection maintained by the source.
******************************************************************************/
void BasePipelineSource::discardDataCollection()
{
	class ResetDataCollectionOperation : public UndoableOperation 
	{
	private:
		OORef<BasePipelineSource> _source;
	public:
		ResetDataCollectionOperation(BasePipelineSource* source) : _source(source) {}
		virtual void undo() override {
			_source->setDataCollectionFrame(-1);
			_source->pipelineCache().invalidate();
			_source->notifyTargetChanged();
		}
	};

	dataset()->undoStack().pushIfRecording<ResetDataCollectionOperation>(this);

	// Throw away cached frame data and notify pipeline that an update is in order.
	setDataCollection(nullptr);
	setDataCollectionFrame(-1);
	pipelineCache().invalidate();
	notifyTargetChanged();

	dataset()->undoStack().pushIfRecording<ResetDataCollectionOperation>(this);
}

}	// End of namespace