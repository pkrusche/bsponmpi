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

/** Mutex to make access to message buffers thread safe */

namespace bsp {
	static tbb::spin_mutex g_context_mutex;
};

/** Thread safety helper */
#define TSLOCK() tbb::spin_mutex::scoped_lock l (bsp::g_context_mutex)


/** @name Initialisation and destruction */
/*@{*/

void bsp::Context::initialize_context (TaskMapper * tm, int bsp_pid, Context * parent)  {
	mapper = tm;
	parentcontext = parent;
	local_pid = tm->global_to_local_pid(bsp_pid);
	pid = bsp_pid;
	runme = NULL;	// this is initialized by the code in Runner.h
	impl = new bsp::ContextImpl(tm, local_pid);
	init ();
}

void bsp::Context::destroy_context () {
	delete (bsp::ContextImpl*) impl;
}
/*@}*/

#define BSP ((ContextImpl*)impl)

void bsp::Context::sync_contexts (bsp::TaskMapper * tm) {
	ContextImpl::bsp_sync(tm);
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
	TSLOCK();
	BSP->bsp_put(pid, src, dst, offset, nbytes);
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
