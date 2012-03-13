/*
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
    Copyright (C) 2006  Wijnand J. Suijlen, 2012, Peter Krusche
                                                                                
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
#ifndef __BSP_Reducers_h__
#define  __BSP_Reducers_h__

#include <tbb/blocked_range.h>

namespace bsp {


	/** Generic reducer that takes a variable type and an operator of the form
	 *
	 * @code
	 * struct _op {
	 *		void operator (_var & l, _var const & r);
	 * }
	 * @end
	 *
	 * (l is merged with r (a'la foldl))
	 *  
	 * and makes it usable with tbb::parallel_reduce
	 *  
	 */
	template <class _var, class _op>
	struct Reducer {
		Reducer ( _var & dst, std::vector<_var> & _vars ) : vars(_vars), mine(dst) {} 

		Reducer ( Reducer & r, tbb::split ) : vars (r.vars), mine (r.mine) {}

		void operator()( const tbb::blocked_range<size_t>& r ) { 

			for( size_t i = r.begin(); i != r.end(); ++i ) {
				r ( mine, vars[i] );
			}
		}

		static const _op r;
		std::vector<_var > & const vars;
		_var & mine;
	};

};

#endif
