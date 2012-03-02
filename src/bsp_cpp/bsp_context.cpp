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

void bsp::Context::initialize_context (TaskMapper * tm, int bsp_pid, Context * parent)  {
	mapper = tm;
	parentcontext = parent;
	local_pid = tm->global_to_local_pid(bsp_pid);
	pid = bsp_pid;
	runme = NULL;	// this is initialized by the code in Runner.h
	impl = new bsp::ContextImpl();
	init ();
}

void bsp::Context::destroy_context () {
	delete (bsp::ContextImpl*) impl;
}


/** @name DRMA */
/*@{*/
void bsp::Context::bsp_push_reg (const void *, size_t) {

}

void bsp::Context::bsp_pop_reg (const void *) {

}

void bsp::Context::bsp_put (int, const void *, void *, long int, size_t) {

}

void bsp::Context::bsp_get (int, const void *, long int, void *, size_t) {

}

/*@}*/

/** @name BSMP */
/*@{*/
void bsp::Context::bsp_send (int, const void *, const void *, size_t) {

}

void bsp::Context::bsp_qsize (int * RESTRICT , size_t * RESTRICT ) {

}

void bsp::Context::bsp_get_tag (int * RESTRICT , void * RESTRICT ) {

}

void bsp::Context::bsp_move (void *, size_t) {

}

void bsp::Context::bsp_set_tagsize (size_t *) {

}

/*@}*/

/** @name High Performance */
/*@{*/
void bsp::Context::bsp_hpput (int, const void *, void *, long int, size_t) {

}

void bsp::Context::bsp_hpget (int, const void *, long int, void *, size_t) {

}

int bsp::Context::bsp_hpmove (void **, void **) {
	return 0;
}
/*@}*/
