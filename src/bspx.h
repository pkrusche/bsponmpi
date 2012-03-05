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


/** @file bsp.h
Defines the BSPx prototypes. 

BSPx allows to use BSPonMPI's message buffering with bspwww-style 
primitives, but communication is done by the user.

Note BSPx primitives are not threadsafe. They are used from bsp_cpp,
which can use tbb mutexes rather than OS standard ones.

@author Peter Krusche
*/

#ifndef BSPX_H
#define BSPX_H

#include <stdarg.h>
#include <stdlib.h>

#include "bsp_config.h"

#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "bsp_private.h"

	/** @name Initialisation */
	/*@{*/
	void bspx_init_bspobject (BSPObject *, int, int );
	void bspx_destroy_bspobject (BSPObject * );
	/*@}*/

	/** @name Superstep */
	/*@{*/
	void bspx_sync (BSPObject *, BSPX_CommFn0,  BSPX_CommFn);
	/*@}*/
	

	/** @name DRMA */
	/*@{*/
	void bspx_push_reg (BSPObject *, const void *, size_t);
	void bspx_pop_reg (BSPObject *, const void *);
	void bspx_put (BSPObject *, int, const void *, void *, long int, size_t);
	void bspx_get (BSPObject *, int, const void *, long int, void *, size_t);
	/*@}*/

	/** @name BSMP */
	/*@{*/
	void bspx_send (BSPObject *, int, const void *, const void *, size_t);
	void bspx_qsize (BSPObject *, int * RESTRICT , size_t * RESTRICT );
	void bspx_get_tag (BSPObject *, int * RESTRICT , void * RESTRICT );
	void bspx_move (BSPObject *, void *, size_t);
	void bspx_set_tagsize (BSPObject *, size_t *);
	/*@}*/

	/** @name High Performance */
	/*@{*/
	void bspx_hpput (BSPObject *, int, const void *, void *, long int, size_t);
	void bspx_hpget (BSPObject *, int, const void *, long int, void *, size_t);
	int bspx_hpmove (BSPObject *, void **, void **);
	/*@}*/

	/** @section Global (BSPRAM) DRMA */
	/*@{*/

	bsp_global_handle_t BSP_CALLING bspx_global_alloc(BSPObject *, size_t array_size);
	void BSP_CALLING bspx_global_free(BSPObject *, bsp_global_handle_t ptr);
	void BSP_CALLING bspx_global_get(BSPObject *, bsp_global_handle_t src, size_t offset, void * dest, size_t size);
	void BSP_CALLING bspx_global_put(BSPObject *, const void * src, bsp_global_handle_t dest, size_t offset, size_t size);
	void BSP_CALLING bspx_global_hpget(BSPObject *, bsp_global_handle_t src, size_t offset, void * dest, size_t size);
	void BSP_CALLING bspx_global_hpput(BSPObject *, const void * src, bsp_global_handle_t dest, size_t offset, size_t size);
	/*@}*/

	/** @name Timing and benchmarking */
	/*@{*/
	double BSP_CALLING bsp_time ();
	void BSP_CALLING bsp_warmup (double);
	/*@}*/


#ifdef __cplusplus
};
#endif // __cplusplus


#endif
