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
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyObject.h>

namespace Ovito::Particles {

/**
 * \brief This utility class finds all neighbor particles within a cutoff radius of a central particle.
 *
 * OVITO provides two facilities for finding the neighbors of particles: The CutoffNeighborFinder class, which
 * finds all neighbors within a certain cutoff radius, and the NearestNeighborFinder class, which finds
 * the *k* nearest neighbor of a particle, where *k* is some positive integer. Note that the cutoff-based neighbor finder
 * can return an unknown number of neighbor particles, while the nearest neighbor finder will return exactly
 * the requested number of nearest neighbors (ordered by increasing distance from the central particle).
 * Whether CutoffNeighborFinder or NearestNeighborFinder is the right choice depends on the application.
 *
 * The CutoffNeighborFinder class must be initialized by a call to prepare(). This function generates a grid of bin
 * cells whose size is on the order of the specified cutoff radius. It sorts all input particles into these bin cells
 * for fast neighbor queries.
 *
 * After the CutoffNeighborFinder has been initialized, one can find the neighbors of some central
 * particle by constructing an instance of the CutoffNeighborFinder::Query class. This is a light-weight class which
 * iterates over all neighbors within the cutoff range of the selected particle.
 *
 * The CutoffNeighborFinder class takes into account periodic boundary conditions. With periodic boundary conditions,
 * a particle can be appear multiple times in the neighbor list of another particle. Note, however, that a different neighbor *vector* is
 * reported for each periodic image of a neighbor.
 */
class OVITO_PARTICLES_EXPORT CutoffNeighborFinder
{
private:

	// An internal per-particle data structure.
	struct NeighborListParticle {
		/// The position of the particle, wrapped at periodic boundaries.
		Point3 pos;
		/// The offset applied to the particle when wrapping it at periodic boundaries.
		Vector3I pbcShift;
		/// Pointer to next particle in linked list.
		const NeighborListParticle* nextInBin;
	};

public:

	/// Default constructor.
	/// You need to call prepare() first before the neighbor finder can be used.
	CutoffNeighborFinder() = default;

	/// \brief Prepares the neighbor finder by sorting particles into a grid of bin cells.
	/// \param cutoffRadius The cutoff radius for neighbor lists.
	/// \param positions The property containing the particle coordinates.
	/// \param simCell The input simulation cell geometry and boundary conditions.
	/// \param selectionProperty Determines which particles are included in the neighbor search (optional).
	/// \param operation An optional callback object that is used to the report progress.
	/// \return \c false when the operation has been canceled by the user;s
	///         \c true on success.
	/// \throw Exception on error.
	bool prepare(FloatType cutoffRadius, ConstPropertyAccess<Point3> positions, const SimulationCellObject* simCell, ConstPropertyAccess<int> selectionProperty, ProgressingTask* operation);

	/// Returns the cutoff radius set via prepare().
	FloatType cutoffRadius() const { return _cutoffRadius; }

	/// Returns the square of the cutoff radius set via prepare().
	FloatType cutoffRadiusSquared() const { return _cutoffRadiusSquared; }

	/// Returns the number of input particles.
	size_t particleCount() const { return particles.size(); }

	/// \brief An iterator class that returns all neighbors of a central particle.
	class OVITO_PARTICLES_EXPORT Query
	{
	public:

		/// Constructs a new neighbor query object that allows iterating over the neighbors of a particle.
		/// \param finder The parent object holding the list of input particles.
		/// \param particleIndex The index of the particle for which to enumerate the neighbors within the cutoff radius.
		Query(const CutoffNeighborFinder& finder, size_t particleIndex);

		/// Constructs a new neighbor query object that allows iterating over the neighbors withing the cutoff range of the given spatial location.
		/// \param finder The parent object holding the list of input particles.
		/// \param location The spatial around which to look for neighbors.
		Query(const CutoffNeighborFinder& finder, const Point3& location);

		/// Indicates whether the end of the list of neighbors has been reached.
		bool atEnd() const { return _atEnd; }

		/// Finds the next neighbor particle within the cutoff radius.
		/// Use atEnd() to test whether another neighbor has been found.
		void next();

		/// Returns the index of the current neighbor particle.
		size_t current() { return _neighborIndex; }

		/// Returns the vector connecting the central particle with the current neighbor.
		const Vector3& delta() const { return _delta; }

		/// Returns the distance squared between the central particle and the current neighbor.
		FloatType distanceSquared() const { return _distsq; }

		/// Returns the PBC shift vector between the central particle and the current neighbor.
		/// The vector is non-zero if the current neighbor vector crosses a periodic boundary.
		const Vector3I& pbcShift() const { return _pbcShift; }

		/// Returns the PBC shift vector between the central particle and the current neighbor as if the two particles
		/// were not wrapped at the periodic boundaries of the simulation cell.
		Vector3I unwrappedPbcShift() const {
			const auto& s1 = _builder.particles[_centerIndex].pbcShift;
			const auto& s2 = _builder.particles[_neighborIndex].pbcShift;
			return Vector3I(
					_pbcShift.x() - s1.x() + s2.x(),
					_pbcShift.y() - s1.y() + s2.y(),
					_pbcShift.z() - s1.z() + s2.z());
		}

	private:

		const CutoffNeighborFinder& _builder;
		bool _atEnd = false;
		Point3 _center, _shiftedCenter;
		size_t _centerIndex = std::numeric_limits<size_t>::max();
		std::vector<Vector3I>::const_iterator _stencilIter;
		Point3I _centerBin;
		Point3I _currentBin;
		const NeighborListParticle* _neighbor = nullptr;
		size_t _neighborIndex = std::numeric_limits<size_t>::max();
		Vector3I _pbcShift;
		Vector3 _delta;
		FloatType _distsq;
	};

private:

	/// The neighbor criterion.
	FloatType _cutoffRadius = 0;

	/// The neighbor criterion.
	FloatType _cutoffRadiusSquared = 0;

	// Simulation cell.
	DataOORef<const SimulationCellObject> simCell;

	/// Number of bins in each spatial direction.
	int binDim[3];

	/// Used to determine the bin from a particle position.
	AffineTransformation reciprocalBinCell;

	/// The internal list of particles.
	std::vector<NeighborListParticle> particles;

	/// An 3d array of cubic bins. Each bin is a linked list of particles.
	std::vector<const NeighborListParticle*> bins;

	/// The list of adjacent cells to visit while finding the neighbors of a
	/// central particle.
	std::vector<Vector3I> stencil;
};

}	// End of namespace
