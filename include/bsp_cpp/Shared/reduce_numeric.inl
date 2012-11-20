/*
BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
Copyright (C) 2006  Wijnand J. Suijlen, 2012 Peter Krusche

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

See the AUTHORS file distributed with this library for author contact
information.
*/


/** @file reduce_numeric.inl

@author Peter Krusche
*/

#ifndef __reduce_numeric_H__
#define __reduce_numeric_H__

#include <algorithm>

#include <cmath>
#include <climits>
#include <iostream>

namespace bsp {

template <class _var>
struct NoReduce
{
	/** neutral elements for minimum declared below */
	inline void make_neutral (_var & v) {
	}

	inline void operator() (_var & l, _var const & r) const {
	}
};

/** Minimum reducer */
template <class _var>
struct ReduceMin {

	/** neutral elements for minimum declared below */
	inline void make_neutral (_var & v) {
		std::cerr << 
			"Neutral element unknown, please supply one by specializing"
			<< typeid(*this).name() << 
			" ::  make_neutral<" 
			<< typeid( _var ).name() << ">"
			<< std::endl;
	}

	inline void operator() (_var & l, _var const & r) const {
		using namespace std;
		l = min(l, r);
	}
};

template <>
inline void ReduceMin<int>::make_neutral(int & v) {
	v = INT_MAX;
}

template <>
inline void ReduceMin<long int>::make_neutral(long int & v) {
	v = LONG_MAX;
}

template <>
inline void ReduceMin<float>::make_neutral(float & v) {
	v = (float)HUGE_VAL;
}

template <>
inline void ReduceMin<double>::make_neutral(double & v) {
	v = HUGE_VAL;
}

/** Maximum reducer */
template <class _var>
struct ReduceMax {
	/** neutral elements for minimum declared below */
	inline void make_neutral (_var & v) {
		std::cerr << 
			"Neutral element unknown, please supply one by specializing"
			<< typeid(*this).name() << 
			" ::  make_neutral<" 
			<< typeid( _var ).name() << ">"
			<< std::endl;
	}

	inline void operator() (_var & l, _var const & r) const {
		using namespace std;
		l = max(l, r);
	}
};

template <>
inline void ReduceMax<int>::make_neutral(int & v) {
	v = INT_MIN;
}

template <>
inline void ReduceMax<long int>::make_neutral(long int & v) {
	v = LONG_MIN;
}

template <>
inline void ReduceMax<float>::make_neutral(float & v) {
	v = -(float)HUGE_VAL;
}

template <>
inline void ReduceMax<double>::make_neutral(double & v) {
	v = -HUGE_VAL;
}

/** Keeps leftmost item reducer */
template <class _var>
struct ReduceFirst {
	inline void make_neutral (_var & v) {
	}
	inline void operator() (_var & , _var const & ) const {
	}
};

/** Keeps leftmost item reducer */
template <class _var>
struct ReduceLast {
	/** neutral elements for minimum declared below */
	inline void make_neutral (_var & v) {
	}
	inline void operator() (_var & l, _var const & r) const {
		using namespace std;
		l = r;
	}
};

/** Return the sum */
template <class _var>
struct ReduceSum {
	/** neutral elements for minimum declared below */
	inline void make_neutral (_var & v) {
		v = 0;
	}
	inline void operator() (_var & l, _var const & r) const {
		using namespace std;
		l += r;
	}
};

/** Return the product */
template <class _var>
struct ReduceProduct {
	/** neutral elements for minimum declared below */
	inline void make_neutral (_var & v) {
		v = 1;
	}
	inline void operator() (_var & l, _var const & r) const {
		using namespace std;
		l += r;
	}
};
};

#endif // __reduce_numeric_H__
