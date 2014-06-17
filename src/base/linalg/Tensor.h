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

/**
 * \file Tensor.h
 * \brief Contains the definition of the Ovito::SymmetricTensor2T class template.
 */

#ifndef __OVITO_TENSOR_H
#define __OVITO_TENSOR_H

#include <base/Base.h>
#include "Matrix3.h"

namespace Ovito {

/// A first order tensor is just a one-dimensional vector.
typedef Vector3 Tensor1;

/// A second order tensor is just a two-dimensional matrix.
typedef Matrix3 Tensor2;

/*
 * \brief A symmetric 2nd order tensor.
 *
 * Stores only the lower left part of the 3x3 matrix.
 */
template<typename T>
class SymmetricTensor2T : public std::array<T, 6>
{
public:

	struct Zero {};
	struct Identity {};

	typedef T value_type;
	typedef std::size_t size_type;

public:

	/// \brief Constructs a tensor without initializing its components.
	/// \note All components are left uninitialized by this constructor and will therefore have an undefined value!
	SymmetricTensor2T() {}

	/// \brief Constructor that initializes all tensor components to the same value.
	explicit SymmetricTensor2T(T val) : std::array<T, 6>{{val,val,val,val,val,val}} {}

	/// \brief Constructor that initializes the six tensor components.
	Q_DECL_CONSTEXPR SymmetricTensor2T(T xx, T yy, T zz, T xy, T xz, T yz) : std::array<T, 6>{{xx,yy,zz,xy,xz,yz}} {}

	/// \brief Initializes the tensor to the null tensor. All components are set to zero.
	Q_DECL_CONSTEXPR SymmetricTensor2T(Zero) : std::array<T, 6>{{T(0), T(0), T(0), T(0), T(0), T(0)}} {}

	/// \brief Initializes the tensor to the identity tensor.
	Q_DECL_CONSTEXPR SymmetricTensor2T(Identity) : std::array<T, 6>{{T(1), T(1), T(1), T(0), T(0), T(0)}} {}

	/// \brief Casts the tensor to a tensor with another data type.
	template<typename U>
	Q_DECL_CONSTEXPR explicit operator SymmetricTensor2T<U>() const {
		return SymmetricTensor2T<U>(
				static_cast<U>(xx()), static_cast<U>(yy()), static_cast<U>(zz()),
				static_cast<U>(xy()), static_cast<U>(xz()), static_cast<U>(yz()));
	}

	/// \brief Returns the number of rows in this matrix.
	static Q_DECL_CONSTEXPR size_type row_count() { return 3; }

	/// \brief Returns the columns of rows in this matrix.
	static Q_DECL_CONSTEXPR size_type col_count() { return 3; }

	/// \brief Tensor element access.
	inline const T& operator()(size_type row, size_type col) const {
		OVITO_ASSERT(row < row_count() && col < col_count());
		if(row < col) std::swap(row, col);
		switch(row - col) {
			case 0: return (*this)[row];
			case 1: return (*this)[row+2];
			case 2: return (*this)[5];
			default: OVITO_ASSERT(false); return (*this)[0];
		}
	}

	/// \brief Tensor element access.
	inline T& operator()(size_type row, size_type col) {
		OVITO_ASSERT(row < row_count() && col < col_count());
		if(row < col) std::swap(row, col);
		switch(row - col) {
			case 0: return (*this)[row];
			case 1: return (*this)[row+2];
			case 2: return (*this)[5];
			default: OVITO_ASSERT(false); return (*this)[0];
		}
	}

	/// \brief Returns the value of the XX component of this tensor.
	Q_DECL_CONSTEXPR T xx() const { return (*this)[0]; }

	/// \brief Returns the value of the YY component of this tensor.
	Q_DECL_CONSTEXPR T yy() const { return (*this)[1]; }

	/// \brief Returns the value of the ZZ component of this tensor.
	Q_DECL_CONSTEXPR T zz() const { return (*this)[2]; }

	/// \brief Returns the value of the XY component of this tensor.
	Q_DECL_CONSTEXPR T xy() const { return (*this)[3]; }

	/// \brief Returns the value of the XZ component of this tensor.
	Q_DECL_CONSTEXPR T xz() const { return (*this)[4]; }

	/// \brief Returns the value of the YZ component of this tensor.
	Q_DECL_CONSTEXPR T yz() const { return (*this)[5]; }

	/// \brief Returns a reference to the XX component of this tensor.
	T& xx() { return (*this)[0]; }

	/// \brief Returns a reference to the YY component of this tensor.
	T& yy() { return (*this)[1]; }

	/// \brief Returns a reference to the ZZ component of this tensor.
	T& zz() { return (*this)[2]; }

	/// \brief Returns a reference to the XY component of this tensor.
	T& xy() { return (*this)[3]; }

	/// \brief Returns a reference to the XZ component of this tensor.
	T& xz() { return (*this)[4]; }

