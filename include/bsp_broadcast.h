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

#ifndef __BSP_BROADCAST_H__
#define __BSP_BROADCAST_H__

#include "bsp_config.h"

#ifndef _SEQUENTIAL
#include "mpi.h"

static inline void bsp_broadcast(int source, void* source_data, size_t len) {
	MPI_Bcast(source_data, (int)len, MPI_BYTE, source, MPI_COMM_WORLD);
}
#else
static inline void bsp_broadcast(int source, void* source_data, size_t len) {
}
#endif // _HAVE_MPI

#endif

