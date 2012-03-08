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

#include <bsp_tools/Avector.h>
#include <vector>


namespace bsp {
	typedef utilities::AVector<double> storagevector;
	

	/** a local memory delivery which will be
	 *  carried out once a superstep is over
	 */
	struct LocalMemoryDelivery {
		char * src;
		char * dst;
		size_t nbytes;
	};

	struct BufferedLocalMemoryDelivery {
		size_t offset;
		char * dst;
		size_t nbytes;
	};

	class LocalDeliveryQueue {
	public:
		enum {
			LQ_MIN = BSP_REQTAB_MIN_SIZE,
			ST_MIN = BSP_DELIVTAB_MIN_SIZE,
		};

		LocalDeliveryQueue() : qend (0), bqend(0) {
			deliv_queue.resize(LQ_MIN);
			buffered_deliv_queue.resize(LQ_MIN);
			storage.resize(ST_MIN);
			storage_end = 0;
		}

		/** execute all queued deliveries */
		inline void execute () {
			size_t z = 0;
			// unbuffered deliveries.
			while (z < qend) {
				LocalMemoryDelivery & d (deliv_queue[z++]);
				memcpy (d.dst, d.src, d.nbytes);
			}
			qend = 0;
			// buffered deliveries
			z = 0;
			while (z < bqend) {
				BufferedLocalMemoryDelivery & d (buffered_deliv_queue[z++]);
				memcpy (d.dst, storage.data + d.offset, d.nbytes);
			}
			bqend = 0;

			storage_end = 0;
			storage.resize(ST_MIN);
			deliv_queue.resize(LQ_MIN);
			buffered_deliv_queue.resize(LQ_MIN);
		}

		/** enqueue a put operation */
		inline void put ( char * src, char * dst, size_t nbytes ) {
			if (buffered_deliv_queue.size() < bqend+1) {
				buffered_deliv_queue.resize(buffered_deliv_queue.size() * 2);
			}
			BufferedLocalMemoryDelivery & d(buffered_deliv_queue[bqend++]);
			d.dst = dst;
			d.offset = storage_end;
			storage_end += ( nbytes + 7 ) >> 3;
			if(storage_end >= storage.exact_size() ) {
				storage.resize( (storage_end + 1) * 2 );
			}
			memcpy ((char*)(storage.data + d.offset), src, nbytes);
			d.nbytes = nbytes;
		}

		/** enqueue an unbuffered put operation */
		inline void hpput ( char * src, char * dst, size_t nbytes ) {
			if (deliv_queue.size() < qend+1) {
				deliv_queue.resize(deliv_queue.size()*2);
			}
			LocalMemoryDelivery & d(deliv_queue[qend++]);
			d.dst = dst;
			d.src = src;
			d.nbytes = nbytes;
		}

	private:
		size_t qend, bqend, storage_end;
		std::vector <LocalMemoryDelivery> deliv_queue;
		std::vector <BufferedLocalMemoryDelivery> buffered_deliv_queue;
		storagevector storage;
	};

};


#endif // __bsp_localdelivery_H__

