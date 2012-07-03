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

/** @file bspx_comm_mpi.h
 
	Declaration of communication routines on top of MPI.

	@author Peter Krusche
  */  

#ifndef __bsp_mpi_comm_H__
#define __bsp_mpi_comm_H__

#include <mpi.h>

void BSP_INIT_MPI (int * pargc, char *** pargv, void * o);
void BSP_EXIT_MPI ();
void BSP_ABORT_MPI (int );

/** this communicator will be used by all communication routines */
extern MPI_Comm bsp_communicator;

/** MPI_Alltoall wrapper */
void BSP_MPI_ALLTOALL_COMM (void * sendbuf, int  sendcount, void * recvbuf, int  recvcount);

/** MPI_Alltoallv wrapper */
void BSP_MPI_ALLTOALLV_COMM (void * sendbuf, int * sendcounts, int * sendoffsets,
	void * recvbuf, int * recvcounts, int * recvoffsets );

#endif // __bsp_mpi_comm_H__
