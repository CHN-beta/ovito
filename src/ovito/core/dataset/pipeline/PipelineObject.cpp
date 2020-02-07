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
#include <ovito/core/dataset/pipeline/PipelineObject.h>
#include <ovito/core/dataset/pipeline/ModifierApplication.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include <ovito/core/dataset/scene/PipelineSceneNode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_OVITO_CLASS(PipelineObject);

/******************************************************************************
* Constructor.
******************************************************************************/
PipelineObject::PipelineObject(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Returns a list of pipeline nodes that have this object in their pipeline.
******************************************************************************/
QSet<PipelineSceneNode*> PipelineObject::pipelines(bool onlyScenePipelines) const
{
	QSet<PipelineSceneNode*> pipelineList;
	for(RefMaker* dependent : this->dependents()) {
		if(PipelineObject* pobj = dynamic_object_cast<PipelineObject>(dependent)) {
			pipelineList.unite(pobj->pipelines(onlyScenePipelines));
		}
		else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
            if(pipeline->dataProvider() == this) {
				if(!onlyScenePipelines || pipeline->isInScene())
		    		pipelineList.insert(pipeline);
			}
		}
	}
	return pipelineList;
}

/******************************************************************************
* Determines whether the data pipeline branches above this pipeline object,
* i.e. whether this pipeline object has multiple dependents, all using this pipeline
* object as input.
******************************************************************************/
bool PipelineObject::isPipelineBranch(bool onlyScenePipelines) const
{
	int pipelineCount = 0;
	for(RefMaker* dependent : this->dependents()) {
		if(ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dependent)) {
			if(modApp->input() == this && !modApp->pipelines(onlyScenePipelines).empty())
				pipelineCount++;
		}
		else if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(dependent)) {
            if(pipeline->dataProvider() == this) {
				if(!onlyScenePipelines || pipeline->isInScene())
		    		pipelineCount++;
			}
		}
	}
	return pipelineCount > 1;
}

/******************************************************************************
* Sets the current status of the pipeline object.
******************************************************************************/
void PipelineObject::setStatus(const PipelineStatus& status)
{
	if(status != _status) {
		_status = status;
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}
}

/******************************************************************************
* Given an animation time, computes the source frame to show.
******************************************************************************/
int PipelineObject::animationTimeToSourceFrame(TimePoint time) const
{
	OVITO_ASSERT(time != TimeNegativeInfinity());
	OVITO_ASSERT(time != TimePositiveInfinity());
	return dataset()->animationSettings()->timeToFrame(time);
}

/******************************************************************************
* Given a source frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint PipelineObject::sourceFrameToAnimationTime(int frame) const
{
	return dataset()->animationSettings()->frameToTime(frame);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
