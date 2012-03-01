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

/** @file bsp_mpi_comm.c
 
	Implementation of communication routines on top of MPI.

	@author Peter Krusche
  */  

#include <stdio.h>

#include "bsp_config.h"
#include "bsp_private.h"
#include "bsp_alloc.h"
#include "bspx_comm_mpi.h"

MPI_Comm bsp_communicator;

extern double bsp_begintime;
extern double BSP_CALLING bsp_time();

void BSP_INIT_MPI (int * pargc, char *** pargv, void * o) {
	BSPObject * bsp = (BSPObject *)o;
	int flag, i, *ranks;
	MPI_Group group, newgroup;

	MPI_Init(pargc, pargv);
	MPI_Comm_size( MPI_COMM_WORLD, &bsp->nprocs);
	MPI_Comm_rank( MPI_COMM_WORLD, &bsp->rank);

	/* initialize if necessary */
	if (MPI_Initialized(&flag), !flag)
	{
		int argc = 0;
		char **argv = NULL;
		fprintf(stderr, "Warning! bsp_init() is not called. Initialization of MPI may fail\n");
		MPI_Init (&argc, &argv);
		MPI_Comm_size (MPI_COMM_WORLD, &bsp->nprocs);
		MPI_Comm_rank (MPI_COMM_WORLD, &bsp->rank);
	}

	MPI_Comm_group( MPI_COMM_WORLD, &group);
	ranks =(int*) bsp_malloc( bsp->nprocs, sizeof(int));
	for (i = 0; i < bsp->nprocs; i++)
		ranks[i] = i;

	MPI_Group_incl(group, bsp->nprocs, ranks, &newgroup);
	MPI_Comm_create(MPI_COMM_WORLD, newgroup, &bsp_communicator);

	bsp_free(ranks);
	bsp_begintime = bsp_time();
}

void BSP_EXIT_MPI () {
	MPI_Finalize();
}

void BSP_MPI_ALLTOALL_COMM (void * sendbuf, int  sendcount, void * recvbuf, int  recvcount) {
	MPI_Alltoall(sendbuf, sendcount, MPI_BYTE, recvbuf, recvcount, MPI_BYTE, bsp_communicator);
}


/**
 * BSP communicator that uses MPI_Alltoall
 */
void BSP_MPI_ALLTOALLV_COMM (void * sendbuf, int * sendcounts, int * sendoffsets,
	void * recvbuf, int * recvcounts, int * recvoffsets ) {
	MPI_Alltoallv(sendbuf, sendcounts, sendoffsets, MPI_BYTE, 
				  recvbuf, recvcounts, recvoffsets, MPI_BYTE,
				  bsp_communicator);
}

/** MPI_Abort wrapper */
void BSP_ABORT_MPI (int err) {
	int flag;
	MPI_Initialized(&flag);
	if (flag) {
		MPI_Abort(MPI_COMM_WORLD, err);
	} else {
		exit (err);
	}
}

