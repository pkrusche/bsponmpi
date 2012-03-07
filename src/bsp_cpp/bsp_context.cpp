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


/** @file bsp_context.cpp

@author Peter Krusche
*/

#include "bsp_config.h"

#include "bsp_contextimpl.h"

#include "bsp_cpp/bsp_cpp.h"

#include <tbb/spin_mutex.h>

extern "C" {
#include "bsp_private.h"

#include "bsp_reqtable.h"
#include "bsp_delivtable.h"
#include "bsp_memreg.h"
};

/** Mutex to make access to message buffers thread safe */

namespace bsp {
	static tbb::spin_mutex g_context_mutex;

};

/** Thread safety helper */
#define TSLOCK() tbb::spin_mutex::scoped_lock l (bsp::g_context_mutex)


/** @name Initialisation and destruction */
/*@{*/

void bsp::Context::initialize_context (int bsp_pid, Context * parent)  {
	ASSERT (parent != NULL);
	parentcontext = parent;
	local_pid = mapper->global_to_local_pid(bsp_pid);
	pid = bsp_pid;
	runme = NULL;	// this is initialized by the code in Runner.h
	impl = new bsp::ContextImpl(mapper, local_pid);
	init ();
}

void bsp::Context::destroy_context () {
	delete (bsp::ContextImpl*) impl;
}
/*@}*/

#define BSP ((ContextImpl*)impl)

void bsp::Context::bsp_sync ( bool local ) {
	if (impl != NULL) {
		// mainly, this serves as a deterrent so programmers don't 
		// get confused between node and task-level syncs.
		throw std::runtime_error("When syncing in a Context, BSP_SYNC needs to be used rather than bsp_sync()");
	} else {
		if (local) {
			ContextImpl::bsp_sync( mapper );
		} else {
			::bsp_sync();
		}
	}
}

/** @name DRMA */
/*@{*/
void bsp::Context::bsp_push_reg (const void * data, size_t len) {
	TSLOCK();
	BSP->bsp_push_reg(data, len);
}

void bsp::Context::bsp_pop_reg (const void * data) {
	TSLOCK();
	BSP->bsp_pop_reg(data);
}

extern BSPObject g_bsp;

/** Adds an element to the table and expands the table when necessary. The
* payload may be copied to the address referenced by the returned pointer. 
@param table Reference to a VarElSizeTable
@param proc Destination processor
@param element Element to be added
@param type Type of element
@return pointer to memory location in which data can be copied
*/
inline char * deliveryTable_push2 (ExpandableTable *RESTRICT  table, const int proc,
	const DelivElement *RESTRICT element, const ItemType type)
{
	const unsigned int slot_size = sizeof(ALIGNED_TYPE);
	const unsigned int tag_size = no_slots(sizeof(DelivElement), slot_size);
	const unsigned int object_size = tag_size + no_slots(element->size, slot_size);
	ALIGNED_TYPE * RESTRICT pointer;
	TSLOCK();

	int free_space = table->rows - table->used_slot_count[proc];

	if ((signed)object_size > free_space) 
	{
		int space_needed = MAX(table->rows, object_size - free_space);
		deliveryTable_expand(table, space_needed);
	}  

	/* manage deliveryTable info */
	if (table->info.deliv.count[proc][type] == 0)
		table->info.deliv.start[proc][type] = table->used_slot_count[proc];
	else
	{
		pointer = (ALIGNED_TYPE *) table->data + table->info.deliv.end[proc][type] + proc * table->rows;
		((DelivElement *) pointer)->next = table->used_slot_count[proc] - table->info.deliv.end[proc][type];
	}

	table->info.deliv.end[proc][type] = table->used_slot_count[proc];
	table->info.deliv.count[proc][type]++;

	pointer = (ALIGNED_TYPE *) table->data + table->used_slot_count[proc] + proc * table->rows;
	* (DelivElement *) pointer = *element;
	pointer+=tag_size;
	
	/* increment counters */
	table->used_slot_count[proc] += object_size;

	return (char* )pointer ;  
}


void bsp::Context::bsp_put (int pid, const void *src, void *dst, long int offset, size_t nbytes) {
	int n, lp;
	mapper->where_is(pid, n, lp);

	char * RESTRICT pointer;
	DelivElement element;
	element.size = (unsigned int) nbytes;
	element.info.put.dst = BSP->get_memreg_address(dst, pid) + offset;
	pointer = deliveryTable_push2(&g_bsp.delivery_table, n, &element, it_put);
	memcpy(pointer, src, nbytes);
}

void bsp::Context::bsp_get (int pid, const void *src, long int offset, void *dst, size_t nbytes) {
	TSLOCK();
	BSP->bsp_get(pid, src, offset, dst, nbytes);
}

/*@}*/

/** @name BSMP */
/*@{*/
void bsp::Context::bsp_send ( int pid, const void *tag, const void *payload, size_t payload_nbytes) {
	TSLOCK();
	BSP->bsp_send(pid, tag, payload, payload_nbytes);
}

void bsp::Context::bsp_qsize (int * nmessages, size_t * accum_nbytes) {
	TSLOCK();
	BSP->bsp_qsize(nmessages, accum_nbytes);
}

void bsp::Context::bsp_get_tag (int * status , void * tag) {
	TSLOCK();
	BSP->bsp_get_tag(status, tag);
}

void bsp::Context::bsp_move (void *payload, size_t reception_nbytes) {
	TSLOCK();
	BSP->bsp_move(payload, reception_nbytes);
}

void bsp::Context::bsp_set_tagsize (size_t * tag_nbytes) {
	TSLOCK();
	BSP->bsp_set_tagsize(tag_nbytes);
}

/*@}*/

/** @name High Performance */
/*@{*/
void bsp::Context::bsp_hpput (int pid, const void * src, void * dst, long int offset, size_t nbytes) {
	TSLOCK();
	BSP->bsp_hpput(pid, src, dst, offset, nbytes);
}

void bsp::Context::bsp_hpget (int pid, const void * src, long int offset, void * dst, size_t nbytes) {
	TSLOCK();
	BSP->bsp_hpget(pid, src, offset, dst, nbytes);
}

int bsp::Context::bsp_hpmove (void ** tag_ptr, void ** payload_ptr) {
	TSLOCK();
	BSP->bsp_hpmove(tag_ptr, payload_ptr);
	return 0;
}
/*@}*/
