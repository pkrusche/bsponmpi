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
#include <cstring>
#include <cassert>
#include <vector> 

#include <boost/shared_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>

#include "Reducers.h"
#include "Shared.h"

namespace bsp {

/** Shared array class 
 * 
 *  Implements element-wise reduction + initialisation.
 *
 *  Has template specialisations for scalar and ByteSerializable elements.
 */

template <class _el, template <class> class _red = bsp::NoReduce  > 
class SharedArray : public bsp::Reduceable
{
public:
	typedef _red < _el > reducer;

	SharedArray() : 
		impl(*this), 
		size(0) {}

	SharedArray(size_t _size) : 
		impl(*this), 
		size(_size), 
		data(new _el[_size]) {}

	void init(_el const & val) {
		for (int i = 0; i < size; ++i)	{
			data[i] = val;
		}
	}

	void make_neutral() {
		static reducer r;
		for (int i = 0; i < size; ++i)	{
			 r.make_neutral(data[i]);
		}
	}

	void reduce_with(Reduceable const * _rhs) {
		static reducer reduce;
		const SharedArray & rhs (* static_cast<SharedArray const *> (_rhs));
		using namespace std;

		if(size < rhs.size) {
			impl.resize(rhs.size);
		}

		for (int i = 0; i < rhs.size; ++i)	{
			reduce(data[i], rhs.data[i]);
		}
	}
	
	void serialize(void * target, size_t nbytes) {
		ASSERT(nbytes >= serialized_size());
		impl.serialize(target, nbytes);
	}

	/** default byte deserialization */
	void deserialize(void * source, size_t nbytes) {
		ASSERT(nbytes <= serialized_size());
		impl.deserialize(source, nbytes);
	}

	/** default size */
	size_t serialized_size() {
		return impl.serialized_size();
	}

	/** number of elements stored in array */
	size_t capacity() {
		return size;
	}

	/** resize array */
	void resize(size_t new_size) {
		impl.resize(new_size);
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

	SharedArray & operator=(SharedArray const & rhs) {
		if(&rhs != this) {
			if(size != rhs.size) {
				impl.resize(rhs.size);
			}
			impl.copy_array( data.get(), rhs.data.get(), rhs.size );
		}
		return *this;
	}

private:

	template <class __el, template <class> class __red>
	class scalar_array_impl {
		public:
		scalar_array_impl(SharedArray<__el, __red> & _a) : a(_a) {}

		SharedArray<__el, __red> & a;

		void serialize(void * target, size_t nbytes) {
			size_t * ts = (size_t*)target;
			*ts++ = a.size;
			memcpy(ts, a.data.get(), sizeof(__el)*a.size);
		}
		
		void deserialize(void * source, size_t nbytes) {
			size_t * ss = (size_t *) source;
			if(*ss != a.size) {
				resize(*ss);
			}
			++ss;
			ASSERT(a.size*sizeof(__el) <= nbytes);
			memcpy(a.data.get(), ss, a.size*sizeof(__el));
		}

		size_t serialized_size() {
			return sizeof(__el) * a.size + sizeof(size_t);
		}

		void resize(size_t new_size) {
			if (new_size == a.size) {
				return;
			}
			using namespace std;
			static reducer reduce;
			int i;
			size_t min_size = min(new_size, a.size);
			
			boost::shared_array<__el> new_array(new __el[new_size]);

			memcpy(new_array.get(), a.data.get(), sizeof(__el) * min_size);

			for (i = a.size; i < new_size; ++i) {
				reduce.make_neutral(new_array[i]);
			}

			a.data = new_array;
			a.size = new_size;			
		}

		void copy_array(__el * target, __el const * source, size_t n) {
			memcpy(target, source, n*sizeof(__el));
		}

	};

	template <class __el, template <class> class __red>
	struct object_array_impl {
		// BOOST_STATIC_ASSERT( 
		// 	boost::is_base_of<bsp::ByteSerializable, __el >::value  );
		object_array_impl(SharedArray<__el, __red> & _a) : a(_a) {}

		SharedArray<__el, __red> & a;

		void serialize(void * target, size_t nbytes) {
			size_t * ts = (size_t*)target;
			*ts++ = a.size;
			char * cts = (char *) ts;
			for (int i = 0; i < a.size; ++i) {
				size_t sz = a[i].serialized_size();
				*((size_t*)cts) = sz;
				cts+= sizeof(size_t);
				a[i].serialize(cts, sz);
				cts+= sz;
			}			
		}

		void deserialize(void * source, size_t nbytes) {
			size_t * ss = (size_t*)source;
			if(*ss != a.size) {
				resize(*ss);
			}
			++ss;
			char * css = (char *) ss;
			for (int i = 0; i < a.size; ++i) {
				size_t sz = *((size_t*)css);
				css+= sizeof(size_t);
				a[i].deserialize(css, sz);
				css+= sz;
			}			
		}

		size_t serialized_size() {
			size_t overall_size = sizeof(size_t);
			for (int i = 0; i < a.size; ++i) {
				overall_size+= sizeof(size_t) + a[i].serialized_size();
			}			
			return overall_size;
		}

		void resize(size_t new_size) {
			if (new_size == a.size) {
				return;
			}
			using namespace std;
			static reducer reduce;
			int i;
			size_t min_size = min(new_size, a.size);
			
			boost::shared_array<__el> new_array(new __el[new_size]);

			for (i = 0; i < min_size; ++i) {
				new_array[i] = a.data[i];
			}

			for (i = a.size; i < new_size; ++i) {
				reduce.make_neutral(new_array[i]);
			}

			a.data = new_array;
			a.size = new_size;			
		}

		void copy_array(__el * target, __el const * source, size_t n) {
			for (int i = 0; i < n; ++i)	{
				target[i] = source[i];
			}
		}
	};


	typedef typename boost::mpl::if_<
		boost::is_scalar< _el >,
		scalar_array_impl<_el, _red>,
		object_array_impl<_el, _red> > :: type impl_t;

	impl_t impl;
	size_t size;
	boost::shared_array<_el> data;

};

};

#endif
