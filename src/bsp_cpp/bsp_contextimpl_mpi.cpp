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

#include "bsp_contextimpl_mpi.h"

#include "bsp.h"
#include "bspx.h"

extern "C" {
#include "bsp_private.h"

#include "bsp_reqtable.h"
#include "bsp_delivtable.h"
#include "bsp_memreg.h"
};

#include "bsp_cpp/TaskMapper.h"
#include "bsp_cpp/Context.h"

#ifdef _HAVE_MPI
#include <mpi.h>

extern "C" {
#include "bspx_comm_mpi.h"
};
#endif

extern "C" {
#include "bspx_comm_seq.h"
};

static BSPObject g_bsp; ///< node-level bsp object
static int g_bsp_refcount = 0; ///< node level BSP object reference count

#define CM_NENTRIES		3

#define	CM_MESSAGE_COUNT	0
#define	CM_REQUEST_COUNT	1
#define CM_FLAGS			2

#define CM_FLAG_GETS			1
#define CM_FLAG_HP				2
#define CM_FLAG_MESSAGES		4

utilities::AVector <unsigned int> bsp::ContextImpl::h_send;
utilities::AVector <unsigned int> bsp::ContextImpl::h_recv;

/**
 * Constructor. Make local BSP object, update processor locations
 */
bsp::ContextImpl::ContextImpl(bsp::TaskMapper * tm, int lpid) 
	: mapper (tm), any_hp(false), local_pid(lpid) {
	if (g_bsp_refcount <= 0) {
		g_bsp_refcount = 1;
		bspx_init_bspobject(&g_bsp, ::bsp_nprocs(), ::bsp_pid());
	} else {
		g_bsp_refcount++;
	}

	// make processor location map

	if (h_send.exact_size() < CM_NENTRIES * ::bsp_nprocs()) {
		h_send.resize(CM_NENTRIES * ::bsp_nprocs());
	}
	if (h_recv.exact_size() < CM_NENTRIES * g_bsp.nprocs) {
		h_recv.resize(CM_NENTRIES * g_bsp.nprocs);
	}
}

/** 
 * Destructor. Destroy local BSP object 
 */
bsp::ContextImpl::~ContextImpl() {
	g_bsp_refcount--;
	if (g_bsp_refcount <= 0) {
		bspx_destroy_bspobject(&g_bsp);
	}
}

/**
 * Execute BSP sync.
 */
