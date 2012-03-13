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

extern "C" {
#include "bsp_private.h"
};

#include "bsp_context_ts.h"

/** @name Initialisation and destruction */
/*@{*/

void bsp::Context::initialize_context (int bsp_pid, Context * parent)  {
	ASSERT (parent != NULL);
	
	parentcontext = parent;
	local_pid = mapper->global_to_local_pid(bsp_pid);
	pid = bsp_pid;
	impl = new bsp::ContextImpl(mapper, local_pid);
	
	// update all shared variables from parent.
//	context_sharing.update(parent->context_sharing);
	init ();
}

void bsp::Context::destroy_context () {
	delete (bsp::ContextImpl*) impl;
}
/*@}*/

#define BSP ((ContextImpl*)impl)

void bsp::Context::bsp_sync () {
	ASSERT (!impl);	/// only the parent context can call bsp_sync.
	ASSERT (mapper);/// we can only sync if we have a task mapper
	ContextImpl::bsp_sync( mapper );
}

void bsp::Context::bsp_reset_buffers () {
	if (impl != NULL) {
		BSP->bsp_reset_buffers();
	} else {
		::bsp_reset_buffers();
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

void bsp::Context::bsp_put (int pid, const void *src, void *dst, long int offset, size_t nbytes) {
	BSP->bsp_put(pid, src, dst, offset, nbytes);
}

void bsp::Context::bsp_get (int pid, const void *src, long int offset, void *dst, size_t nbytes) {
	BSP->bsp_get(pid, src, offset, dst, nbytes);
}

/*@}*/

/** @name BSMP */
/*@{*/
void bsp::Context::bsp_send ( int pid, const void *tag, const void *payload, size_t payload_nbytes) {
	BSP->bsp_send(pid, tag, payload, payload_nbytes);
}

void bsp::Context::bsp_hpsend (int pid, const void *tag, const void *payload, size_t payload_nbytes) {
	BSP->bsp_hpsend(pid, tag, payload, payload_nbytes);
}

void bsp::Context::bsp_qsize (int * nmessages, size_t * accum_nbytes) {
	BSP->bsp_qsize(nmessages, accum_nbytes);
}

void bsp::Context::bsp_get_tag (int * status , void * tag) {
	BSP->bsp_get_tag(status, tag);
}

void bsp::Context::bsp_move (void *payload, size_t reception_nbytes) {
	BSP->bsp_move(payload, reception_nbytes);
}

void bsp::Context::bsp_set_tagsize (size_t * tag_nbytes) {
	BSP->bsp_set_tagsize(tag_nbytes);
}

/*@}*/

/** @name High Performance */
/*@{*/
void bsp::Context::bsp_hpput (int pid, const void * src, void * dst, long int offset, size_t nbytes) {
	BSP->bsp_hpput(pid, src, dst, offset, nbytes);
}

void bsp::Context::bsp_hpget (int pid, const void * src, long int offset, void * dst, size_t nbytes) {
	BSP->bsp_hpget(pid, src, offset, dst, nbytes);
}

int bsp::Context::bsp_hpmove (void ** tag_ptr, void ** payload_ptr) {
	BSP->bsp_hpmove(tag_ptr, payload_ptr);
	return 0;
}
/*@}*/
