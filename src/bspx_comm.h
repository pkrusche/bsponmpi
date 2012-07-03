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

/** @file bspx_comm.h
	@brief typedef of BSPX_CommFn, the core communication primitive used
	@author Peter Krusche
  */  

#ifndef __bspx_comm_H__
#define __bspx_comm_H__

/** pointer to wrapper for MPI_Alltoall */
typedef void (*BSPX_CommFn0) (void * , int, void * , int );

/** This is a pointer to a wrapper for (something like) MPI_Alltoallv */
typedef void (*BSPX_CommFn) (void * sendbuf, int * sendcounts, int * sendoffsets,
	void * recvbuf, int * recvcounts, int * recvoffsets );


#endif // __bspx_comm_H__
