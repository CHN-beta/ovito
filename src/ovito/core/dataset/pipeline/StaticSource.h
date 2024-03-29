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
#include <ovito/core/dataset/data/DataCollection.h>
#include "PipelineObject.h"

namespace Ovito {

/**
 * \brief A source PipelineObject returning a static data collection.
 */
class OVITO_CORE_EXPORT StaticSource : public PipelineObject
{
	OVITO_CLASS(StaticSource)
	Q_CLASSINFO("DisplayName", "Pipeline source");

public:

	/// \brief Standard constructor.
	Q_INVOKABLE StaticSource(ObjectCreationParams params, DataCollection* data = nullptr);

	/// \brief Asks the object for the result of the data pipeline.
	virtual SharedFuture<PipelineFlowState> evaluate(const PipelineEvaluationRequest& request) override;

	/// \brief Returns the results of an immediate and preliminary evaluation of the data pipeline.
	virtual PipelineFlowState evaluateSynchronous(const PipelineEvaluationRequest& request) override;

	/// Returns the list of data objects that are managed by this data source.
	/// The returned data objects will be displayed as sub-objects of the data source in the pipeline editor.
	virtual const DataCollection* getSourceDataCollection() const override { return dataCollection(); }

protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The data collection owned by this source.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DataOORef<const DataCollection>, dataCollection, setDataCollection);
};

}	// End of namespace
