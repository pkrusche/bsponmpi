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


/** @file SharedVariable.h

@author Peter Krusche
*/

#ifndef __SharedVariable_H__
#define __SharedVariable_H__

#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>

#include "Shared.h"

#include "Initializers.h"
#include "Reducers.h"

namespace bsp {

	/** Node-local shared variable implementation */
	template <class _init, class _red>
	class SharedVariable : public SharedInit {
	public:
		/** Run initializer in tbb parallel for */
		bool initialize() {
			Initializer<Shared*, _init> i (this, vars);
			tbb::parallel_for( tbb::blocked_range<size_t> (0, vars.size() - 1), i );
			return true;
		}
		
		/** Run reducer in tbb parallel for */
		bool reduce() {
			Reducer<Shared*, _red> r (vars);
			tbb::parallel_reduce( tbb::blocked_range<size_t> (0, vars.size() - 1), i );
			return true;
		}		
	};
};


#endif // __SharedVariable_H__