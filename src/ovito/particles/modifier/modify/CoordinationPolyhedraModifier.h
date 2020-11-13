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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/mesh/surface/SurfaceMeshAccess.h>
#include <ovito/mesh/surface/SurfaceMeshVis.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>

namespace Ovito { namespace Particles {

/**
 * \brief A modifier that creates coordination polyhedra around atoms.
 */
class OVITO_PARTICLES_EXPORT CoordinationPolyhedraModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class CoordinationPolyhedraModifierClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	Q_OBJECT
	OVITO_CLASS_META(CoordinationPolyhedraModifier, CoordinationPolyhedraModifierClass)

	Q_CLASSINFO("DisplayName", "Coordination polyhedra");
	Q_CLASSINFO("Description", "Visualize atomic coordination polyhedra.");
#ifndef OVITO_BUILD_WEBGUI
	Q_CLASSINFO("ModifierCategory", "Visualization");
#else
	Q_CLASSINFO("ModifierCategory", "-");
#endif

public:

	/// Constructor.
	Q_INVOKABLE CoordinationPolyhedraModifier(DataSet* dataset);

	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void loadUserDefaults(Application::ExecutionContext executionContext) override;	

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const PipelineEvaluationRequest& request, ModifierApplication* modApp, const PipelineFlowState& input, Application::ExecutionContext executionContext) override;

private:

	/// Computation engine that builds the polyhedra.
	class ComputePolyhedraEngine : public Engine
	{
	public:

		/// Constructor.
		ComputePolyhedraEngine(Application::ExecutionContext executionContext, DataSet* dataset, ConstPropertyPtr positions,
				ConstPropertyPtr selection, ConstPropertyPtr particleTypes, ConstPropertyPtr particleIdentifiers,
				ConstPropertyPtr bondTopology, ConstPropertyPtr bondPeriodicImages, DataOORef<SurfaceMesh> mesh) :
			Engine(executionContext),
			_positions(std::move(positions)),
			_selection(std::move(selection)),
			_particleTypes(std::move(particleTypes)),
			_particleIdentifiers(std::move(particleIdentifiers)),
			_bondTopology(std::move(bondTopology)),
			_bondPeriodicImages(std::move(bondPeriodicImages)),
			_mesh(std::move(mesh)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the simulation cell geometry.
		const SimulationCellObject* cell() const { return _mesh->domain(); }

	private:

		ConstPropertyPtr _positions;
		ConstPropertyPtr _selection;
		ConstPropertyPtr _particleTypes;
		ConstPropertyPtr _particleIdentifiers;
		ConstPropertyPtr _bondTopology;
		ConstPropertyPtr _bondPeriodicImages;

		/// The generated mesh structure.
		DataOORef<SurfaceMesh> _mesh;
	};

	/// The vis element for rendering the polyhedra.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(SurfaceMeshVis, surfaceMeshVis, setSurfaceMeshVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE | PROPERTY_FIELD_OPEN_SUBEDITOR);
};

}	// End of namespace
}	// End of namespace
