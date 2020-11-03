////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include "PTMAlgorithm.h"


namespace Ovito { namespace Particles {

/******************************************************************************
* Creates the algorithm object.
******************************************************************************/
PTMAlgorithm::PTMAlgorithm() : NearestNeighborFinder(MAX_INPUT_NEIGHBORS)
{
	ptm_initialize_global();
}

/******************************************************************************
* Constructs a new kernel from the given algorithm object, which must have
* previously been initialized by a call to PTMAlgorithm::prepare().
******************************************************************************/
PTMAlgorithm::Kernel::Kernel(const PTMAlgorithm& algo) : NeighborQuery(algo), _algo(algo)
{
	// Reserve thread-local storage of PTM routine.
	_handle = ptm_initialize_local();
}

/******************************************************************************
* Destructor.
******************************************************************************/
PTMAlgorithm::Kernel::~Kernel()
{
	// Release thread-local storage of PTM routine.
	ptm_uninitialize_local(_handle);
}

// Neighbor data passed to PTM routine.  Used in the get_neighbours callback function.
struct ptmnbrdata_t
{
	const NearestNeighborFinder* neighFinder;
	ConstPropertyAccess<int> particleTypes;
	const std::vector<uint64_t>* cachedNeighbors;
};

static int get_neighbours(void* vdata, size_t _unused_lammps_variable, size_t atom_index, int num_requested, ptm_atomicenv_t* env)
{
	ptmnbrdata_t* nbrdata = (ptmnbrdata_t*)vdata;
	const NearestNeighborFinder* neighFinder = nbrdata->neighFinder;
	const ConstPropertyAccess<int>& particleTypes = nbrdata->particleTypes;
	const std::vector<uint64_t>& cachedNeighbors = *nbrdata->cachedNeighbors;

	// Find nearest neighbors.
	OVITO_ASSERT(atom_index < cachedNeighbors.size());
	NearestNeighborFinder::Query<PTMAlgorithm::MAX_INPUT_NEIGHBORS> neighQuery(*neighFinder);
	neighQuery.findNeighbors(atom_index);
	int numNeighbors = std::min(num_requested - 1, neighQuery.results().size());
	OVITO_ASSERT(numNeighbors <= PTMAlgorithm::MAX_INPUT_NEIGHBORS);
	OVITO_ASSERT(cachedNeighbors[atom_index] != 0);

	ptm_decode_correspondences(PTM_MATCH_FCC,   //this gives us default behaviour
							   cachedNeighbors[atom_index], env->correspondences);

	// Bring neighbor coordinates into a form suitable for the PTM library.
	env->atom_indices[0] = atom_index;
	env->points[0][0] = 0;
	env->points[0][1] = 0;
	env->points[0][2] = 0;
	for(int i = 0; i < numNeighbors; i++) {
		int p = env->correspondences[i+1] - 1;
		OVITO_ASSERT(p >= 0);
		OVITO_ASSERT(p < neighQuery.results().size());
		env->atom_indices[i+1] = neighQuery.results()[p].index;
		env->points[i+1][0] = neighQuery.results()[p].delta.x();
		env->points[i+1][1] = neighQuery.results()[p].delta.y();
		env->points[i+1][2] = neighQuery.results()[p].delta.z();
	}

	// Build list of particle types for ordering identification.
	if(particleTypes) {
		env->numbers[0] = particleTypes[atom_index];
		for(int i = 0; i < numNeighbors; i++) {
			int p = env->correspondences[i+1] - 1;
			env->numbers[i+1] = particleTypes[neighQuery.results()[p].index];
		}
	}
	else {
		for(int i = 0; i < numNeighbors + 1; i++) {
			env->numbers[i] = 0;
		}
	}

	env->num = numNeighbors + 1;
	return numNeighbors + 1;
}


/******************************************************************************
* Identifies the local structure of the given particle and builds the list of
* nearest neighbors that form the structure.
******************************************************************************/
PTMAlgorithm::StructureType PTMAlgorithm::Kernel::identifyStructure(size_t particleIndex, const std::vector<uint64_t>& cachedNeighbors, Quaternion* qtarget)
{
	OVITO_ASSERT(cachedNeighbors.size() == _algo.particleCount());

	// Validate input.
	if(particleIndex >= _algo.particleCount())
		throw Exception("Particle index is out of range.");

	// Make sure public constants remain consistent with internal ones from the PTM library.
	OVITO_STATIC_ASSERT(MAX_INPUT_NEIGHBORS == PTM_MAX_INPUT_POINTS - 1);
	OVITO_STATIC_ASSERT(MAX_OUTPUT_NEIGHBORS == PTM_MAX_NBRS);

	ptmnbrdata_t nbrdata;
	nbrdata.neighFinder = &_algo;
	nbrdata.particleTypes = _algo._identifyOrdering ? _algo._particleTypes : nullptr;
	nbrdata.cachedNeighbors = &cachedNeighbors;

	int32_t flags = 0;
	if(_algo._typesToIdentify[SC]) flags |= PTM_CHECK_SC;
	if(_algo._typesToIdentify[FCC]) flags |= PTM_CHECK_FCC;
	if(_algo._typesToIdentify[HCP]) flags |= PTM_CHECK_HCP;
	if(_algo._typesToIdentify[ICO]) flags |= PTM_CHECK_ICO;
	if(_algo._typesToIdentify[BCC]) flags |= PTM_CHECK_BCC;
	if(_algo._typesToIdentify[CUBIC_DIAMOND]) flags |= PTM_CHECK_DCUB;
	if(_algo._typesToIdentify[HEX_DIAMOND]) flags |= PTM_CHECK_DHEX;
	if(_algo._typesToIdentify[GRAPHENE]) flags |= PTM_CHECK_GRAPHENE;

	// Call PTM library to identify the local structure.
	ptm_result_t result;
	int errorCode = ptm_index(_handle,
			particleIndex, get_neighbours, (void*)&nbrdata,
			flags,
			true,
			true,
			_algo._calculateDefGradient,
			&result,
			&_env);

	int32_t type = result.structure_type;
	_orderingType = result.structure_type;
	_scale = result.scale;
	_rmsd = result.rmsd;
	_interatomicDistance = result.interatomic_distance;
	_bestTemplateIndex = result.best_template_index;
	_bestTemplate = result.best_template;
	memcpy(_q, result.orientation, 4 * sizeof(double));
	if (_algo._calculateDefGradient) {
		memcpy(_F.elements(), result.F,  9 * sizeof(double));
	}

	OVITO_ASSERT(errorCode == PTM_NO_ERROR);

	// Convert PTM classification back to our own scheme.
	if(type == PTM_MATCH_NONE || (_algo._rmsdCutoff != 0 && _rmsd > _algo._rmsdCutoff)) {
		_structureType = OTHER;
		_orderingType = ORDERING_NONE;
		_rmsd = 0;
		_interatomicDistance = 0;
		_q[0] = _q[1] = _q[2] = _q[3] = 0;
		_scale = 0;
		_bestTemplateIndex = 0;
		_F.setZero();
	}
	else {
		_structureType = ptm_to_ovito_structure_type(type);
	}

#if 0
	if (_structureType != OTHER && qtarget != nullptr) {

		//arrange orientation in PTM format
		double qtarget_ptm[4] = { qtarget->w(), qtarget->x(), qtarget->y(), qtarget->z() };

		double disorientation = 0;	//TODO: this is probably not needed
		int template_index = ptm_remap_template(type, true, _bestTemplateIndex, qtarget_ptm, _q, &disorientation, _env.correspondences, &_bestTemplate);
		if (template_index < 0)
			return _structureType;

		_bestTemplateIndex = template_index;
	}
#endif

	return _structureType;

#if 0
////--------replace this with saved PTM data--------------------

//todo: when getting stored PTM data (when PTM is not called here), assert that output_conventional_orientations=true was used.

	//TODO: don't hardcode input flags
	int32_t flags = PTM_CHECK_FCC | PTM_CHECK_DCUB | PTM_CHECK_GRAPHENE;// | PTM_CHECK_HCP | PTM_CHECK_BCC;

	// Call PTM library to identify local structure.
	int32_t type = PTM_MATCH_NONE, alloy_type = PTM_ALLOY_NONE;
	double scale, interatomic_distance;
	double rmsd;
	double q[4];
	int8_t correspondences[PolyhedralTemplateMatchingModifier::MAX_NEIGHBORS+1];
	ptm_index(handle, numNeighbors + 1, points, nullptr, flags, true,
			&type, &alloy_type, &scale, &rmsd, q, nullptr, nullptr, nullptr, nullptr,
			&interatomic_distance, nullptr, nullptr, nullptr, correspondences);
	if (rmsd > 0.1) {	//TODO: don't hardcode threshold
		type = PTM_MATCH_NONE;
		rmsd = INFINITY;
	}

	if (type == PTM_MATCH_NONE)
		return;
////------------------------------------------------------------

	double qmapped[4];
	double qtarget[4] = {qw, qx, qy, qz};	//PTM quaterion ordering
	const double (*best_template)[3] = NULL;
	int _template_index = ptm_remap_template(type, true, input_template_index, qtarget, q, qmapped, &disorientation, correspondences, &best_template);
	if (_template_index < 0)
		return;

	//arrange orientation in OVITO format
	qres[0] = qmapped[1];	//qx
	qres[1] = qmapped[2];	//qy
	qres[2] = qmapped[3];	//qz
	qres[3] = qmapped[0];	//qw

	//structure_type = type;
	template_index = _template_index;
	for (int i=0;i<ptm_num_nbrs[type];i++) {

		int index = correspondences[i+1] - 1;
		auto r = neighQuery.results()[index];

		Vector3 tcoords(best_template[i+1][0], best_template[i+1][1], best_template[i+1][2]);
		result.emplace_back(r.delta, tcoords, r.distanceSq, r.index);
	}
#endif
}

int PTMAlgorithm::Kernel::cacheNeighbors(size_t particleIndex, uint64_t* res)
{
	// Validate input.
	OVITO_ASSERT(particleIndex < _algo.particleCount());

	// Make sure public constants remain consistent with internal ones from the PTM library.
	OVITO_STATIC_ASSERT(MAX_INPUT_NEIGHBORS == PTM_MAX_INPUT_POINTS - 1);
	OVITO_STATIC_ASSERT(MAX_OUTPUT_NEIGHBORS == PTM_MAX_NBRS);

	// Find nearest neighbors around the central particle.
	NeighborQuery::findNeighbors(particleIndex);
	int numNeighbors = NeighborQuery::results().size();

	double points[PTM_MAX_INPUT_POINTS - 1][3];
	for(int i = 0; i < numNeighbors; i++) {
		points[i][0] = NeighborQuery::results()[i].delta.x();
		points[i][1] = NeighborQuery::results()[i].delta.y();
		points[i][2] = NeighborQuery::results()[i].delta.z();
	}

	return ptm_preorder_neighbours(_handle, numNeighbors, points, res);
}

/******************************************************************************
* Returns the number of neighbors for the PTM structure found for the current particle.
******************************************************************************/
int PTMAlgorithm::Kernel::numTemplateNeighbors() const
{
	return ptm_num_nbrs[_structureType];
}

/******************************************************************************
* Returns the neighbor information corresponding to the i-th neighbor in the
* PTM template identified for the current particle.
******************************************************************************/
const NearestNeighborFinder::Neighbor& PTMAlgorithm::Kernel::getTemplateNeighbor(int index) const
{
	OVITO_ASSERT(_structureType != OTHER);
	OVITO_ASSERT(index >= 0 && index < numTemplateNeighbors());
	int mappedIndex = _env.correspondences[index + 1] - 1;
	return getNearestNeighbor(mappedIndex);
}

/******************************************************************************
* Returns the ideal vector corresponding to the i-th neighbor in the PTM template
* identified for the current particle.
******************************************************************************/
const Vector_3<double>& PTMAlgorithm::Kernel::getIdealNeighborVector(int index) const
{
	OVITO_ASSERT(_structureType != OTHER);
	OVITO_ASSERT(index >= 0 && index < numTemplateNeighbors());
	OVITO_ASSERT(_bestTemplate != nullptr);
	return *reinterpret_cast<const Vector_3<double>*>(_bestTemplate[index + 1]);
}

}	// End of namespace
}	// End of namespace
