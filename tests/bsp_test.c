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
                                                                                
    You may contact me via electronic mail:
      wjsuijle@users.sourceforge.net
    or snail mail:
      W.J. Suijlen
      Kraaiheidelaan 10
      2803 VP Gouda
      The Netherlands
*/

#include "bsp_test.h"
#include <stdlib.h>
#include <memory.h>

int 
MPI_Alltoall(void * in, int nin, int sz_in, void *out, int nout, int sz_out,
             int bla) 
{
  memcpy(out, in, nout*sz_out*NPROCS);
  assert(sz_in == sz_out);
  assert(nin == nout);
  return 0;
}

int 
MPI_Alltoallv(void * in, int* bytes_in, int* offset_in , int sz_in,
              void *out, int* bytes_out, int* offset_out, int sz_out,
             int bla) 
{
  int i;
  assert(sz_in == sz_out);
  for (i = 0; i < NPROCS; i++)
    {
      memcpy( ((char *) out) + offset_out[i], 
              ((char *) in) + offset_in[i], bytes_out[i] * sz_out);
      assert(bytes_in[i] == bytes_out[i]);
    }  
  return 0;  
}

int MPI_Initialized(int *flag) { *flag = 0; return 0;}
int MPI_Abort(int comm, int errc)
{
  exit(errc);
  return 0;
}  

