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


/** @file bsp_contextimpl_mpi.cpp

@author Peter Krusche
*/

#include "bsp_config.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>

#include "bsp_contextimpl.h"

#include "bsp.h"
#include "bspx.h"

#include "bsp_cpp/TaskMapper.h"
#include "bsp_cpp/Context.h"

//#define _DEBUGSUPERSTEPS

BSPObject bsp::ContextImpl::g_bsp; ///< node-level bsp object
int bsp::ContextImpl::g_bsp_refcount = 0; ///< node level BSP object reference count

#define	CM_REQUEST_COUNT	0
#define	CM_MESSAGE_COUNT	1
#define CM_FLAGS			2

#define CM_FLAG_GETS			1
#define CM_FLAG_MESSAGES		2

/**
 * Constructor. Make local BSP object, update processor locations
 */
bsp::ContextImpl::ContextImpl(bsp::TaskMapper * tm, int lpid) 
	: mapper (tm), any_hp(false), local_pid(lpid) {
	if (g_bsp_refcount <= 0) {
		g_bsp_refcount = 1;
		bspx_init_bspobject(&g_bsp, ::bsp_nprocs(), ::bsp_pid());

		/* clear the buffers */			
		deliveryTable_reset(&g_bsp.delivery_table);
		deliveryTable_reset(&g_bsp.delivery_received_table);

	} else {
		g_bsp_refcount++;
	}
	global_pid = tm->local_to_global_pid(lpid);
	memoryRegister_initialize( &memory_register, mapper->nprocs(), 1, global_pid );
}

/** 
 * Destructor. Destroy local BSP object 
 */
bsp::ContextImpl::~ContextImpl() {
	memoryRegister_destruct(&memory_register);
	g_bsp_refcount--;
	if (g_bsp_refcount <= 0) {
		bspx_destroy_bspobject(&g_bsp);
	}
}

/**
 * Execute BSP sync.
 */
void bsp::ContextImpl::bsp_sync( TaskMapper * mapper ) {	
	int reg_req_size = -1;
	bool any_hp = false;
	bool any_gets = false;

	/************************************************************************/
	/* Step 1. exchange communication matrix.                               */
	/************************************************************************/
	for (int lp = 0; lp < mapper->procs_this_node(); ++lp) {
		ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(lp).get_impl());

		/* here we also carry out all local deliveries */
		cimpl->localDeliveries.execute();

		if (reg_req_size < 0) {
			reg_req_size = (int)cimpl->reg_requests.size();
		} else {
			if (reg_req_size != cimpl->reg_requests.size()) {
				throw std::runtime_error("bsp_sync(): mismatched number of registration requests.");
			}
		}
		any_hp |= cimpl->any_hp;
	}
	
	if (reg_req_size < 0) {
		reg_req_size = 0;
	}

	reg_req_size = ((reg_req_size&MAX_REGISTER_REQS) << 4);

	bool any_messages = deliveryTable_empty(&g_bsp.delivery_table) == 0;
#ifdef _DEBUGSUPERSTEPS
	static int nstep = 0;
	nstep++;
#endif
 	for (int p = 0; p < g_bsp.nprocs; ++p) {
		g_bsp.send_index[3 * p + CM_MESSAGE_COUNT] = g_bsp.delivery_table.used_slot_count[p];
		g_bsp.send_index[3 * p + CM_REQUEST_COUNT] = g_bsp.request_table.used_slot_count[p];
		g_bsp.send_index[3 * p + CM_FLAGS] = 0;

		if (g_bsp.request_table.used_slot_count[p] > 0) {
			any_gets = true;
		}
	}

	for (int p = 0; p < g_bsp.nprocs; ++p) {
		if ( any_gets ) {
			g_bsp.send_index[3 * p + CM_FLAGS] |= CM_FLAG_GETS;
		}
		if ( any_messages ) {
			g_bsp.send_index[3 * p + CM_FLAGS] |= CM_FLAG_MESSAGES;
		}
		
		g_bsp.send_index[3 * p + CM_FLAGS] |= reg_req_size 
#ifdef _DEBUGSUPERSTEPS
			| (nstep & 0xf) << 20
#endif
			; 
	}

#ifdef _DEBUGSUPERSTEPS
	std::cout << "step " << nstep << " - " << "P" << g_bsp.rank << " CM Exchange" << std::endl;
	std::cout.flush();
#endif

	_BSP_COMM0 (g_bsp.send_index, 3*sizeof(unsigned int), 
				g_bsp.recv_index, 3*sizeof(unsigned int) );

