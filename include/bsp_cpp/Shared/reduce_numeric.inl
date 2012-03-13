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

/** Minimum reducer */
template <class _var>
struct ReduceMin {
	void operator() (_var & l, _var const & r) const {
		using namespace std;
		l = min(l, r);
	}
};

/** Maximum reducer */
template <class _var>
struct ReduceMax {
	void operator() (_var & l, _var const & r) const {
		using namespace std;
		l = max(l, r);
	}
};

/** Keeps leftmost item reducer */
template <class _var>
struct ReduceFirst {
	void operator() (_var & , _var const & ) const {
	}
};

/** Keeps leftmost item reducer */
template <class _var>
struct ReduceLast {
	void operator() (_var & l, _var const & r) const {
		using namespace std;
		l = r;
	}
};

/** Return the sum */
template <class _var>
struct ReduceSum {
	void operator() (_var & l, _var const & r) const {
		using namespace std;
		l += r;
	}
};

/** Return the product */
template <class _var>
struct ReduceProduct {
	void operator() (_var & l, _var const & r) const {
		using namespace std;
		l += r;
	}
};

#endif // __reduce_numeric_H__
