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

namespace Ovito { namespace StdObj {

/**
 * \brief Stores the geometry and boundary conditions of a simulation box.
 *
 * The geometry of a simulation cell is a parallelepiped defined by three edge vectors.
 * A fourth vector specifies the coordinates of the origin of the simulation cell.
 */
class OVITO_STDOBJ_EXPORT SimulationCellObject : public DataObject
{
	Q_OBJECT
	OVITO_CLASS(SimulationCellObject)

public:

	/// \brief Constructor. Creates an empty simulation cell.
	Q_INVOKABLE SimulationCellObject(DataSet* dataset) : DataObject(dataset),
		_cellMatrix(AffineTransformation::Zero()),
		_reciprocalSimulationCell(AffineTransformation::Zero()),
		_pbcX(false), _pbcY(false), _pbcZ(false), _is2D(false) {}

	/// \brief Constructs a cell from three vectors specifying the cell's edges.
	/// \param a1 The first edge vector.
	/// \param a2 The second edge vector.
	/// \param a3 The third edge vector.
	/// \param origin The origin position.
	SimulationCellObject(DataSet* dataset, const Vector3& a1, const Vector3& a2, const Vector3& a3,
			const Point3& origin = Point3::Origin(), bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellMatrix(a1, a2, a3, origin - Point3::Origin()),
		_pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {}

	/// \brief Constructs a cell from a matrix that specifies its shape and position in space.
	/// \param cellMatrix The matrix
	SimulationCellObject(DataSet* dataset, const AffineTransformation& cellMatrix, bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellMatrix(cellMatrix), _pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {}

	/// \brief Constructs a cell with an axis-aligned box shape.
	/// \param box The axis-aligned box.
	/// \param pbcX Specifies whether periodic boundary conditions are enabled in the X direction.
	/// \param pbcY Specifies whether periodic boundary conditions are enabled in the Y direction.
	/// \param pbcZ Specifies whether periodic boundary conditions are enabled in the Z direction.
	SimulationCellObject(DataSet* dataset, const Box3& box, bool pbcX = false, bool pbcY = false, bool pbcZ = false, bool is2D = false) :
		DataObject(dataset),
		_cellMatrix(box.sizeX(), 0, 0, box.minc.x(), 0, box.sizeY(), 0, box.minc.y(), 0, 0, box.sizeZ(), box.minc.z()),
		_pbcX(pbcX), _pbcY(pbcY), _pbcZ(pbcZ), _is2D(is2D) {
		OVITO_ASSERT_MSG(box.sizeX() >= 0 && box.sizeY() >= 0 && box.sizeZ() >= 0, "SimulationCellObject constructor", "The simulation box must have a non-negative volume.");
	}
	
	/// Initializes the object's parameter fields with default values and loads 
	/// user-defined default values from the application's settings store (GUI only).
	virtual void initializeObject(ExecutionContext executionContext) override;

	/// Returns inverse of the simulation cell matrix.
	/// This matrix maps the simulation cell to the unit cube ([0,1]^3).
	const AffineTransformation& reciprocalCellMatrix() const {
		if(!_isReciprocalMatrixValid)
			computeInverseMatrix();
		return _reciprocalSimulationCell;
	}

	void invalidateReciprocalCellMatrix() { _isReciprocalMatrixValid = false; }

	/// Returns the current simulation cell matrix.
	const AffineTransformation& matrix() const { return cellMatrix(); }

	/// Returns the current reciprocal simulation cell matrix.
	const AffineTransformation& inverseMatrix() const { return reciprocalCellMatrix(); }

	/// Computes the (positive) volume of the three-dimensional cell.
	FloatType volume3D() const {
		return std::abs(cellMatrix().determinant());
	}

	/// Computes the (positive) volume of the two-dimensional cell.
	FloatType volume2D() const {
		return cellMatrix().column(0).cross(cellMatrix().column(1)).length();
	}

	/// \brief Enables or disables periodic boundary conditions in the three spatial directions.
	void setPbcFlags(const std::array<bool,3>& flags) {
		setPbcX(flags[0]);
		setPbcY(flags[1]);
		setPbcZ(flags[2]);
	}

	/// Sets the PBC flags.
	void setPbcFlags(bool pbcX, bool pbcY, bool pbcZ) { 
		setPbcX(pbcX); 
		setPbcY(pbcY); 
		setPbcZ(pbcZ); 
	}

	/// \brief Returns the periodic boundary flags in all three spatial directions.
	std::array<bool,3> pbcFlags() const {
		return {{ pbcX(), pbcY(), pbcZ() }};
	}

	/// \brief Returns the periodic boundary flags in all three spatial directions.
	std::array<bool,3> pbcFlagsCorrected() const {
		return {{ pbcX(), pbcY(), pbcZ() && !is2D() }};
	}

	/// Returns whether the simulation cell has periodic boundary conditions applied in the given direction.
	bool hasPbc(size_t dim) const { OVITO_ASSERT(dim < 3); return dim == 0 ? pbcX() : (dim == 1 ? pbcY() : pbcZ()); }

	/// Returns whether the simulation cell has periodic boundary conditions applied in the given direction.
	bool hasPbcCorrected(size_t dim) const { OVITO_ASSERT(dim < 3); return dim == 0 ? pbcX() : (dim == 1 ? pbcY() : (pbcZ() && !is2D())); }

	/// Returns whether the simulation cell has periodic boundary conditions applied in at least one direction.
	bool hasPbc() const { return pbcX() || pbcY() || pbcZ(); }

	/// Returns whether the simulation cell has periodic boundary conditions applied in at least one direction.
	bool hasPbcCorrected() const { return pbcX() || pbcY() || (pbcZ() && !is2D()); }

	/// \brief Returns the first edge vector of the cell.
	const Vector3& cellVector1() const { return cellMatrix().column(0); }

	/// \brief Returns the second edge vector of the cell.
	const Vector3& cellVector2() const { return cellMatrix().column(1); }

	/// \brief Returns the third edge vector of the cell.
	const Vector3& cellVector3() const { return cellMatrix().column(2); }

	/// \brief Returns the origin point of the cell.
	const Point3& cellOrigin() const { return Point3::Origin() + cellMatrix().column(3); }

	/// Returns true if the three edges of the cell are parallel to the three coordinates axes.
	bool isAxisAligned() const {
		if(cellMatrix()(1,0) != 0 || cellMatrix()(2,0) != 0) return false;
		if(cellMatrix()(0,1) != 0 || cellMatrix()(2,1) != 0) return false;
		if(cellMatrix()(0,2) != 0 || cellMatrix()(1,2) != 0) return false;
		return true;
	}

	/// Checks if two simulation cells are identical.
	bool equals(const SimulationCellObject& other) const {
		return cellMatrix() == other.cellMatrix() && pbcX() == other.pbcX() && pbcY() == other.pbcY() && pbcZ() == other.pbcZ() && is2D() == other.is2D();
	}

	/// Converts a point given in reduced cell coordinates to a point in absolute coordinates.
	Point3 reducedToAbsolute(const Point3& reducedPoint) const { return cellMatrix() * reducedPoint; }

	/// Converts a point given in absolute coordinates to a point in reduced cell coordinates.
	Point3 absoluteToReduced(const Point3& absPoint) const { return reciprocalCellMatrix() * absPoint; }

	/// Converts a vector given in reduced cell coordinates to a vector in absolute coordinates.
	Vector3 reducedToAbsolute(const Vector3& reducedVec) const { return cellMatrix() * reducedVec; }

	/// Converts a vector given in absolute coordinates to a point in vector cell coordinates.
	Vector3 absoluteToReduced(const Vector3& absVec) const { return reciprocalCellMatrix() * absVec; }

	/// Wraps a point at the periodic boundaries of the cell.
	Point3 wrapPoint(const Point3& p) const {
		Point3 pout = p;
		for(size_t dim = 0; dim < 3; dim++) {
			if(hasPbcCorrected(dim)) {
				if(FloatType s = std::floor(reciprocalCellMatrix().prodrow(p, dim)))
					pout -= s * cellMatrix().column(dim);
			}
		}
		return pout;
	}

	/// Wraps a vector at the periodic boundaries of the cell using minimum image convention.
	Vector3 wrapVector(const Vector3& v) const {
		Vector3 vout = v;
		for(size_t dim = 0; dim < 3; dim++) {
			if(hasPbcCorrected(dim)) {
				if(FloatType s = std::floor(reciprocalCellMatrix().prodrow(v, dim) + FloatType(0.5)))
					vout -= s * cellMatrix().column(dim);
			}
		}
		return vout;
	}

	/// Calculates the normal vector of the given simulation cell side.
	Vector3 cellNormalVector(size_t dim) const {
		size_t dim1 = (dim + 1) % 3;
		size_t dim2 = (dim + 2) % 3;
		Vector3 normal = cellMatrix().column(dim1).cross(cellMatrix().column(dim2));
		// Flip normal if necessary.
		if(normal.dot(cellMatrix().column(dim)) < 0)
			return normal / (-normal.length());
		else
			return normal.safelyNormalized();
	}

	/// Tests if a vector so long that it would be wrapped at a periodic boundary when using the minimum image convention.
	bool isWrappedVector(const Vector3& v) const {
		for(size_t dim = 0; dim < 3; dim++) {
			if(hasPbcCorrected(dim)) {
				if(std::abs(reciprocalCellMatrix().prodrow(v, dim)) >= FloatType(0.5))
					return true;
			}
		}
		return false;
	}

	/// \brief Helper function that computes the modulo operation for two integer numbers k and n.
	///
	/// This function can handle negative numbers k. This allows mapping any number k that is
	/// outside the interval [0,n) back into the interval. Use this to implement periodic boundary conditions.
	static inline int modulo(int k, int n) {
		return ((k %= n) < 0) ? k+n : k;
	}

	/// \brief Helper function that computes the modulo operation for two floating-point numbers k and n.
	///
	/// This function can handle negative numbers k. This allows mapping any number k that is
	/// outside the interval [0,n) back into the interval. Use this to implement periodic boundary conditions.
	static inline FloatType modulo(FloatType k, FloatType n) {
		k = std::fmod(k, n);
		return (k < 0) ? k+n : k;
	}	

	/// Returns whether this data object wants to be shown in the pipeline editor
	/// under the data source section.
	virtual bool showInPipelineEditor() const override { return true; }

	/// Creates an editable proxy object for this DataObject and synchronizes its parameters.
	virtual void updateEditableProxies(PipelineFlowState& state, ConstDataObjectPath& dataPath) const override;

	/// \brief Returns the title of this object.
	virtual QString objectTitle() const override { return tr("Simulation cell"); }

	////////////////////////////// Support functions for the Python bindings //////////////////////////////

	/// Indicates to the Python binding layer that this object has been temporarily put into a
	/// writable state. In this state, the binding layer will allow write access to the cell's internal data.
	bool isWritableFromPython() const { return _isWritableFromPython != 0; }

	/// Puts the simulation cell into a writable state.
	/// In the writable state, the Python binding layer will allow write access to the cell's internal data.
	void makeWritableFromPython();

	/// Puts the simulation cell array back into the default read-only state.
	/// In the read-only state, the Python binding layer will not permit write access to the cell's internal data.
	void makeReadOnlyFromPython() {
		OVITO_ASSERT(_isWritableFromPython > 0);
		_isWritableFromPython--;
	}

protected:

	/// Is called when the value of a non-animatable field of this object changes.
	virtual void propertyChanged(const PropertyFieldDescriptor* field) override;

private:

	/// Computes the inverse of the cell matrix.
	void computeInverseMatrix() const;

	/// Stores the three cell vectors and the position of the cell origin.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, cellMatrix, setCellMatrix);

	/// The inverse of the cell matrix, which is kept in sync with the cell matrix at all times.
	mutable AffineTransformation _reciprocalSimulationCell;
	/// Indicates whether the reciprocal matrix is in sync with the cell's matrix.
	mutable bool _isReciprocalMatrixValid = false;

	/// Specifies periodic boundary condition in the X direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcX, setPbcX);
	DECLARE_SHADOW_PROPERTY_FIELD(pbcX);
	/// Specifies periodic boundary condition in the Y direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcY, setPbcY);
	DECLARE_SHADOW_PROPERTY_FIELD(pbcY);
	/// Specifies periodic boundary condition in the Z direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, pbcZ, setPbcZ);
	DECLARE_SHADOW_PROPERTY_FIELD(pbcZ);

	/// Stores the dimensionality of the system.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, is2D, setIs2D);
	DECLARE_SHADOW_PROPERTY_FIELD(is2D);

	/// This is a special flag used by the Python bindings to indicate that
	/// this simulation cell has been temporarily put into a writable state.
	int _isWritableFromPython = 0;
};

}	// End of namespace
}	// End of namespace
