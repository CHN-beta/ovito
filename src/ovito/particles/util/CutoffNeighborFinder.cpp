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

#include <ovito/particles/Particles.h>
#include <ovito/core/utilities/concurrent/Task.h>
#include <ovito/core/dataset/DataSet.h>
#include "CutoffNeighborFinder.h"

namespace Ovito::Particles {

/******************************************************************************
* Initialization function.
******************************************************************************/
bool CutoffNeighborFinder::prepare(FloatType cutoffRadius, ConstPropertyAccess<Point3> positions, const SimulationCellObject* cell, ConstPropertyAccess<int> selectionProperty, ProgressingTask* operation)
{
	OVITO_ASSERT(positions);
	if(operation) operation->setProgressMaximum(0);

	_cutoffRadius = cutoffRadius;
	_cutoffRadiusSquared = cutoffRadius * cutoffRadius;
	if(_cutoffRadius <= 0)
		throw Exception("Invalid parameter: Neighbor cutoff radius must be positive.");

	// Check input simulation cell.
	// If it is periodic, make sure it is not degenerate.
	// If it is non-periodic and degenerate, replace the box with a non-degenerate one.
	bool is2D = false;
	if(cell && cell->isDegenerate()) {
		is2D = cell->is2D();
		if(cell->hasPbcCorrected())
			cell->throwException("Invalid input: Periodic simulation cell is degenerate.");
		else
			cell = nullptr;
	}
	simCell = cell;

	// If needed, create an ad-hoc simulation cell that is non-periodic and non-degenerate.
	if(!simCell) {
		Box3 boundingBox;
		boundingBox.addPoints(positions);
		if(boundingBox.isEmpty()) boundingBox.addPoint(Point3::Origin());
		if(boundingBox.sizeX() <= FLOATTYPE_EPSILON) boundingBox.maxc.x() = boundingBox.minc.x() + 1.0;
		if(boundingBox.sizeY() <= FLOATTYPE_EPSILON) boundingBox.maxc.y() = boundingBox.minc.y() + 1.0;
		if(boundingBox.sizeZ() <= FLOATTYPE_EPSILON) boundingBox.maxc.z() = boundingBox.minc.z() + 1.0;
		simCell = DataOORef<SimulationCellObject>::create(positions.buffer()->dataset(), 
				ObjectCreationParams::WithoutVisElement, AffineTransformation(
					Vector3(boundingBox.sizeX(), 0, 0),
					Vector3(0, boundingBox.sizeY(), 0),
					Vector3(0, 0, boundingBox.sizeZ()),
					boundingBox.minc - Point3::Origin()), false, false, false, is2D);
	}
	OVITO_ASSERT(!simCell->is2D() || !simCell->matrix().column(2).isZero());

	AffineTransformation binCell;
	binCell.translation() = simCell->matrix().translation();
	std::array<Vector3,3> planeNormals;

	// Determine the number of bins along each simulation cell vector.
	const double binCountLimit = 128*128*128;
	for(size_t i = 0; i < 3; i++) {
		planeNormals[i] = simCell->cellNormalVector(i);
		OVITO_ASSERT(planeNormals[i] != Vector3::Zero());
		FloatType x = std::abs(simCell->matrix().column(i).dot(planeNormals[i]) / _cutoffRadius);
		binDim[i] = std::max((int)floor(std::min(x, FloatType(binCountLimit))), 1);
	}
	if(simCell->is2D())
		binDim[2] = 1;

	// Impose limit on the total number of bins.
	double estimatedBinCount = (double)binDim[0] * (double)binDim[1] * (double)binDim[2];

	// Reduce bin count in each dimension by the same fraction to stay below total bin count limit.
	if(estimatedBinCount > binCountLimit) {
		if(!simCell->is2D()) {
			double factor = pow(binCountLimit / estimatedBinCount, 1.0/3.0);
			for(size_t i = 0; i < 3; i++)
				binDim[i] = std::max((int)(binDim[i] * factor), 1);
		}
		else {
			double factor = pow(binCountLimit / estimatedBinCount, 1.0/2.0);
			for(size_t i = 0; i < 2; i++)
				binDim[i] = std::max((int)(binDim[i] * factor), 1);
		}
	}

	qint64 binCount = (qint64)binDim[0] * (qint64)binDim[1] * (qint64)binDim[2];
	OVITO_ASSERT(binCount > 0 && binCount < (qint64)0xFFFFFFFF);

	// Compute bin cell.
	for(size_t i = 0; i < 3; i++) {
		binCell.column(i) = simCell->matrix().column(i) / binDim[i];
	}
	if(!binCell.inverse(reciprocalBinCell))
		throw Exception("Invalid input: Simulation cell is degenerate.");

	// Generate stencil.

	// This helper functions computes the shortest distance between a point and a bin cell located at the origin.
	auto shortestCellCellDistance = [binCell, planeNormals](const Vector3I& d) {
		Vector3 p = binCell * d.toDataType<FloatType>();
		// Compute distance from point to corner.
		FloatType distSq = p.squaredLength();
		for(size_t dim = 0; dim < 3; dim++) {
			// Compute shortest distance from point to edge.
			FloatType t = p.dot(binCell.column(dim)) / binCell.column(dim).squaredLength();
			if(t > 0.0 && t < 1.0)
				distSq = std::min(distSq, (p - t * binCell.column(dim)).squaredLength());
			// Compute shortest distance from point to cell face.
			const Vector3& n = planeNormals[dim];
			t = n.dot(p);
			if(t*t < distSq) {
				Vector3 p0 = p - t * n;
				const Vector3& u = binCell.column((dim+1)%3);
				const Vector3& v = binCell.column((dim+2)%3);
				FloatType a = u.dot(v)*p0.dot(v) - v.squaredLength()*p0.dot(u);
				FloatType b = u.dot(v)*p0.dot(u) - u.squaredLength()*p0.dot(v);
				FloatType denom = u.dot(v);
				denom *= denom;
				denom -= u.squaredLength()*v.squaredLength();
				a /= denom;
				b /= denom;
				if(a > 0 && b > 0 && a < 1 && b < 1)
					distSq = t*t;
			}
		}
		return distSq;
	};

	for(int stencilRadius = 0; stencilRadius < 100; stencilRadius++) {
		size_t oldCount = stencil.size();
		if(oldCount > 100*100)
			throw Exception("Neighbor cutoff radius is too large compared to the simulation cell size.");
		int stencilRadiusX = simCell->hasPbcCorrected(0) ? stencilRadius : std::min(stencilRadius, binDim[0] - 1);
		int stencilRadiusY = simCell->hasPbcCorrected(1) ? stencilRadius : std::min(stencilRadius, binDim[1] - 1);
		int stencilRadiusZ = simCell->hasPbcCorrected(2) ? stencilRadius : std::min(stencilRadius, binDim[2] - 1);
		for(int ix = -stencilRadiusX; ix <= stencilRadiusX; ix++) {
			for(int iy = -stencilRadiusY; iy <= stencilRadiusY; iy++) {
				for(int iz = -stencilRadiusZ; iz <= stencilRadiusZ; iz++) {
					if(std::abs(ix) < stencilRadius && std::abs(iy) < stencilRadius && std::abs(iz) < stencilRadius)
						continue;
					if(operation && operation->isCanceled())
						return false;
					FloatType shortestDistance = FLOATTYPE_MAX;
					for(int dx = -1; dx <= 1; dx++) {
						for(int dy = -1; dy <= 1; dy++) {
							for(int dz = -1; dz <= 1; dz++) {
								Vector3I d(dx + ix, dy + iy, dz + iz);
								shortestDistance = std::min(shortestDistance, shortestCellCellDistance(d));
							}
						}
					}
					if(shortestDistance < _cutoffRadius * _cutoffRadius) {
						stencil.push_back(Vector3I(ix,iy,iz));
					}
				}
			}
		}
		if(stencil.size() == oldCount)
			break;
	}

	// An 3d array of bins.
	// Each bin is a linked list of particles.
	bins.resize(binCount, nullptr);

	// Sort particles into bins.
	particles.resize(positions.size());
	const Point3* p = positions.cbegin();
	for(size_t pindex = 0; pindex < particles.size(); pindex++, ++p) {

		if(operation && operation->isCanceled())
			return false;

		NeighborListParticle& a = particles[pindex];
		a.pos = *p;
		a.pbcShift.setZero();

		if(selectionProperty && !selectionProperty[pindex])
			continue;

		// Determine the bin the particle is located in.
		Point3 rp = reciprocalBinCell * (*p);

		Point3I binLocation;
		for(size_t k = 0; k < 3; k++) {
			binLocation[k] = (int)floor(rp[k]);
			if(simCell->hasPbcCorrected(k)) {
				if(binLocation[k] < 0 || binLocation[k] >= binDim[k]) {
					int shift;
					if(binLocation[k] < 0)
						shift = -(binLocation[k]+1)/binDim[k]+1;
					else
						shift = -binLocation[k]/binDim[k];
					a.pbcShift[k] = shift;
					a.pos += (FloatType)shift * simCell->matrix().column(k);
					binLocation[k] = SimulationCellObject::modulo(binLocation[k], binDim[k]);
				}
			}
			else if(binLocation[k] < 0) {
				binLocation[k] = 0;
			}
			else if(binLocation[k] >= binDim[k]) {
				binLocation[k] = binDim[k] - 1;
			}
			OVITO_ASSERT(binLocation[k] >= 0 && binLocation[k] < binDim[k]);
		}

		// Put particle into its bin.
		size_t binIndex = binLocation[0] + binLocation[1]*binDim[0] + binLocation[2]*binDim[0]*binDim[1];
		a.nextInBin = bins[binIndex];
		bins[binIndex] = &a;
	}

	return true;
}

/******************************************************************************
* Iterator constructor
******************************************************************************/
CutoffNeighborFinder::Query::Query(const CutoffNeighborFinder& finder, size_t particleIndex)
	: _builder(finder), _centerIndex(particleIndex)
{
	OVITO_ASSERT(particleIndex < _builder.particleCount());

	_stencilIter = _builder.stencil.begin();
	_center = _builder.particles[particleIndex].pos;

	// Determine the bin the central particle is located in.
	for(size_t k = 0; k < 3; k++) {
		_centerBin[k] = (int)std::floor(_builder.reciprocalBinCell.prodrow(_center, k));
		if(_centerBin[k] < 0) _centerBin[k] = 0;
		else if(_centerBin[k] >= _builder.binDim[k]) _centerBin[k] = _builder.binDim[k] - 1;
	}

	next();
}

/******************************************************************************
* Iterator constructor
******************************************************************************/
CutoffNeighborFinder::Query::Query(const CutoffNeighborFinder& finder, const Point3& location)
	: _builder(finder), _center(finder.simCell->wrapPoint(location))
{
	_stencilIter = _builder.stencil.begin();

	// Determine the bin the central particle is located in.
	for(size_t k = 0; k < 3; k++) {
		_centerBin[k] = (int)std::floor(_builder.reciprocalBinCell.prodrow(_center, k));
		if(_centerBin[k] < 0) _centerBin[k] = 0;
		else if(_centerBin[k] >= _builder.binDim[k]) _centerBin[k] = _builder.binDim[k] - 1;
	}

	next();
}

/******************************************************************************
* Iterator function.
******************************************************************************/
void CutoffNeighborFinder::Query::next()
{
	OVITO_ASSERT(!_atEnd);

	for(;;) {
		while(_neighbor) {
			_delta = _neighbor->pos - _shiftedCenter;
			_neighborIndex = _neighbor - _builder.particles.data();
			_neighbor = _neighbor->nextInBin;
			_distsq = _delta.squaredLength();
			if(_distsq <= _builder._cutoffRadiusSquared && (_neighborIndex != _centerIndex || _pbcShift != Vector3I::Zero()))
				return;
		};

		for(;;) {
			if(_stencilIter == _builder.stencil.end()) {
				_atEnd = true;
				_neighborIndex = std::numeric_limits<size_t>::max();
				return;
			}

			_shiftedCenter = _center;
			_pbcShift.setZero();
			bool skipBin = false;
			for(size_t k = 0; k < 3; k++) {
				_currentBin[k] = _centerBin[k] + (*_stencilIter)[k];
				if(!_builder.simCell->hasPbcCorrected(k)) {
					if(_currentBin[k] < 0 || _currentBin[k] >= _builder.binDim[k]) {
						skipBin = true;
						break;
					}
				}
				else {
					if(_currentBin[k] >= _builder.binDim[k]) {
						int s = _currentBin[k] / _builder.binDim[k];
						_pbcShift[k] = s;
						_currentBin[k] -= s * _builder.binDim[k];
						_shiftedCenter -= _builder.simCell->matrix().column(k) * (FloatType)s;
					}
					else if(_currentBin[k] < 0) {
						int s = (_currentBin[k] - _builder.binDim[k] + 1) / _builder.binDim[k];
						_pbcShift[k] = s;
						_currentBin[k] -= s * _builder.binDim[k];
						_shiftedCenter -= _builder.simCell->matrix().column(k) * (FloatType)s;
					}
				}
				OVITO_ASSERT(_currentBin[k] >= 0 && _currentBin[k] < _builder.binDim[k]);
			}
			++_stencilIter;
			if(!skipBin) {
				_neighbor = _builder.bins[_currentBin[0] + _currentBin[1] * _builder.binDim[0] + _currentBin[2] * _builder.binDim[0] * _builder.binDim[1]];
				break;
			}
		}
	}
}

}	// End of namespace
