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


/** @file bsp_localdelivery.h

@author Peter Krusche
*/

#ifndef __bsp_localdelivery_H__
#define __bsp_localdelivery_H__

#include <vector>
#include <tbb/scalable_allocator.h>

namespace bsp {

	/** a local memory delivery which will be
	 *  carried out once a superstep is over
	 */
	struct LocalMemoryDelivery {
		char * src;
		char * dst;
		size_t nbytes;
		bool free_src;
	};

	class LocalDeliveryQueue {
	public:
		tbb::scalable_allocator<char> alloc;

		enum {
			LQ_MIN = 64,
		};

		LocalDeliveryQueue() : qend (0) {
			deliv_queue.resize(LQ_MIN);
		}

		/** execute all queued deliveries */
		inline void execute () {
			size_t z = 0;
			while (z < qend) {
				LocalMemoryDelivery & d (deliv_queue[z]);
				memcpy (d.dst, d.src, d.nbytes);
				if(d.free_src) {
					alloc.deallocate(d.src, d.nbytes);
				}
				++z;
			}
			qend = 0;
			deliv_queue.resize(LQ_MIN);
		}

		/** enqueue a put operation */
		inline void put ( char * src, char * dst, size_t nbytes, bool buffer ) {
			LocalMemoryDelivery d;
			d.dst = dst;
			if ( !buffer ) {
				d.src = src;
				d.free_src = false;
			} else {
				d.src = alloc.allocate(nbytes);
				d.free_src = true;
				memcpy (d.src, src, nbytes);
			}
			d.nbytes = nbytes;
			if (deliv_queue.size() < qend+1) {
				deliv_queue.resize(deliv_queue.size()*2);
			}
			deliv_queue[qend++] = d;
		}

	private:
		size_t qend;
		std::vector <LocalMemoryDelivery> deliv_queue;
	};

};


#endif // __bsp_localdelivery_H__