#ifdef _DEBUGSUPERSTEPS
	{
		int step = nstep & 0xf;
		// check all processors are in the same superstep
		for (int p = 0; p < g_bsp.nprocs; ++p) {
			int step2 = g_bsp.recv_index[3 * p + CM_FLAGS] >> 20;
			g_bsp.recv_index[3 * p + CM_FLAGS] &= 0xfff;
			if (step2 != step) {
				bsp_abort("Not all processors are in the same superstep. I (%i) am in %i. Got %i from %i \n", g_bsp.rank, step, step2, p);
			}
		}

		std::ostringstream s;
		s << "step " << nstep << " - " << "P" << g_bsp.rank << " sent: ";
		for (int p = 0; p < g_bsp.nprocs; ++p) {
			s << p << ":(M" << g_bsp.send_index[3 * p + CM_MESSAGE_COUNT] << ",R"
				<<	    g_bsp.send_index[3 * p + CM_REQUEST_COUNT] << ",F"
				<<	    g_bsp.send_index[3 * p + CM_FLAGS] << ") ";
		}
		s << std::endl;
		s << "step " << nstep << " - " << "P" << g_bsp.rank << " got: ";
		bool _any_messages = false;
		bool _any_gets = false;
		for (int p = 0; p < g_bsp.nprocs; ++p) {
			s << p << ":(M" << g_bsp.recv_index[3 * p + CM_MESSAGE_COUNT] << ",R"
			  <<	    g_bsp.recv_index[3 * p + CM_REQUEST_COUNT] << ",F"
			  <<	    g_bsp.recv_index[3 * p + CM_FLAGS] << ") ";
			if (g_bsp.recv_index[3 * p + CM_FLAGS] & CM_FLAG_MESSAGES) {
				_any_messages = true;
			}

			if (g_bsp.recv_index[3 * p + CM_FLAGS] & CM_FLAG_GETS) {
				_any_gets = true;
			}
		}
		if (_any_messages) {
			s << ", will exchange messages";
		}
		if (_any_gets) {
			s << ", will exchange RT";
		}
		s << std::endl;
		std::cout << s.str();
		std::cout.flush();
	}