void bsp::ContextImpl::bsp_sync(bsp::TaskMapper * mapper) {	
	int reg_req_size = -1;
	bool any_hp = false;

	/************************************************************************/
	/* Step 1. exchange communication matrix.                               */
	/************************************************************************/
	for (int lp = 0; lp < mapper->procs_this_node(); ++lp) {
		ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(lp).get_impl());
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

	for (int p = 0; p < g_bsp.nprocs; ++p) {
		h_send[CM_NENTRIES * p + CM_MESSAGE_COUNT] = g_bsp.delivery_table.used_slot_count[p];
		h_send[CM_NENTRIES * p + CM_REQUEST_COUNT] = g_bsp.request_table.used_slot_count[p];

		h_send[CM_NENTRIES * p + CM_FLAGS] = 0;
		if ( h_send[CM_NENTRIES * p + CM_REQUEST_COUNT] > 0) {
			h_send[CM_NENTRIES * p + CM_FLAGS] |= CM_FLAG_GETS;
		}
		if ( h_send[CM_NENTRIES * p + CM_MESSAGE_COUNT] > 0) {
			h_send[CM_NENTRIES * p + CM_FLAGS] |= CM_FLAG_MESSAGES;
		}
		if ( any_hp ) {
			h_send[CM_NENTRIES * p + CM_FLAGS] |= CM_FLAG_HP;
		}

		h_send[CM_NENTRIES * p + CM_FLAGS] |= reg_req_size; 
	}
	
	_BSP_COMM0 (h_send.data, CM_NENTRIES*sizeof(unsigned int), 
				h_recv.data, CM_NENTRIES*sizeof(unsigned int) );

	/************************************************************************/
	/* Step 2. Process memory register registrations.                       */
	/************************************************************************/

	reg_req_size >>= 4;

	bool any_messages = false, any_gets = false;
	for (int p = 0; p < g_bsp.nprocs; ++p) {
		// local operations are exempt
		if ( p == g_bsp.rank ) {
			continue;
		}

		if (h_recv[CM_NENTRIES * p + CM_FLAGS] & CM_FLAG_MESSAGES) {
			any_messages = true;
		}

		if (h_recv[CM_NENTRIES * p + CM_FLAGS] & CM_FLAG_GETS) {
			any_gets = true;
		}

		if (h_recv[CM_NENTRIES * p + CM_FLAGS] & CM_FLAG_HP) {
			any_hp = true;
		}

#ifdef _DEBUG
		if ( reg_req_size != ( (h_recv[CM_NENTRIES * p + CM_FLAGS] >> 4) & MAX_REGISTER_REQS ) ) {
			throw std::runtime_error("bsp_sync(): mismatched number of registration requests.");
		}
#endif
	}

	/**
	 * This is expensive, so avoid registering/deregistering stuff too often.
	 */

	if (reg_req_size > 0) {
		process_memoryreg_ops(mapper, reg_req_size);
	}

	/************************************************************************/
	/* Step 3: process locally queued requests                              */
	/************************************************************************/

	deliveryTable_execute(&g_bsp.delivery_received_table, 
		&g_bsp.memory_register, &g_bsp.message_queue, g_bsp.rank);

	/************************************************************************/
	/* Step 4: Global sync                                                  */
	/************************************************************************/

	messageQueue_sync(&g_bsp.message_queue);
	requestTable_reset(&g_bsp.request_received_table);
	deliveryTable_reset(&g_bsp.delivery_received_table);


	/** if any gets were performed we need to 
  	 *  exchange them and convert them to put requests
	 */
	if (any_gets) {
		process_get_requests();
	}

	/** 
	 * put requests and messages are sent/received here. 
	 */
	if ( any_messages ) {
		using namespace std;

		unsigned int maxdelrows = 0;

		/** determine the required size for the table */
		for (int p = 0; p < g_bsp.nprocs; p++) {
			maxdelrows = max( h_recv[ CM_MESSAGE_COUNT + CM_NENTRIES * p ] 
							+ g_bsp.delivery_received_table.used_slot_count[p], 
						maxdelrows);
		}

		/** make delivery reception table big enough */
		if (g_bsp.delivery_received_table.rows < maxdelrows ) {
			deliveryTable_expand(&g_bsp.delivery_received_table, 
				maxdelrows - g_bsp.delivery_received_table.rows );
		}  

		/* copy necessary indices to received_tables */
		for (int p = 0; p < g_bsp.nprocs; p++)  {
			// we run a modified version of expandableTable_comm here which
			// preserves stuff in delivery_received_table which might already
			// be there through node-local delivery
			// 

			// this one points to the first free slot where we can receive from 
			// processor p (this will be zero except for p == bsp_pid() 
			g_bsp.delivery_received_table.offset[p] = 
				p * g_bsp.delivery_received_table.rows * g_bsp.delivery_received_table.slot_size
				+ g_bsp.delivery_received_table.slot_size * g_bsp.delivery_received_table.used_slot_count[p];
			
			// this one points to the first element sent to processor p
			g_bsp.delivery_table.offset[p] = p * g_bsp.delivery_table.rows * g_bsp.delivery_table.slot_size;

			// this one says how many bytes we send to processor p
			g_bsp.delivery_table.bytes[p] = g_bsp.delivery_table.used_slot_count[p] * g_bsp.delivery_table.slot_size;
			
			// this one says how many bytes we receive from processor p
			g_bsp.delivery_received_table.bytes[p] = 
				h_recv[CM_NENTRIES * p + CM_MESSAGE_COUNT ] * g_bsp.delivery_received_table.slot_size;
		}

		/* the next thing to do is to walk the send table and actually send the 
		* data */
		_BSP_COMM1(g_bsp.delivery_table.data, g_bsp.delivery_table.bytes, g_bsp.delivery_table.offset,
				   g_bsp.delivery_received_table.data, g_bsp.delivery_received_table.bytes, 
				   g_bsp.delivery_received_table.offset );


		/* clear the buffers */			
		deliveryTable_reset(&g_bsp.delivery_table);

		/* pack the memoryRegister */
		memoryRegister_pack(&g_bsp.memory_register);
	}

	/** execute put operations */
	deliveryTable_execute(&g_bsp.delivery_received_table, 
		&g_bsp.memory_register, &g_bsp.message_queue, g_bsp.rank);

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
	r.serial = memory_register_map.size() + reg_requests.size();
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

/** Sync memory register operations */
void bsp::ContextImpl::process_memoryreg_ops(bsp::TaskMapper * mapper, int reg_req_size) {
	int ppn = mapper->procs_per_node();
	utilities::AVector<MemoryRegister_Reg> vs, vr;
	vs.resize(g_bsp.nprocs * reg_req_size * ppn);
	vr.resize(g_bsp.nprocs * reg_req_size * ppn);

	for (int lp = 0; lp < mapper->procs_this_node(); ++lp) {
		for (int p = 0; p < g_bsp.nprocs; ++p) {
			ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(lp).get_impl());

			int o = p * reg_req_size * ppn + lp * reg_req_size;
			while (!cimpl->reg_requests.empty()) {
				vs[o++] = cimpl->reg_requests.front();
				cimpl->reg_requests.pop();
			}
		}
	}
	_BSP_COMM0 (
		vs.data, reg_req_size * ppn * sizeof (MemoryRegister_Reg), 
		vr.data, reg_req_size * ppn * sizeof (MemoryRegister_Reg) );

	for (int req = 0; req < reg_req_size; ++req) {
		size_t serial = -1;
		size_t size = -1;
		int push_or_pop = -1;

		for (int llp = 0; llp < mapper->procs_this_node(); ++llp) {
			ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(llp).get_impl());
			int my_local_pid = cimpl->local_pid;

			MemoryRegister reg;

			reg.pointers = new const void * [mapper->nprocs()];

			MemoryRegister_Reg & my_r (vr[ g_bsp.rank*reg_req_size*ppn + my_local_pid*reg_req_size + req ]);

			for (int gp = 0; gp < mapper->nprocs(); ++gp) {
				int p, lp;
				mapper->where_is(gp, p, lp);
				MemoryRegister_Reg & r (vr[ p*reg_req_size*ppn + lp*reg_req_size + req ]);

				// Check ordering 
				if (serial == -1) {
					serial = r.serial;
				} else {
					if (serial != r.serial) {
						throw std::runtime_error("bsp_sync(): memory register setup sequence mismatch. Pushreg/popreg operations must be collectively called in the correct order.");
					}
				}

				// Check size
				if (size == -1) {
					size = r.size;
				} else {
					if (size != r.size) {
						throw std::runtime_error("bsp_sync(): memory register setup size mismatch. Pushreg/popreg operations must register blocks of the same size.");
					}
				}

				// check push or pop
				if (push_or_pop == -1) {
					push_or_pop = r.push ? 1 : 0;
				} else {
					if (r.push && push_or_pop != 1) {
						throw std::runtime_error("bsp_sync(): pushreg/popreg mismatch.");
					}
					if (!r.push && push_or_pop != 0) {
						throw std::runtime_error("bsp_sync(): pushreg/popreg mismatch.");
					}
				}

				// save pointer
				reg.pointers[gp] = r.data;
			}

			reg.nbytes = size;
			reg.serial = serial;

			if (push_or_pop == 1) {
				if (cimpl->memory_register_map.find(my_r.data) != cimpl->memory_register_map.end()) {
					throw std::runtime_error("bsp_sync(): detected duplicate pushreg for the same address.");
				}
				cimpl->memory_register_map[my_r.data] = reg;
			} else {
				std::map<const void *, MemoryRegister>::iterator it = 
					cimpl->memory_register_map.find(my_r.data);

				if (it == cimpl->memory_register_map.end()) {
					throw std::runtime_error("bsp_sync(): mismatched popreg.");
				}
				delete [] it->second.pointers;
				cimpl->memory_register_map.erase(it);
			}
		}
	}
}

