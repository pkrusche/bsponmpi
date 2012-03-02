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

#include "bsp_cpp/bsp_cpp.h"

#include "bsp_contextimpl_mpi.h"

#include "bsp_reqtable.h"
#include "bsp_delivtable.h"

/** @name Initialisation and destruction */
/*@{*/

void bsp::Context::initialize_context (TaskMapper * tm, int bsp_pid, Context * parent)  {
	mapper = tm;
	parentcontext = parent;
	local_pid = tm->global_to_local_pid(bsp_pid);
	pid = bsp_pid;
	runme = NULL;	// this is initialized by the code in Runner.h
	impl = new bsp::ContextImpl(tm->nprocs(), pid);
	init ();
}

void bsp::Context::destroy_context () {
	delete (bsp::ContextImpl*) impl;
}
/*@}*/

#define BSP &(((ContextImpl*)impl)->bsp)

void bsp::Context::sync_contexts (bsp::TaskMapper * tm) {
#ifdef _HAVE_MPI
	unsigned int maxreqrows = 0, maxdelrows = 0;

	unsigned int any_messages = 0;
	unsigned int any_gets = 0; 
	unsigned int any_hp   = 0;

	/* reset message buffers */	
	for(int j = 0; j < tm->procs_per_node(); ++j) {
		ContextImpl * ci = ((ContextImpl*)tm->get_context(j).impl);
		BSPObject * bsp =  &(ci->bsp);

		messageQueue_sync(& (bsp->message_queue));
		requestTable_reset( & (bsp->request_received_table) );
		deliveryTable_reset( & (bsp->delivery_received_table) );

		/* communicate information */
		for (int p = 0; p < (unsigned)bsp->nprocs; p++) {
			any_gets	 |= bsp->request_table.used_slot_count[p];
			any_messages |= bsp->delivery_table.used_slot_count[p];
		}
		any_hp |= ci->any_hp ? 4 : 0;
	}

	any_gets     = any_gets		? 1 : 0;
	any_messages = any_messages ? 2 : 0;
	any_hp       = any_hp		? 4 : 0;

	for (int p = 0; p < (unsigned)tm->nprocs(); p++) {
		(BSP)->send_index[3*p    ] = (BSP)->request_table.used_slot_count[p];
		(BSP)->send_index[3*p + 1] = (BSP)->delivery_table.used_slot_count[p];
		(BSP)->send_index[3*p + 2] = any_gets | any_messages | any_hp;
	}  

	infocomm (	(BSP)->send_index, 3*sizeof(unsigned int), 
				(BSP)->recv_index, 3*sizeof(unsigned int)
	);

	/* expand buffers if necessary */
	maxreqrows = array_max((BSP)->recv_index, 3*(BSP)->nprocs, 3);
	for (p = 0; p < (unsigned)(BSP)->nprocs; p++)
		maxdelrows = MAX( (BSP)->recv_index[1 + 3*p] + 
		(BSP)->request_table.info.req.data_sizes[p], maxdelrows);

	if ( (BSP)->request_received_table.rows < maxreqrows )
	{
		maxreqrows = MAX((BSP)->request_received_table.rows, maxreqrows);
		requestTable_expand(&(BSP)->request_received_table, maxreqrows);
	}  

	if ((BSP)->delivery_received_table.rows < maxdelrows )
	{
		maxdelrows = MAX((BSP)->delivery_received_table.rows, maxdelrows);
		deliveryTable_expand(&(BSP)->delivery_received_table, maxdelrows );
	}  

	/* copy necessary indices to received_tables */
	for (p = 0; p < (unsigned)(BSP)->nprocs; p++) 
	{
		(BSP)->request_received_table.used_slot_count[p] = (BSP)->recv_index[3*p];
		(BSP)->delivery_received_table.used_slot_count[p] =
			(BSP)->recv_index[1 + 3*p] + (BSP)->request_table.info.req.data_sizes[p] ;
	}	

	/* Now we may conclude something about the communcation pattern */
	any_gets = 0;
	for (p = 0; p < (unsigned)(BSP)->nprocs; p++)   
		any_gets |= (BSP)->recv_index[3*p + 2];

	/* communicate & execute */
	if (any_gets) 
	{
		expandableTable_comm(&(BSP)->request_table, &(BSP)->request_received_table,
			communicator);
		requestTable_execute(&(BSP)->request_received_table, &(BSP)->delivery_table);
	}

	expandableTable_comm(&(BSP)->delivery_table, &(BSP)->delivery_received_table,
		communicator);
	deliveryTable_execute(&(BSP)->delivery_received_table, 
		&(BSP)->memory_register, &(BSP)->message_queue, (BSP)->rank);
	
	/* clear the buffers */			
	requestTable_reset(&(BSP)->request_table);
	deliveryTable_reset(&(BSP)->delivery_table);

	/* pack the memoryRegister */
	memoryRegister_pack(&(BSP)->memory_register);
#else
	// execute by running memcpy
	for(int j = 0; j < tm->procs_per_node(); ++j) {
		BSPObject * bsp =  &(((ContextImpl*)tm->get_context(j).impl)->bsp);
		bspx_sync(bsp, _BSP_COMM0, _BSP_COMM1);
	}
#endif
}

/** Thread safety helper */
#define TSLOCK() tbb::spin_mutex::scoped_lock l (ContextImpl::context_mutex)

/** @name DRMA */
/*@{*/
void bsp::Context::bsp_push_reg (const void * data, size_t len) {
	TSLOCK();
	bspx_push_reg(BSP, data, len);
}

void bsp::Context::bsp_pop_reg (const void * data) {
	TSLOCK();
	bspx_pop_reg(BSP, data);
}

void bsp::Context::bsp_put (int pid, const void *src, void *dst, long int offset, size_t nbytes) {
	TSLOCK();
	bspx_put(BSP, pid, src, dst, offset, nbytes);
}

void bsp::Context::bsp_get (int pid, const void *src, long int offset, void *dst, size_t nbytes) {
	TSLOCK();
	bspx_get(BSP, pid, src, offset, dst, nbytes);
}

/*@}*/

/** @name BSMP */
/*@{*/
void bsp::Context::bsp_send ( int pid, const void *tag, const void *payload, size_t payload_nbytes) {
	TSLOCK();
	bspx_send(BSP, pid, tag, payload, payload_nbytes);
}

void bsp::Context::bsp_qsize (int * nmessages, size_t * accum_nbytes) {
	TSLOCK();
	bspx_qsize(BSP, nmessages, accum_nbytes);
}

void bsp::Context::bsp_get_tag (int * status , void * tag) {
	TSLOCK();
	bspx_get_tag(BSP, status, tag);
}

void bsp::Context::bsp_move (void *payload, size_t reception_nbytes) {
	TSLOCK();
	bspx_move(BSP, payload, reception_nbytes);
}

void bsp::Context::bsp_set_tagsize (size_t * tag_nbytes) {
	TSLOCK();
	bspx_set_tagsize(BSP, tag_nbytes);
}

/*@}*/

/** @name High Performance */
/*@{*/
void bsp::Context::bsp_hpput (int, const void *, void *, long int, size_t) {
	TSLOCK();
	throw std::runtime_error("bsp_hpput is not supported.");
}

void bsp::Context::bsp_hpget (int, const void *, long int, void *, size_t) {
	TSLOCK();
	throw std::runtime_error("bsp_hpget is not supported.");
}

int bsp::Context::bsp_hpmove (void **, void **) {
	TSLOCK();
	throw std::runtime_error("bsp_hpmove is not supported.");
	return 0;
}
/*@}*/
