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
Defines the BSPlib prototypes. This include file should be used when
writing a BSP program and is therefore  part of the installation.  
@author Wijnand Suijlen
*/

#ifndef BSP_H
#define BSP_H

#include <stdarg.h>
#include <stdlib.h>

#include "bsp_config.h"

/** These control the default buffer sizes.
 *  They could be tuned to optimize behaviour of the library
 *  towards space or time efficiency.
 *  
 *  Changing them requires recompilation.
 */

#ifndef BSP_DELIVTAB_MIN_SIZE
#define BSP_DELIVTAB_MIN_SIZE 256*8
#endif

#ifndef BSP_REQTAB_MIN_SIZE
#define BSP_REQTAB_MIN_SIZE   256
#endif

#ifndef BSP_MEMREG_MIN_SIZE
#define BSP_MEMREG_MIN_SIZE  1	
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef int bsp_global_handle_t;

	/** @name Initialisation */
	/*@{*/
	void BSP_CALLING bsp_init (int*, char **[]);
	void BSP_CALLING bsp_end ();
	/*@}*/
	/** @name Halt */
	/*@{*/
	void BSP_CALLING bsp_abort (const char *format, ...);
	/*@}*/

	/** @name Enquiry */
	/*@{*/
	int BSP_CALLING bsp_nprocs ();
	int BSP_CALLING bsp_pid ();
	double BSP_CALLING bsp_time ();
	/*@}*/

	/** @name Superstep */
	/*@{*/
	void BSP_CALLING bsp_sync ();
	void BSP_CALLING bsp_reset_buffers();
	/*@}*/

	/** @name DRMA */
	/*@{*/
	void BSP_CALLING bsp_push_reg (const void *, size_t);
	void BSP_CALLING bsp_pop_reg (const void *);
	void BSP_CALLING bsp_put (int, const void *, void *, long int, size_t);
	void BSP_CALLING bsp_get (int, const void *, long int, void *, size_t);
	/*@}*/

	/** @name BSMP */
	/*@{*/
	void BSP_CALLING bsp_send (int, const void *, const void *, size_t);
	void BSP_CALLING bsp_qsize (int * RESTRICT , size_t * RESTRICT );
	void BSP_CALLING bsp_get_tag (int * RESTRICT , void * RESTRICT );
	void BSP_CALLING bsp_move (void *, size_t);
	void BSP_CALLING bsp_set_tagsize (size_t *);
	/*@}*/

	/** @name High Performance */
	/*@{*/
	void BSP_CALLING bsp_hpput (int, const void *, void *, long int, size_t);
	void BSP_CALLING bsp_hpget (int, const void *, long int, void *, size_t);
	int BSP_CALLING bsp_hpmove (void **, void **);
	/*@}*/

	/** @name Global DRMA */
	/*@{*/
	bsp_global_handle_t BSP_CALLING bsp_global_alloc(size_t array_size);
	void BSP_CALLING bsp_global_free(bsp_global_handle_t ptr);
	void BSP_CALLING bsp_global_get(bsp_global_handle_t src, size_t offset, void * dest, size_t size);
	void BSP_CALLING bsp_global_put(const void * src, bsp_global_handle_t dest, size_t offset, size_t size);
	void BSP_CALLING bsp_global_hpget(bsp_global_handle_t src, size_t offset, void * dest, size_t size);
	void BSP_CALLING bsp_global_hpput(const void * src, bsp_global_handle_t dest, size_t offset, size_t size);
	/*@}*/

	/** @name Timing and benchmarking */
	/*@{*/
	double BSP_CALLING bsp_time ();
	double BSP_CALLING bsp_dtime ();
	void BSP_CALLING bsp_warmup (double);
	/*@}*/


#ifdef __cplusplus
};
#endif // __cplusplus


#endif
