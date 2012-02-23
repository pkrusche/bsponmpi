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

/** @file bsp_mpistub.h
 * Contains definitions for MPI stub functions if no MPI is used.
 * 
 * @author Peter Krusche
 */

#ifndef bsp_mpistub_h__
#define bsp_mpistub_h__

#include <stdio.h>
#include <assert.h>

#include "bsp_config.h"

#define MPI_Datatype  int
#define MPI_COMM_WORLD 0
#define MPI_INT      sizeof(int)
#define MPI_BYTE     sizeof(char)
#define MPI_UNSIGNED sizeof(unsigned int)

typedef int MPI_Comm;
typedef int MPI_Group;

int 
MPI_Alltoall(void * in, int nin, int sz_in, void *out, int nout, int sz_out,
			 MPI_Comm bla) ;

int 
MPI_Alltoallv(void * in, int* bytes_in, int* offset_in , int sz_in,
			  void *out, int* bytes_out, int* offset_out, int sz_out,
			  MPI_Comm bla) ;

int MPI_Initialized(int *flag);
int MPI_Abort(int comm, int errc);
#endif // bsp_mpistub_h__

