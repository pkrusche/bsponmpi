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


/** @file SharedArray.h

@author Peter Krusche

Shared Array class template

*/

#ifndef __BSPONMPI_SHARED_ARRAY_H__
#define __BSPONMPI_SHARED_ARRAY_H__

#include <cstdlib>
#include <cassert>
#include <vector> 

#include "boost/shared_array.hpp"

#include "Reducers.h"

namespace bsp {

template <class _el, class _red = bsp::NoReduce<_el> > 
class SharedArray : public bsp::Reduceable
{
public:
	SharedArray() : size(0) {}
	SharedArray(size_t _size) : size(_size), data(new _el[_size]) {}

	void init(_el const & val) {
		for (int i = 0; i < size; ++i)	{
			data[i] = val;
		}
	}

	void make_neutral() {
		static _red r;
		for (int i = 0; i < size; ++i)	{
			 r.make_neutral(data[i]);
		}
	}

	void reduce_with(Reduceable const * rhs) {
		static _red reduce;
		const SharedArray * sa = static_cast<SharedArray const *> (rhs);
		using namespace std;

		size_t s = min(sa->size, size);
		for (int i = 0; i < s; ++i)	{
			reduce(data[i], sa->data[i]);
		}
	}
	
	void serialize(void * target, size_t nbytes) {
		ASSERT(nbytes >= serialized_size());

	}

	/** default byte deserialization */
	void deserialize(void * source, size_t nbytes) {
		ASSERT(nbytes <= serialized_size());

	}

	/** default size */
	size_t serialized_size() {
		return sizeof(_el)*size;
	}

	operator _el * () {
		return data.get();
	}

	_el & operator[](int pos) {
		return data[pos];
	}

	const _el & operator[](int pos) const {
		return data[pos];
	}

private:
	size_t size;
	boost::shared_array<_el> data;
};

};

#endif
