/*
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
    Copyright (C) 2006  Wijnand J. Suijlen
                                                                                
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

/** @file bspx_global_drma.c
    Implementation of global array DRMA operations
    @author Peter Krusche */

#include "bsp.h"
#include "bspx.h"

#include <stdlib.h>

#include "bsp_exptable.h"
#include "bsp_mesgqueue.h"
#include "bsp_private.h"
#include "bsp_tools/aligned_malloc.h"

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT assert
#else
#define ASSERT(x) 
#endif
#endif

/** Allocate shared memory block
    @param array_size size of block to allocate
    @return a handle to the block
  */  
bsp_global_handle_t BSP_CALLING bspx_global_alloc (BSPObject * bsp, size_t array_size ) {
    int procs = bsp->nprocs;
    bsp_global_handle_t handle;
    size_t alloc_size = ( array_size + procs - 1 ) / procs;

    if ( bsp->global_overflow ) {
        int count = 0;
        while ( bsp->global_arrays[bsp->global_array_last].local_slice != NULL ) {
            ++bsp->global_array_last;
            if ( count++ > BSP_MAX_GLOBAL_ARRAYS ) {
                bsp_abort ( "bspx_global_alloc: ran out of handles." );
            }
        }
    }

    bsp->global_arrays[bsp->global_array_last].array_size = array_size;
    bsp->global_arrays[bsp->global_array_last].local_slice = aligned_malloc ( alloc_size, 32 );
    bsp->global_arrays[bsp->global_array_last].local_size = alloc_size;

    bspx_push_reg (bsp, bsp->global_arrays[bsp->global_array_last].local_slice,
                   bsp->global_arrays[bsp->global_array_last].local_size );

    handle = bsp->global_array_last;
    bsp->global_array_last++;
    bsp->global_array_last %= BSP_MAX_GLOBAL_ARRAYS;
    if ( bsp->global_array_last == 0 ) {
        bsp->global_overflow = 1;
    }
    return handle;
}

/** Free shared memory block
    @param ptr a handle to the block
  */  

void BSP_CALLING bspx_global_free (BSPObject * bsp, bsp_global_handle_t ptr ) {
    bspx_pop_reg (bsp, bsp->global_arrays[ptr].local_slice );
    aligned_free ( bsp->global_arrays[ptr].local_slice );
    bsp->global_arrays[ptr].local_slice = NULL;
}

/** Get data from global shared memory block
    @param src the block handle
    @param offset the offset
    @param dest the destination data pointer
    @param size the size
 */

void BSP_CALLING bspx_global_get (BSPObject * bsp, bsp_global_handle_t src, size_t offset, void * dest, size_t size ) {
	size_t procs = bsp->nprocs;
	size_t gsize = bsp->global_arrays[src].array_size;

	size_t offset_proc = offset * procs / gsize;
	size_t offset_idx = offset - offset_proc*bsp->global_arrays[src].local_size;

	while ( size > 0 ) {

		size_t size_todo = size > bsp->global_arrays[src].local_size - offset_idx ? 
			bsp->global_arrays[src].local_size - offset_idx : size;
		ASSERT(offset_proc < procs);
		ASSERT(offset_idx < bsp->global_arrays[src].local_size);
		ASSERT(offset_idx + size_todo <= bsp->global_arrays[src].local_size);
		bspx_get (bsp, (int)offset_proc, bsp->global_arrays[src].local_slice, (long int)offset_idx, dest, size_todo );
		size-= size_todo;
		dest = ((char*) dest) + size_todo;
		++offset_proc;
		offset_idx = 0;
	}
}

/** Put data to global shared memory block
    @param src the source data
    @param dest the destination block handle
	@param offset the offset
    @param size the size
 */
void BSP_CALLING bspx_global_put (BSPObject * bsp, const void * src, bsp_global_handle_t dest, size_t offset, size_t size ) {
	size_t procs = bsp->nprocs;
	size_t gsize = bsp->global_arrays[dest].array_size;

	size_t offset_proc = offset * procs / gsize;
	size_t offset_idx = offset - offset_proc*bsp->global_arrays[dest].local_size;

	while ( size > 0 ) {
		size_t size_todo = size > bsp->global_arrays[dest].local_size - offset_idx ? 
			bsp->global_arrays[dest].local_size - offset_idx : size;
		ASSERT(offset_proc < procs);
		ASSERT(offset_idx < bsp->global_arrays[dest].local_size);
		ASSERT(offset_idx + size_todo <= bsp->global_arrays[dest].local_size);

		bspx_put (bsp, (int)offset_proc, src, bsp->global_arrays[dest].local_slice, (long int)offset_idx, size_todo );
		src = ((char*) src) + size_todo;
		size-= size_todo;
		++offset_proc;
		offset_idx = 0;
	}

}

/** Get data from global shared memory block (unbuffered)
    @param src the block handle
    @param offset the offset
    @param dest the destination data pointer
    @param size the size
 */

void BSP_CALLING bspx_global_hpget (BSPObject * bsp, bsp_global_handle_t src, size_t offset, void * dest, size_t size ) {
	size_t procs = bsp->nprocs;
	size_t gsize = bsp->global_arrays[src].array_size;

	size_t offset_proc = offset * procs / gsize;
	size_t offset_idx = offset - offset_proc*bsp->global_arrays[src].local_size;

	while ( size > 0 ) {
		size_t size_todo = size > bsp->global_arrays[src].local_size - offset_idx ? 
			bsp->global_arrays[src].local_size - offset_idx : size;
		ASSERT(offset_proc < procs);
		ASSERT(offset_idx < bsp->global_arrays[src].local_size);
		ASSERT(offset_idx + size_todo <= bsp->global_arrays[src].local_size);
		bspx_hpget (bsp, (int)offset_proc, bsp->global_arrays[src].local_slice, (long int)offset_idx, dest, size_todo );
		dest = ((char*) dest) + size_todo;
		size-= size_todo;
		++offset_proc;
		offset_idx = 0;
	}
}

/** Put data to global shared memory block (unbuffered)
    @param src the source data
    @param dest the destination block handle
	@param offset the offset
    @param size the size
 */

void BSP_CALLING bspx_global_hpput (BSPObject * bsp, const void * src, bsp_global_handle_t dest, size_t offset, size_t size ) {
	size_t procs = bsp->nprocs;
	size_t gsize = bsp->global_arrays[dest].array_size;

	size_t offset_proc = offset * procs / gsize;
	size_t offset_idx = offset - offset_proc*bsp->global_arrays[dest].local_size;

	while ( size > 0 ) {
		size_t size_todo = size > bsp->global_arrays[dest].local_size - offset_idx ? 
			bsp->global_arrays[dest].local_size - offset_idx : size;
		ASSERT(offset_proc < procs);
		ASSERT(offset_idx < bsp->global_arrays[dest].local_size);
		ASSERT(offset_idx + size_todo <= bsp->global_arrays[dest].local_size);
		bspx_hpput (bsp, (int)offset_proc, src, bsp->global_arrays[dest].local_slice, (long int)offset_idx, size_todo );
		src = ((char*) src) + size_todo;
		size-= size_todo;
		++offset_proc;
		offset_idx = 0;
	}

}