	/// \brief Returns a reference to the YZ component of this tensor.
	T& yz() { return (*this)[5]; }

};

// Addition / subtraction

template<typename T>
Q_DECL_CONSTEXPR inline SymmetricTensor2T<T> operator+(const SymmetricTensor2T<T>& A, const SymmetricTensor2T<T>& B)
{
	return { A[0]+B[0], A[1]+B[1], A[2]+B[2], A[3]+B[3], A[4]+B[4], A[5]+B[5] };
}

template<typename T>
Q_DECL_CONSTEXPR inline SymmetricTensor2T<T> operator-(const SymmetricTensor2T<T>& A, const SymmetricTensor2T<T>& B)
{
	return { A[0]-B[0], A[1]-B[1], A[2]-B[2], A[3]-B[3], A[4]-B[4], A[5]-B[5] };
}

template<typename T>
Q_DECL_CONSTEXPR inline SymmetricTensor2T<T> operator-(const SymmetricTensor2T<T>& A, typename SymmetricTensor2T<T>::Identity)
{
	return { A[0]-T(1), A[1]-T(1), A[2]-T(1), A[3], A[4], A[5] };
}

// Product with scalar

template<typename T>
Q_DECL_CONSTEXPR inline SymmetricTensor2T<T> operator*(const SymmetricTensor2T<T>& A, T s)
{
	return { A[0]*s, A[1]*s, A[2]*s, A[3]*s, A[4]*s, A[5]*s };
}

template<typename T>
Q_DECL_CONSTEXPR inline SymmetricTensor2T<T> operator*(T s, const SymmetricTensor2T<T>& A)
{
	return { A[0]*s, A[1]*s, A[2]*s, A[3]*s, A[4]*s, A[5]*s };
}

// Special tensor products:

/// Computes A^t * A.
template<typename T>
inline SymmetricTensor2T<T> Product_AtA(const Matrix_3<T>& A)
{
	SymmetricTensor2T<T> S;
	for(size_t i = 0; i < 3; i++) {
		for(size_t j = 0; j <= i; j++) {
			T b = 0;
			for(size_t k = 0; k < 3; k++)
				b += A(k,i) * A(k,j);
			S(i,j) = b;
		}
	}
	return S;
}

/// Computes A * A^t.
template<typename T>
inline SymmetricTensor2T<T> Product_AAt(const Matrix_3<T>& A)
{
	SymmetricTensor2T<T> S;
	for(size_t i = 0; i < 3; i++) {
		for(size_t j = 0; j <= i; j++) {
			T b = 0;
			for(size_t k = 0; k < 3; k++)
				b += A(i,k) * A(j,k);
			S(i,j) = b;
		}
	}
	return S;
}

/// Computes A * S * A^t.
template<typename T>
inline SymmetricTensor2T<T> TripleProduct_ASAt(const Matrix_3<T>& A, const SymmetricTensor2T<T>& S)
{
	Matrix_3<T> AS = A * S;
	SymmetricTensor2T<T> R;
	for(size_t i = 0; i < 3; i++) {
		for(size_t j = 0; j <= i; j++) {
			T b = 0;
			for(size_t k=0; k<3; k++)
				b += AS(i,k) * A(j,k);
			R(i,j) = b;
		}
	}
	return R;
}

/// Compute the double contraction of two tensors (A : B)
template<typename T>
inline T DoubleContraction(const SymmetricTensor2T<T>& A, const SymmetricTensor2T<T>& B)
{
	T d = 0;
	for(size_t i = 0; i < 3; i++)
		d += A[i] * B[i];
	for(size_t i = 3; i < 6; i++)
		d += T(2) * A[i] * B[i];
	return d;
}

/// Writes the symmetric tensor to an output stream.
template<typename T>
inline std::ostream& operator<<(std::ostream &os, const SymmetricTensor2T<T>& m) {
	for(typename SymmetricTensor2T<T>::size_type row = 0; row < m.row_count(); row++) {
		for(typename SymmetricTensor2T<T>::size_type col = 0; col < m.col_count(); col++) {
			os << m(row, col) << " ";
		}
		os << std::endl;
	}
	return os;
}

/// Writes a symmetric tensor to an output stream.
template<typename T>
inline SaveStream& operator<<(SaveStream& stream, const SymmetricTensor2T<T>& m)
{
	for(typename SymmetricTensor2T<T>::size_type i = 0; i < m.size(); i++)
		stream << m[i];
	return stream;
}

/// Reads a symmetric tensor from an input stream.
template<typename T>
inline LoadStream& operator>>(LoadStream& stream, SymmetricTensor2T<T>& m)
{
	for(typename SymmetricTensor2T<T>::size_type i = 0; i < m.size(); i++)
		stream >> m[i];
	return stream;
}

/**
 * \fn typedef SymmetricTensor2
 * \brief Template class instance of the SymmetricTensor2T class.
 */
typedef SymmetricTensor2T<FloatType> SymmetricTensor2;

};	// End of namespace

Q_DECLARE_METATYPE(Ovito::SymmetricTensor2);
Q_DECLARE_METATYPE(Ovito::SymmetricTensor2*);
Q_DECLARE_TYPEINFO(Ovito::SymmetricTensor2, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::SymmetricTensor2*, Q_PRIMITIVE_TYPE);

#endif // __OVITO_TENSOR_H
