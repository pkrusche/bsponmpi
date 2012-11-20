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

#include "bsp_level1.h"
#include "SharedSerialization.h"

namespace bsp {
	
	class Context;
	
	/** this class is used to declare variables shared between 
	    parent and child contexts 
		
		The two functions initialize() and reduce() can be
		overloaded and will be used to initialize or combine values.

	*/
	class Shared : public ByteSerializable {
	public: 

		/** add variable as an input/output */
		inline void add_child ( Shared * s ) {
			vars.push_back(s);
		}

		/** remove all dependent variables */
		inline void reset_children () {
			vars.clear();
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

		/** Neutral initializer
		 * 
		 * Reduce operations must have a neutral element.
		 * This function assigns the neutral element to the
		 * value inside this shared element.
		 */
		virtual void make_neutral() = 0;

		/* assign our value to another Shared object */
		virtual void assign(Shared * ) = 0;
		
	protected:
		std::vector<Shared*> vars;
	};

	/** Specialize broadcast for ByteSerializable objects */
	template <> 
	inline void bsp_broadcast<ByteSerializable>(int source, ByteSerializable & data) {
		size_t size = data.serialized_size();
		bsp_broadcast(source, size);
		char * sdata = new char [size];
		if (bsp_pid() == source) {
			data.serialize(sdata, size);
		}
		::bsp_broadcast(source, sdata, size);
		if (bsp_pid() != source) {
			data.deserialize(sdata, size);
		}
		delete [] sdata;
	}


	/** Specialize broadcast for pointers to shared values */
	template <> 
	inline void bsp_broadcast<Shared*>(int source, Shared* & data) {
		bsp_broadcast(source, (ByteSerializable&)(*data));
	}

};

#endif