/** bsp_put which is aware of node locality */
void bsp::ContextImpl::bsp_put(int pid, const void * src, void * dst, long int offset, size_t nbytes) {
	int n, lp;
	mapper->where_is(pid, n, lp);
	
	char * RESTRICT pointer;
	if (n == bsp_pid()) {
		ContextImpl * cimpl = (ContextImpl *) (mapper->get_context(lp).get_impl());

		DelivElement element;
		element.size = (unsigned int) nbytes;
		element.info.put.dst = ((char*)memory_register_map[dst].pointers[pid]) + offset;

		pointer = (char*)deliveryTable_push(&g_bsp.delivery_received_table, n, &element, it_put);
		memcpy(pointer, src, nbytes);
	} else {
		char * RESTRICT pointer;
		DelivElement element;
		element.size = (unsigned int) nbytes;
		element.info.put.dst = ((char*)memory_register_map[dst].pointers[pid]) + offset;
		pointer = (char*)deliveryTable_push(&g_bsp.delivery_table, n, &element, it_put);
		memcpy(pointer, src, nbytes);
	}
}

void bsp::ContextImpl::bsp_get (int, const void *, long int, void *, size_t) {

}

void bsp::ContextImpl::bsp_hpput (int, const void *, void *, long int, size_t) {

}

