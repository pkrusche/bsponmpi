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


/** @file initializers.h

@author Peter Krusche
*/
#ifndef __initializers_H__
#define __initializers_H__

#include <tbb/blocked_range.h>

namespace bsp {

	/** Generic initializer which is usable in tbb::parallel_for 
	 * 
	 * Takes a variable type and an initializer of this form:
	 * 
	 * @code
	 * struct _init {
	 *		void operator () (const var & input, _var & v) {
	 *			// do something to v
	 *		}
	 * }
	 * @end
	 *
	 */
	template <class _var, class _init>
	struct Initializer {
		Initializer ( const _var & _input, 
			std::vector<_var> & _vars ) : input(_input), vars(_vars) {} 

		void operator()( const tbb::blocked_range<size_t>& r ) const { 
			for( size_t i = r.begin(); i != r.end(); ++i ) {
				i( input, vars[i] );
			}
		} 

		static const _init i;
		const _var & input;
		std::vector<_var> & const vars;
	};

#include "init_dummy.inl"
#include "init_assign.inl"

}

#endif // __initializers_H__
