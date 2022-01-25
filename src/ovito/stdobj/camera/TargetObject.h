////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
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


#include <ovito/stdobj/StdObj.h>
#include <ovito/core/dataset/data/DataObject.h>
#include <ovito/core/dataset/data/DataVis.h>
#include <ovito/core/rendering/LinePrimitive.h>

namespace Ovito::StdObj {

/**
 * A simple helper object that serves as direction target for camera and light objects.
 */
class OVITO_STDOBJ_EXPORT TargetObject : public DataObject
{
	OVITO_CLASS(TargetObject)
	Q_CLASSINFO("DisplayName", "Target");

public:

	/// Constructor.
	Q_INVOKABLE TargetObject(ObjectCreationParams params);
};

/**
 * \brief A visual element rendering target objects in the interactive viewports.
 */
class OVITO_STDOBJ_EXPORT TargetVis : public DataVis
{
	OVITO_CLASS(TargetVis)
	Q_CLASSINFO("DisplayName", "Target icon");

public:

	/// \brief Constructor.
	Q_INVOKABLE TargetVis(ObjectCreationParams params) : DataVis(params) {}

	/// \brief Lets the vis element render a data object.
	virtual PipelineStatus render(TimePoint time, const ConstDataObjectPath& path, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, const ConstDataObjectPath& path, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval) override;
};

}	// End of namespace
