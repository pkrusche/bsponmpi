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

/** @file bsp_comm_seq.c
    @brief Implements MPI_Alltoall[v] on one processor if no MPI library is used.
    @author Peter Krusche */



#include <stdlib.h>
#include <string.h>

#include "bsp_config.h"
#include "bsp_private.h"
#include "bspx_comm_seq.h"


extern double bsp_begintime;
extern double BSP_CALLING bsp_time();

void BSP_INIT_SEQ (int * pargc, char *** pargv, void * o) {
	BSPObject * bsp = (BSPObject*)o;

	bsp->nprocs = 1;
	bsp->rank = 0;
	bsp_begintime = bsp_time();
}

void BSP_EXIT_SEQ () {}

/** MPI_Alltoall wrapper */
void BSP_SEQ_ALLTOALL_COMM (void * sendbuf, int  sendcount, void * recvbuf, int  recvcount) {
//	ASSERT(sendcount == recvcount);
	memcpy(recvbuf, sendbuf, recvcount);
}

/** MPI_Alltoallv wrapper */
void BSP_SEQ_ALLTOALLV_COMM (void * sendbuf, int * sendcounts, int * sendoffsets,
	void * recvbuf, int * recvcounts, int * recvoffsets ) {
	int copy_size = sendcounts[0] > recvcounts[0] ? recvcounts[0] : sendcounts[0];
	memcpy( ((char *) recvbuf) + recvoffsets[0], 
		((char *) sendbuf) + sendoffsets[0], copy_size );

}

/** abort wrapper */
void BSP_ABORT_SEQ (int err) {
	exit (err);
}
