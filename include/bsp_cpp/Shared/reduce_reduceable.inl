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

#ifndef __REDUCE_REDUCEABLE_H__
#define __REDUCE_REDUCEABLE_H__

#include <algorithm>

#include "SharedSerialization.h"

namespace bsp {

	/** Reduceable interface */
	class Reduceable : public ByteSerializable {
		public:
            /** reduction requires that we can assign a neutral element
                w.r.t. the reduction operator
             */
            virtual void make_neutral() = 0;
            /** Reduction operator implementation
             */
			virtual void reduce_with(Reduceable const * ) = 0;

            virtual void serialize (void * target, size_t nbytes) = 0;

            virtual void deserialize (void * source, size_t nbytes) = 0;

            virtual size_t serialized_size () = 0;
	};

	template <class _var>
	struct Reduce {
        inline void make_neutral(_var & l) {
            l.make_neutral();
        }

		inline void operator() (_var & l, _var const & r) const {
			using namespace std;
			l.reduce_with(&r);
		}
	};

};

#endif