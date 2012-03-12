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

extern BSPObject g_bsp;
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

		/** reset global and local delivery buffers */
		void bsp_reset_buffers() {
			TSLOCK();
			localDeliveries.reset_buffers();
			bspx_resetbuffers(&g_bsp);
		}

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
					memoryRegister_find ( &memory_register, global_pid, pid, (const char *)dst)
//					(char*)memory_register_map[dst].pointers[pid])
					 + offset, 
					nbytes);
			} else {
				char * pointer;
				DelivElement element;
				element.size = (unsigned int) nbytes;
				element.info.put.dst = memoryRegister_find ( &memory_register, global_pid, pid, (const char *)dst) + offset;
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
				localDeliveries.hpput (memoryRegister_find ( &memory_register, global_pid, pid, (const char*)src) + offset, 
					(char*) dst, nbytes);
			} else {
				ReqElement elem;
				elem.size = (unsigned int )nbytes;
				elem.src = memoryRegister_find ( &memory_register, global_pid, pid, (const char*)src);
					// ((char*)memory_register_map[src].pointers[pid]);
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
				char * destination = memoryRegister_find ( &memory_register, global_pid, pid, (const char*)dst) + offset;
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
				element.info.put.dst = memoryRegister_find ( &memory_register, global_pid, pid, (const char*)dst) + offset;
					//((char*)memory_register_map[dst].pointers[pid]) + offset;
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
				char * source = memoryRegister_find ( &memory_register, global_pid, pid, (const char*)src) + offset;
				memcpy(dst, source, nbytes);
			} else {
				ReqElement elem;
				elem.size = (unsigned int )nbytes;
				elem.src = memoryRegister_find ( &memory_register, global_pid, pid, (const char*)src);
					//((char*)memory_register_map[src].pointers[pid]);
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
			int n = mapper->global_to_node(pid);
			int lp = mapper->global_to_local(pid);

			if (n == g_bsp.rank) {
				TSLOCK();
				((ContextImpl*)mapper->get_context(lp)->get_impl())->localDeliveries.send(
					tag, g_bsp.message_queue.send_tag_size, 
					payload, payload_nbytes
				);
			} else {
				/** remote delivery */
				DelivElement element;
				char * RESTRICT pointer;
				element.size = (unsigned int )payload_nbytes + g_bsp.message_queue.send_tag_size + sizeof(int);
				element.info.send.payload_size = (unsigned int )payload_nbytes + sizeof(int);

				TSLOCK();
				pointer = (char *)deliveryTable_push(&g_bsp.delivery_table, n, &element, it_send);

				// we prepend the target local pid to the data
				*((int*) (pointer + g_bsp.message_queue.send_tag_size ) ) = lp;
				memcpy( pointer, tag, g_bsp.message_queue.send_tag_size);
				memcpy( pointer + sizeof(int) + g_bsp.message_queue.send_tag_size, payload, payload_nbytes);
			}
		}

		/** unbuffered send. do not touch tag or payload after this call.  */
		inline void bsp_hpsend (int pid, 
			const void *tag, const void *payload, size_t payload_nbytes) {
			int n = mapper->global_to_node(pid);
			int lp = mapper->global_to_local(pid);

			if (n == g_bsp.rank) {
				TSLOCK();
				((ContextImpl*)mapper->get_context(lp)->get_impl())->localDeliveries.hpsend (
					tag, payload, payload_nbytes
				);
			} else {
				/** remote delivery */
				DelivElement element;
				char * RESTRICT pointer;
				element.size = (unsigned int )payload_nbytes + g_bsp.message_queue.send_tag_size + sizeof(int);
				element.info.send.payload_size = (unsigned int )payload_nbytes + sizeof(int);

				TSLOCK();
				pointer = (char *)deliveryTable_push(&g_bsp.delivery_table, pid, &element, it_send);

				// we prepend the target local pid to the data
				*((int*) (pointer + g_bsp.message_queue.send_tag_size ) ) = lp;
				memcpy( pointer, tag, g_bsp.message_queue.send_tag_size);
				memcpy( pointer + sizeof(int) + g_bsp.message_queue.send_tag_size, payload, payload_nbytes);
			}
		}

		/** BSMP message queue size */
		inline void bsp_qsize (int * messages, size_t * accum_bytes) {
			*messages = localDeliveries.bsmp_qsize();
			*accum_bytes = localDeliveries.bsmp_move_bytes();
		}

		/** Get tag and payload size */
		inline void bsp_get_tag (int * status, void * tag) {
			if (localDeliveries.bsmp_qsize() > 0) {
				TSLOCK();
				*status = (int) localDeliveries.bsmp_top_size();
				memcpy (tag, localDeliveries.bsmp_top_tag(), 
					g_bsp.message_queue.recv_tag_size);
			} else {
				*status = -1;
			}
		}

		/** move data from the top of the message queue */
		inline void bsp_move (void * target, size_t nbytes) {
			if (localDeliveries.bsmp_qsize() > 0) {
				TSLOCK();
				ASSERT (nbytes <= localDeliveries.bsmp_top_size());
				memcpy (target, localDeliveries.bsmp_top_message(), nbytes);
				localDeliveries.bsmp_advance();
			}
		}

		/** the tag size is managed by bspx */
		inline void bsp_set_tagsize (size_t * t) {
			TSLOCK();
			bspx_set_tagsize(&g_bsp, t);
		}

		/** Here, we might again win something. When messages 
		 *  are sent and received using hp operations, not memcpy
		 *  is necessary at all.
		 */
		int bsp_hpmove (void **tag_ptr, void **payload_ptr) {
			if (localDeliveries.bsmp_qsize() > 0) {
				TSLOCK();
				*tag_ptr = (void*) localDeliveries.bsmp_top_tag();
				*payload_ptr = (void*) localDeliveries.bsmp_top_message();
				int size = (int) localDeliveries.bsmp_top_size();
				localDeliveries.bsmp_advance();
				return size;
			} else {
				return -1;
			}
		}

	private:

		/************************************************************************/
		/* Synchronization helpers                                              */
		/************************************************************************/

		/** process memory register registrations 
		 *
		 * @param reg_req_size : number of push and pop requests.
		 */

		static void process_memoryreg_ops(TaskMapper *, int reg_req_size);

		int global_pid; ///< global pid
		int local_pid; ///< local pid 

		/** remember the task mapper */
		TaskMapper * mapper;

		bool any_hp;

		/** We reimplement BSPonMPI's registration mechanism here
		 *  using C++ maps. 
		 *  
		 *  TODO, this could probably be done faster somehow.
		 */
		
		/** all valid memory registers, indexed by ptr */
		std::map<const void*, MemoryRegister> memory_register_map;
		/** the same for quick access */
		ExpandableTable memory_register; 

		/** new registrations are buffered here */
		std::queue< MemoryRegister_Reg > reg_requests;	

		/** each context can do its node-local deliveries independently. */
		LocalDeliveryQueue	localDeliveries;

	};

};

#endif // __bsp_contextimpl_mpi_H__
