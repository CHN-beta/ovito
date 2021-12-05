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


#include <ovito/particles/Particles.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/pipeline/PipelineObject.h>

namespace Ovito::Particles {

/**
 * \brief Loads particle trajectories from a separate file and injects them into the modification pipeline.
 */
class OVITO_PARTICLES_EXPORT LoadTrajectoryModifier : public Modifier
{
	/// Give this modifier class its own metaclass.
	class LoadTrajectoryModifierClass : public ModifierClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using ModifierClass::ModifierClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	OVITO_CLASS_META(LoadTrajectoryModifier, LoadTrajectoryModifierClass)

	Q_CLASSINFO("DisplayName", "Load trajectory");
	Q_CLASSINFO("Description", "Load atomic trajectories or dynamic bonds from a trajectory file.");
#ifndef OVITO_QML_GUI
	Q_CLASSINFO("ModifierCategory", "Modification");
#else
	Q_CLASSINFO("ModifierCategory", "-");
#endif

public:

	/// Constructor.
	Q_INVOKABLE LoadTrajectoryModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ObjectInitializationHints hints) override;	
	
	/// Determines the time interval over which a computed pipeline state will remain valid.
	virtual TimeInterval validityInterval(const ModifierEvaluationRequest& request) const override;

	/// Modifies the input data.
	virtual Future<PipelineFlowState> evaluate(const ModifierEvaluationRequest& request, const PipelineFlowState& input) override;

	/// Modifies the input data synchronously.
	virtual void evaluateSynchronous(const ModifierEvaluationRequest& request, PipelineFlowState& state) override;

	/// Returns the number of animation frames this modifier can provide.
	virtual int numberOfSourceFrames(int inputFrames) const override {
		return trajectorySource() ? trajectorySource()->numberOfSourceFrames() : inputFrames;
	}

	/// Given an animation time, computes the source frame to show.
	virtual int animationTimeToSourceFrame(TimePoint time, int inputFrame) const override {
		return trajectorySource() ? trajectorySource()->animationTimeToSourceFrame(time) : inputFrame;
	}

	/// Given a source frame index, returns the animation time at which it is shown.
	virtual TimePoint sourceFrameToAnimationTime(int frame, TimePoint inputTime) const override {
		return trajectorySource() ? trajectorySource()->sourceFrameToAnimationTime(frame) : inputTime;
	}

	/// Returns the human-readable labels associated with the animation frames (e.g. the simulation timestep numbers).
	virtual QMap<int, QString> animationFrameLabels(QMap<int, QString> inputLabels) const override {
		if(trajectorySource())
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
			inputLabels.insert(trajectorySource()->animationFrameLabels());
#else
			inputLabels.unite(trajectorySource()->animationFrameLabels());
#endif
		return std::move(inputLabels);
	}

protected:

	/// \brief Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor* field, RefTarget* oldTarget, RefTarget* newTarget, int listIndex) override;

private:

	/// Transfer the particle positions from the trajectory frame to the current pipeline input state.
	void applyTrajectoryState(PipelineFlowState& state, const PipelineFlowState& trajState, ObjectInitializationHints initializationHints);

	/// The source for trajectory data.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(OORef<PipelineObject>, trajectorySource, setTrajectorySource, PROPERTY_FIELD_NO_SUB_ANIM);
};

}	// End of namespace
