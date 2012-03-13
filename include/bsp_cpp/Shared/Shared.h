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


/** @file Shared.h

Helper class to allow value sharing between contexts.

@author Peter Krusche
*/

#ifndef __BSP_SHARED_H__
#define __BSP_SHARED_H__ 

#include <vector>

namespace bsp {
	
	class Context;
	
	/** this class is used to declare variables shared between 
	    parent and child contexts 
		
		The two functions initialize() and reduce() can be
		overloaded and will be used to initialize or combine values.

	*/
	class Shared {
	public: 

		/** add variable as an input/output */
		inline void add_child ( Shared * s ) {
			vars.push_back(s);
		}

		/** Initializer function 
		 * 
		  @return true if any action was performed 
		 */
		inline virtual bool initialize()  { return false; }

		/** Reducer function 
		 * 
		  @return true if any action was performed 
		 */
		inline virtual bool reduce() { return false; }
		

		/** byte array serialization. This is necessary to 
		 *  be able to use bsp_fold and bsp_broadcast on a 
		 *  node level */
		virtual void serialize (void * target, size_t nbytes) = 0;
		virtual size_t serialized_size () = 0;

	protected:
		std::vector<Shared*> vars;
	};
};

#endif