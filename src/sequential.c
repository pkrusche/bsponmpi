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

/** @file bsp_exptable.c
    Implements MPI_Alltoall[v] on one processor if no MPI library is used.
    @author Peter Krusche */

#ifdef _SEQUENTIAL

#include "bsp_config.h"
#include "bsp_mpistub.h"
#include <stdlib.h>
#include <string.h>

int ompi_mpi_comm_world = 0;
int ompi_mpi_byte = 0;

int 
MPI_Comm_rank(MPI_Comm c, int * r) {
	*r = 0;
	return 0;
}

int 
MPI_Alltoall(void * in, int nin, int sz_in, void *out, int nout, int sz_out,
			 int bla) 
{
	memcpy(out, in, nout*sz_out);
	assert(sz_in == sz_out);
	assert(nin == nout);
	return 0;
}

int 
MPI_Alltoallv(void * in, int* bytes_in, int* offset_in , int sz_in,
			  void *out, int* bytes_out, int* offset_out, int sz_out,
			  int bla) 
{
	int copy_size = bytes_in[0] > bytes_out[0] ? bytes_out[0] : bytes_in[0];
	assert(sz_in == sz_out);
	memcpy( ((char *) out) + offset_out[0], 
		    ((char *) in) + offset_in[0], copy_size * sz_out);
    
	return 0;  
}

int MPI_Initialized(int *flag) { *flag = 0; return 0;}
int MPI_Abort(int comm, int errc)
{
	exit(errc);
	return 0;
}  

#endif // _SEQUENTIAL
