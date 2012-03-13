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


/** @file SharedSerialization.h

@author Peter Krusche
*/

#ifndef __SharedSerialization_H__
#define __SharedSerialization_H__

namespace bsp {

	/** byte array serialization. This is necessary to 
	 *  be able to use C++ variables through MPI */
	class ByteSerializable {
	public:
		virtual ~ByteSerializable() {}

		/**
		 * For simple types, this will just be a memcpy. 
		 * 
		 * Provide specializations for more complex types.
		 */
		virtual void serialize (void * target, size_t nbytes) = 0;

		virtual void deserialize (void * source, size_t nbytes) = 0;

		virtual size_t serialized_size () = 0;
	};

};

#endif // __SharedSerialization_H__