void bsp::ContextImpl::bsp_hpget (int, const void *, long int, void *, size_t) {

}

/** Process all get requests, i.e. translate them into put requests */
void bsp::ContextImpl::process_get_requests() {
	unsigned int maxreqrows = array_max(h_recv.data 
		+ CM_REQUEST_COUNT, CM_NENTRIES*g_bsp.nprocs, CM_NENTRIES);

	if ( maxreqrows > 0 ) {
		requestTable_expand(&g_bsp.request_received_table, maxreqrows);
	}

	// tell expandableTable_comm how much data to expect
	for (int p = 0; p < g_bsp.nprocs; p++)  {
		g_bsp.request_received_table.used_slot_count[p] 
		= h_recv[CM_NENTRIES * p + CM_REQUEST_COUNT];
	}

	expandableTable_comm(&g_bsp.request_table, &g_bsp.request_received_table,
		_BSP_COMM1);

	requestTable_execute(&g_bsp.request_received_table, &g_bsp.delivery_table);

	// clear buffers
	requestTable_reset(&g_bsp.request_table);
	requestTable_reset(&g_bsp.request_received_table);
}

void bsp::ContextImpl::bsp_send (int, const void *, const void *, size_t) {

}

void bsp::ContextImpl::bsp_qsize (int * , size_t * ) {

}

void bsp::ContextImpl::bsp_get_tag (int * , void * ) {

}

void bsp::ContextImpl::bsp_move (void *, size_t) {

}


/** set tag size globally */
void bsp::ContextImpl::bsp_set_tagsize (size_t * size) {
	size_t ts = *size + sizeof(int);
	bspx_set_tagsize(&g_bsp, &ts);
	if (ts > + sizeof(int)) {
		ts -= + sizeof(int);
	}
	*size = ts;
}

int bsp::ContextImpl::bsp_hpmove (void **, void **) {
	return 0;
}

