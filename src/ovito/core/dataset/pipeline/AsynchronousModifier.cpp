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
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/DataSetContainer.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifierApplication.h>
#include <ovito/core/app/Application.h>
#include "AsynchronousModifier.h"

#ifdef Q_OS_LINUX
	#include <malloc.h>
#endif

namespace Ovito {


// Export this class template specialization from the DLL under Windows.
template class OVITO_CORE_EXPORT Future<AsynchronousModifier::EnginePtr>;

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AsynchronousModifier::AsynchronousModifier(DataSet* dataset) : Modifier(dataset)
{
}

/******************************************************************************
* Asks the object for the result of the data pipeline.
******************************************************************************/
Future<PipelineFlowState> AsynchronousModifier::evaluate(const ModifierEvaluationRequest& request, const PipelineFlowState& input)
{
	// Get the modifier application, which stores cached computation results.
	const AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(request.modApp());
	if(!asyncModApp) 
		return Future<PipelineFlowState>::createFailed(Exception(tr("Wrong type of modifier application.")));

	// Check if there is an existing computation result that can be reused as is.
	if(const EnginePtr& engine = asyncModApp->completedEngine()) {
		if(engine->validityInterval().contains(request.time())) {
			// Inject the cached computation result into the pipeline.
			UndoSuspender noUndo(this);
			PipelineFlowState output = input;
			engine->applyResults(request, output);
			output.intersectStateValidity(engine->validityInterval());
			return output;
		}
	}

	// Asynchrounous task managing the execution of the compute engine(s).
	class EngineExecutionTask : public Task 
	{
	public:

		/// Constructor.
		EngineExecutionTask(const ModifierEvaluationRequest& request, EnginePtr engine, const PipelineFlowState& state, std::vector<EnginePtr> validStages = {}) : 
				Task(Task::Started, &request.dataset()->taskManager()),
				_request(request),
				_modApp(static_object_cast<AsynchronousModifierApplication>(request.modApp())),
				_engine(std::move(engine)),
				_state(state),
				_validStages(std::move(validStages)) {}

		/// Creates a future returning the results of this asynchronous task to the caller.
		Future<PipelineFlowState> future() {
			return Future<PipelineFlowState>::createFromTask(shared_from_this(), _state);
		}

		/// Is called when this task gets canceled by the system.
		virtual void cancel() noexcept override {
			_executionFuture.reset(); // Cancel compute engine that is currently running.
			Task::cancel();
			setFinished();
		}

		/// Starts running the next compute engine.
		void go() {
			OVITO_ASSERT(_modApp);
			OVITO_ASSERT(!isCanceled());
			OVITO_ASSERT(_engine);
			
			// Restrict the validity interval of the engine to the validity interval of the input pipeline state.
			TimeInterval iv = _engine->validityInterval();
			iv.intersect(_state.stateValidity());
			_engine->setValidityInterval(iv);

			_validStages.push_back(_engine);
			bool asynchronousMode = !_engine->preferSynchronousExecution();
			if(asynchronousMode)
				_executionFuture = taskManager()->runTaskAsync(_engine);
			else
				_executionFuture = taskManager()->runTaskSync(_engine);
			_executionFuture.finally(_modApp->executor(), asynchronousMode, 
				std::bind(&EngineExecutionTask::executionFinished, static_pointer_cast<EngineExecutionTask>(shared_from_this()), std::placeholders::_1));
		}

		/// Is called by the system when the current compute engine finishes.
		void executionFinished(const TaskPtr& task) {
			OVITO_ASSERT(task == _engine);
			_executionFuture.reset();
			if(!isCanceled() && !task->isCanceled()) {
				if(task->exceptionStore()) {
					setException(task->exceptionStore());
					setFinished();
				}
				else {
					// Ask the compute engine for a continuation engine.
					if(EnginePtr continuationEngine = _engine->createContinuationEngine(_request, _state)) {

						// Restrict the validity of the continuation engine to the validity interval of the parent engine.
						TimeInterval iv = continuationEngine->validityInterval();
						iv.intersect(_engine->validityInterval());
						continuationEngine->setValidityInterval(iv);

						// Repeat the cycle with the new engine.
						_engine = std::move(continuationEngine);
						go();
					}
					else {
						// If the current engine has no continuation, we are done.

						// Add the computed results to the input pipeline state.
						_engine->applyResults(_request, _state);
						_state.intersectStateValidity(_engine->validityInterval());
						_modApp->setCompletedEngine(std::move(_engine));
						_modApp->setValidStages(std::move(_validStages));
#ifdef OVITO_DEBUG
						this->_resultSet = true;
#endif
						setFinished();
					}
				}
			}
			else cancel();
		}

	private:

		ModifierEvaluationRequest _request;
		OORef<AsynchronousModifierApplication> _modApp;
		EnginePtr _engine;
		SharedFuture<> _executionFuture;
		std::vector<EnginePtr> _validStages;
		PipelineFlowState _state;
	};

	// Check if there are any partially completed computation results that can serve as starting point for a new computation.
	if(!asyncModApp->validStages().empty() && asyncModApp->validStages().back()->validityInterval().contains(request.time())) {
		// Create the asynchronous task object and continue the execution of engines.
		auto task = std::make_shared<EngineExecutionTask>(request, asyncModApp->validStages().back(), input, asyncModApp->validStages());
		task->executionFinished(asyncModApp->validStages().back());
		return task->future();
	}
	else {
		// Otherwise, ask the subclass to create a new compute engine to perform the computation from scratch.
		return createEngine(request, input).then(executor(), [this, request = request, input = input, modApp = QPointer<const AsynchronousModifierApplication>(asyncModApp)](EnginePtr engine) mutable {
			if(!modApp || modApp->modifier() != this)
				throw Exception(tr("Modifier has been deleted from the pipeline."));
			// Create the asynchronous task object and start running the engine.
			auto task = std::make_shared<EngineExecutionTask>(std::move(request), std::move(engine), std::move(input));
			task->go();
			return task->future();
		});
	}
}

/******************************************************************************
* Modifies the input data synchronously.
******************************************************************************/
void AsynchronousModifier::evaluateSynchronous(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	OVITO_ASSERT(!dataset()->undoStack().isRecording());

	// If results are still available from the last pipeline evaluation, apply them to the input data.
	applyCachedResultsSynchronous(request, state);
	
	// Call base implementation.
	Modifier::evaluateSynchronous(request, state);
}

/******************************************************************************
* This function is called from AsynchronousModifier::evaluateSynchronous() to 
* apply the results from the last asycnhronous compute engine during a 
* synchronous pipeline evaluation.
******************************************************************************/
bool AsynchronousModifier::applyCachedResultsSynchronous(const ModifierEvaluationRequest& request, PipelineFlowState& state)
{
	// If results are still available from the last pipeline evaluation, apply them to the input data.
	if(const AsynchronousModifierApplication* asyncModApp = dynamic_object_cast<AsynchronousModifierApplication>(request.modApp())) {
		if(const AsynchronousModifier::EnginePtr& engine = asyncModApp->completedEngine()) {
			UndoSuspender noUndo(this);
			engine->applyResults(request, state);
			state.intersectStateValidity(engine->validityInterval());
			return true;
		}
	}
	return false;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void AsynchronousModifier::saveToStream(ObjectSaveStream& stream, bool excludeRecomputableData) const
{
	Modifier::saveToStream(stream, excludeRecomputableData);
	stream.beginChunk(0x02);
	// Chunk reserved for future use.
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void AsynchronousModifier::loadFromStream(ObjectLoadStream& stream)
{
	Modifier::loadFromStream(stream);
	stream.expectChunk(0x02);
	// Chunk reserved for future use.
	stream.closeChunk();
}

#ifdef Q_OS_LINUX
/******************************************************************************
* Destructor.
******************************************************************************/
AsynchronousModifier::Engine::~Engine()
{
	// Some engines allocate considerable amounts of memory in small chunks,
	// which is sometimes not released back to the OS by the C memory allocator.
	// This call to malloc_trim() will explicitly trigger an attempt to release free memory
	// at the top of the heap.
	::malloc_trim(0);
}
#endif

}	// End of namespace
