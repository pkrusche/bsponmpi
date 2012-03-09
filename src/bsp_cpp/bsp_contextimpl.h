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


/** @file bsp_contextimpl.h

Implementation header for ContextImpl implementation

@author Peter Krusche
*/

#ifndef __bsp_contextimpl_mpi_H__
#define __bsp_contextimpl_mpi_H__

#include "bspx.h"

#include <map>
#include <queue>

#include <boost/shared_array.hpp>

#include <tbb/concurrent_hash_map.h>

#include "bsp_localdelivery.h"
#include "bsp_context_ts.h"


#include "bsp_cpp/TaskMapper.h"
#include "bsp_cpp/Context.h"

extern "C" {
#include "bsp_private.h"

#include "bsp_reqtable.h"
#include "bsp_delivtable.h"
#include "bsp_memreg.h"
};
namespace bsp {

	struct MemoryRegister {
		boost::shared_array<const void*>	pointers;
		size_t				nbytes;
		size_t				serial;
	};

	struct MemoryRegister_Reg {
		const void * data;
		size_t size;
		size_t serial;
		bool   push;
	};

	class ContextImpl {
	public:

		enum {
			MAX_REGISTER_REQS = 0xfffffff,	///< maximum number of memory register (de-)registrations per superstep
		};

		ContextImpl(TaskMapper * tm, int local_pid);
		~ContextImpl();

		/** This is where all contexts within a task mapper are synchronized */
		static void bsp_sync (TaskMapper *);

		void bsp_reset_buffers();

		/** push and pop operations use the local push/pop queue */
		void bsp_push_reg (const void *, size_t);
		void bsp_pop_reg (const void *);
		
		/** Put and get are local node aware, i.e. they only use the global
		 *  queue when they actually have to do remote deliveries.
		 *  
		 *  This should make them faster than standard put/get for local
		 *  delivery, but might cost some mutex waiting time for remote node
		 *  deliveries. Which shouldn't matter that much because 
		 */
		inline void bsp_put(int pid, const void* src, void* dst, long offset, size_t nbytes) {
			int n = mapper->global_to_node(pid);
			if (g_bsp.rank == n) {
				localDeliveries.put(
					(char *)src, 
					((char*)memory_register_map[dst].pointers[pid]) + offset, 
					nbytes);
			} else {
				char * pointer;
				DelivElement element;
				element.size = (unsigned int) nbytes;
				element.info.put.dst = ((char*)memory_register_map[dst].pointers[pid]) + offset;
				{ 
					TSLOCK();
					pointer = (char*)deliveryTable_push(&g_bsp.delivery_table, n, &element, it_put);
					memcpy(pointer, src, nbytes);
				}
			}
		}

		/**
		 * Local gets become preferable over puts in this implementation. 
		 */
		inline void bsp_get (int pid, const void * src, long int offset, void * dst, size_t nbytes) {
			int n = mapper->global_to_node(pid);
			if (g_bsp.rank == n) {
				localDeliveries.hpput (((char*)memory_register_map[src].pointers[pid]) + offset, 
					(char*) dst, nbytes);
			} else {
				ReqElement elem;
				elem.size = (unsigned int )nbytes;
				elem.src = ((char*)memory_register_map[src].pointers[pid]);
				elem.dst = (char* )dst;
				elem.offset = offset;
				{
					TSLOCK();
					/* place get command in buffer */
					requestTable_push(&g_bsp.request_table, n, &elem);
				}
			}
		}

		/** With hpput, we may win some time because we can do local deliveries
		 *  instantly */
		inline void bsp_hpput (int pid, const void * src, void * dst, long int offset, size_t nbytes) {
			int n = mapper->global_to_node(pid);

			if (n == g_bsp.rank) {
				// we have the data here on the same node, and can do the transfer now.
				char * destination = ((char*)memory_register_map[dst].pointers[pid]) + offset;
				memcpy(destination, src, nbytes);
			} else {
				// TODO we could technically also move this bit into 
				// a separate thread and use send/recv to allow sparse communication
				// ( presumably this would be faster than pooling in our Alltoallv
				//   call in bsp_sync() )
				// The same applies to hpget.
				// 
				// if it's somewhere else, we do the same as put
				char * RESTRICT pointer;
				DelivElement element;
				element.size = (unsigned int) nbytes;
				element.info.put.dst = ((char*)memory_register_map[dst].pointers[pid]) + offset;
				{
					TSLOCK();
					pointer = (char*)deliveryTable_push(&g_bsp.delivery_table, n, &element, it_put);
					memcpy(pointer, src, nbytes);
				}
			}
		}

		/** With hpget, we may also win some time because we can do local deliveries
		 *  instantly */
		inline void bsp_hpget (int pid, const void * src, long int offset, void * dst, size_t nbytes) {
			int n = mapper->global_to_node(pid);

			if (n == g_bsp.rank) {
				char * source = ((char*)memory_register_map[src].pointers[pid]) + offset;
				memcpy(dst, source, nbytes);
			} else {
				ReqElement elem;
				elem.size = (unsigned int )nbytes;
				elem.src = ((char*)memory_register_map[src].pointers[pid]);
				elem.dst = (char* )dst;
				elem.offset = offset;

				/* place get command in buffer */
				TSLOCK();
				requestTable_push(&g_bsp.request_table, n, &elem);
			}
		}

		/** we add an int to indicate which processor it's going to */
		inline void bsp_send (int pid, 
			const void *tag, const void *payload, size_t payload_nbytes) {

			DelivElement element;
			char * RESTRICT pointer;
			element.size = (unsigned int )payload_nbytes + g_bsp.message_queue.send_tag_size;
			element.info.send.payload_size = (unsigned int )payload_nbytes + sizeof(int);
			pointer = (char *)deliveryTable_push(&g_bsp.delivery_table, pid, &element, it_send);

			// we prepend the target pid
			*((int*)pointer) = pid;
			memcpy( pointer + sizeof(int), tag, g_bsp.message_queue.send_tag_size);
			memcpy( pointer + sizeof(int)+ g_bsp.message_queue.send_tag_size, payload, payload_nbytes);
		}
		
		void bsp_qsize (int * , size_t * );
		void bsp_get_tag (int * , void * );
		void bsp_move (void *, size_t);
		void bsp_set_tagsize (size_t *);
		int bsp_hpmove (void **, void **);

	private:

		/************************************************************************/
		/* Synchronization helpers                                              */
		/************************************************************************/

		/** process memory register registrations 
		 *
		 * @param reg_req_size : number of push and pop requests.
		 */

		static void process_memoryreg_ops(TaskMapper *, int reg_req_size);

		int local_pid; ///< local pid 

		/** remember the task mapper */
		TaskMapper * mapper;

		/** We reimplement BSPonMPI's registration mechanism here
		 *  using C++ maps. 
		 */

		bool any_hp;

		std::map<const void*, MemoryRegister> memory_register_map; /**< all valid memory registers, indexed by ptr 
															     in this context */

		std::queue< MemoryRegister_Reg > reg_requests;	/**< new registrations are buffered here */

		/** each context can do its node-local deliveries independently. */
		LocalDeliveryQueue	localDeliveries;

		/** we use our own bsp object */
		static BSPObject g_bsp;
		static int g_bsp_refcount;
	};

};

#endif // __bsp_contextimpl_mpi_H__
