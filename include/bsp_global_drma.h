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

#ifndef bsp_global_drma_h__
#define bsp_global_drma_h__

#include "bsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef int bsp_global_handle_t;

bsp_global_handle_t BSP_CALLING bsp_global_alloc(size_t array_size);
void BSP_CALLING bsp_global_free(bsp_global_handle_t ptr);
void BSP_CALLING bsp_global_get(bsp_global_handle_t src, size_t offset, void * dest, size_t size);
void BSP_CALLING bsp_global_put(const void * src, bsp_global_handle_t dest, size_t offset, size_t size);
void BSP_CALLING bsp_global_hpget(bsp_global_handle_t src, size_t offset, void * dest, size_t size);
void BSP_CALLING bsp_global_hpput(const void * src, bsp_global_handle_t dest, size_t offset, size_t size);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // bsp_global_drma_h__