#endif

	/************************************************************************/
	/* Step 2. Process memory register registrations.                       */
	/************************************************************************/

	reg_req_size >>= 4;

	any_messages = false;
	bool local_messages = false;
	for (int p = 0; p < g_bsp.nprocs; ++p) {
		// local operations are exempt
		if (g_bsp.recv_index[3 * p + CM_FLAGS] & CM_FLAG_MESSAGES) {
			any_messages = true;
		}

		if (g_bsp.recv_index[3 * p + CM_FLAGS] & CM_FLAG_GETS) {
			any_gets = true;
		}
		using namespace std;
		reg_req_size = max ((unsigned)reg_req_size, g_bsp.recv_index[3 * p + CM_FLAGS] >> 4);
	}

	/**
	 * This is expensive, so avoid registering/deregistering stuff too often.
	 */

	if (reg_req_size > 0) {
		ContextImpl::process_memoryreg_ops(mapper, reg_req_size);
	}

	/************************************************************************/
	/* Step 3: Global sync                                                  */
	/************************************************************************/
	
	messageQueue_sync(&g_bsp.message_queue);
	requestTable_reset(&g_bsp.request_received_table);
	deliveryTable_reset(&g_bsp.delivery_received_table);

	/** 
	 * put requests and messages are sent/received here. 
	 */
	if ( any_messages || any_gets ) {
		using namespace std;
		unsigned int maxdelrows = 0;
		
		/* expand buffers if necessary */
		for (unsigned int p = 0; p < (unsigned)g_bsp.nprocs; p++) {
			maxdelrows = max( g_bsp.recv_index[CM_MESSAGE_COUNT + 3*p] + 
				g_bsp.request_table.info.req.data_sizes[p], maxdelrows );
		}
		
		if (g_bsp.delivery_received_table.rows < maxdelrows ) {
			maxdelrows = max (g_bsp.delivery_received_table.rows, (unsigned)maxdelrows);
			deliveryTable_expand (&g_bsp.delivery_received_table, maxdelrows );
		}  

		/* copy necessary indices to received_tables */
		for (unsigned int p = 0; p < (unsigned)g_bsp.nprocs; p++) {
			g_bsp.delivery_received_table.used_slot_count[p] =
				g_bsp.recv_index[ CM_MESSAGE_COUNT + 3 * p ] + 
				g_bsp.request_table.info.req.data_sizes[p] ;
		}	

		/** if any gets were performed we need to 
	  	 *  exchange them and convert them to put requests
		 */
		if (any_gets) {
			unsigned int maxreqrows = array_max(g_bsp.recv_index + CM_REQUEST_COUNT, 3*g_bsp.nprocs, 3);

			if ( maxreqrows > 0 ) {
				requestTable_expand(&g_bsp.request_received_table, maxreqrows);
			}

			// tell expandableTable_comm how much data to expect
			for (int p = 0; p < g_bsp.nprocs; p++)  {
				g_bsp.request_received_table.used_slot_count[p] 
					= g_bsp.recv_index[3 * p + CM_REQUEST_COUNT];
			}

#ifdef _DEBUGSUPERSTEPS
			std::cout << "step " << nstep << " - " << "P" << g_bsp.rank << " RT Exchange --->" << std::endl;
			std::cout.flush();
#endif
			expandableTable_comm(&g_bsp.request_table, &g_bsp.request_received_table,
				_BSP_COMM1);
#ifdef _DEBUGSUPERSTEPS
			std::cout << "step " << nstep << " - " << "P" << g_bsp.rank << " RT Exchange done." << std::endl;
			std::cout.flush();
#endif

			requestTable_execute(&g_bsp.request_received_table, &g_bsp.delivery_table);
		}

#ifdef _DEBUGSUPERSTEPS
		std::ostringstream s;
		s << "\ts"<< nstep << " proc " << g_bsp.rank << " receives [";
		for (int p = 0; p < g_bsp.nprocs; p++)  {
			s << g_bsp.delivery_received_table.used_slot_count[p] << ",";
		}
		s << "]" << std::endl;
		s << "\ts"<< nstep << " proc " << g_bsp.rank << " sends [";
		for (int p = 0; p < g_bsp.nprocs; p++)  {
			s << g_bsp.delivery_table.used_slot_count[p] << ",";
		}
		s << "]" << std::endl;
		std::cout << s.str() << std::endl;
		std::cout.flush();
#endif

#ifdef _DEBUGSUPERSTEPS
		std::cout << "step " << nstep << " - " << "P" << g_bsp.rank << " Data Exchange --->" << std::endl;
		std::cout.flush();
#endif
		expandableTable_comm(&g_bsp.delivery_table, &g_bsp.delivery_received_table,
			_BSP_COMM1);
#ifdef _DEBUGSUPERSTEPS
		std::cout << "step " << nstep << " - " << "P" << g_bsp.rank << " Data Exchange done." << std::endl;
		std::cout.flush();
#endif

		/** execute put operations */
		deliveryTable_execute(&g_bsp.delivery_received_table, 
			&g_bsp.memory_register, &g_bsp.message_queue, g_bsp.rank);

		/** split message queue */
		int bytes;
		void * tag, * message;
		bytes = bspx_hpmove(&g_bsp, &tag, &message);
		while (bytes >= 0) {
			// all messages sent through ContextImpl's (hp)send functions
			// will include an integer at the beginning of the message that
			// specifies the local processor to send to
			ASSERT (bytes >= sizeof(int));
			int lp = *((int*)message);
			ASSERT (lp>=0);
			ASSERT (lp < mapper->procs_this_node());
#ifdef _DEBUGSUPERSTEPS
			std::cout << "step " << nstep << " - " << "P" << g_bsp.rank << " Received message with tag " << 
					*((int*)tag) << 
					" for lp " << *((int*)message) << std::endl;
				std::cout.flush();
#endif
			ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(lp).get_impl());
			cimpl->localDeliveries.hpsend(tag, ((int*)message)+1, bytes-sizeof(int));

			bytes = bspx_hpmove(&g_bsp, &tag, &message);
		}		
	}

	for (int lp = 0; lp < mapper->procs_this_node(); ++lp) {
		ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(lp).get_impl());
		cimpl->localDeliveries.bsmp_messagequeue_sync();			
	}

	/* clear the buffers */			
	deliveryTable_reset(&g_bsp.delivery_table);
	requestTable_reset(&g_bsp.request_table);

	/* pack the memoryRegister 
	
	not necessary, we are managing memory registers on a 
	local process basis.
	
	memoryRegister_pack(&g_bsp.memory_register);
	*/
}

/** Push register implementation which distinguishes between local and 
 *  remote locations.
 */
void bsp::ContextImpl::bsp_push_reg(const void * ident, size_t nbytes) {
	ASSERT (memory_register_map.find (ident) == memory_register_map.end());

	// we limit the number of requests per superstep so we can fit them into a
	// single value
	ASSERT(reg_requests.size() < MAX_REGISTER_REQS);

	MemoryRegister_Reg r;
	r.data   = ident;
	r.size   = nbytes;
	r.serial = memory_register.used_slot_count[global_pid] + reg_requests.size();
	r.push   = true;
	reg_requests.push( r );
}

/** Pop register */
void bsp::ContextImpl::bsp_pop_reg(const void * ident) {
	// we limit the number of requests per superstep so we can fit them into a
	// single value
	ASSERT(reg_requests.size() < MAX_REGISTER_REQS);

	std::map<const void *, MemoryRegister>::iterator it = memory_register_map.find (ident);
	
	if (it == memory_register_map.end()) {
		throw std::runtime_error("bsp_pop_reg: unknown register in pop_reg.");
	}

	MemoryRegister_Reg r;
	r.data   = ident;
	r.size   = it->second.nbytes;
	r.serial = it->second.serial;
	r.push   = false;
	reg_requests.push( r );
}
