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


/** @file MessageBuffer.h

This implements a message buffer.

@author Peter Krusche
*/

#ifndef __MessageBuffer_H__
#define __MessageBuffer_H__

#include "bsp.h"

#include <stdlib.h>
#include <string.h>

#include "Avector.h"

namespace utilities {

class MessageBuffer {
public:

	typedef utilities::AVector<double> storagevector;

	MessageBuffer() : storage_end(0) {}

	/** buffer a data block, expand storage vector if necessary */
	inline size_t buffer (const void * src, size_t nbytes) {
		size_t pos = storage_end;
		// this relies on the fact that double variables are 8 bytes long. as they should be.
		ASSERT (sizeof(double) == 8);
		storage_end += ( nbytes + 7 ) >> 3;
		size_t s = storage.exact_size();
		if(storage_end >= s ) {
			s = storage_end + BSP_DELIVTAB_MIN_SIZE + 1;
			storage.resize(s);
		}
		memcpy ((char*)(storage.data + pos), src, nbytes);
		return pos;
	}

	/** get a buffered data block start. */
	inline const void * get (size_t offset) {
		return storage.data + offset;
	}

	/** get the size of this buffer */
	inline size_t size () {
		return storage.exact_size() * sizeof (double);
	}

	/** clear the buffer */
	inline void clear() {
		storage.resize(BSP_DELIVTAB_MIN_SIZE);
		storage_end = 0;
	}

private:
	storagevector storage;
	size_t storage_end;
};

};

#endif // __MessageBuffer_H__
