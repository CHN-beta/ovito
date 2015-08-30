///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __OVITO_BONDS_STORAGE_H
#define __OVITO_BONDS_STORAGE_H

#include <plugins/particles/Particles.h>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace Ovito { namespace Particles {

/**
 * A single bond between two particles.
 */
struct Bond {

	/// If the bond crosses a periodic boundary, this tells us
	/// in which direction.
	Vector_3<int8_t> pbcShift;

	/// The index of the first particle.
	/// Note that we are not using size_t here to save memory.
	unsigned int index1;

	/// The index of the second particle.
	/// Note that we are not using size_t here to save memory.
	unsigned int index2;
};

/**
 * \brief List of bonds, which connect pairs of particles.
 */
class OVITO_PARTICLES_EXPORT BondsStorage : public std::vector<Bond>, public QSharedData
{
public:

	/// Writes the stored data to an output stream.
	void saveToStream(SaveStream& stream, bool onlyMetadata = false) const;

	/// Reads the stored data from an input stream.
	void loadFromStream(LoadStream& stream);
};

/**
 * \brief Helper class that allows to efficiently iterate over the half-bonds that are adjacent to a particle.
 */
class OVITO_PARTICLES_EXPORT ParticleBondMap
{
public:

	class bond_index_iterator : public boost::iterator_facade<bond_index_iterator, size_t const, boost::forward_traversal_tag> {
	public:
		bond_index_iterator(const ParticleBondMap* map, size_t startIndex) :
			_map(map), _currentIndex(startIndex) {}
	private:
		size_t _currentIndex;
		const ParticleBondMap* _map;

		friend class boost::iterator_core_access;

		void increment() {
			OVITO_ASSERT(_currentIndex < _map->_nextBond.size());
			_currentIndex = _map->_nextBond[_currentIndex];
		}

		bool equal(const bond_index_iterator& other) const {
			return this->_currentIndex == other._currentIndex;
		}

		size_t dereference() const { return _currentIndex; }
	};

public:

	/// Initializes the helper class.
	ParticleBondMap(const BondsStorage* bonds, size_t numberOfParticles);

	/// Returns an iterator range over the indices of the half-bonds adjacent to the given particle.
	boost::iterator_range<bond_index_iterator> bondsOfParticle(size_t particleIndex) const {
		OVITO_ASSERT(particleIndex < _startIndices.size());
		return boost::iterator_range<bond_index_iterator>(
				bond_index_iterator(this, _startIndices[particleIndex]),
				bond_index_iterator(this, _nextBond.size()));
	}

private:

	/// Contains the first half-bond index for each particle.
	std::vector<size_t> _startIndices;

	/// Stores the index of the next half-bond of particle in the linked list.
	std::vector<size_t> _nextBond;
};

}	// End of namespace
}	// End of namespace

#endif // __OVITO_BONDS_STORAGE_H
