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

/** @file bsp_abort.c
 * implements the internal abort function: bsp_intern_abort()
 * @author Wijnand Suijlen
 */
#include <stdio.h>
#include "bsp_config.h"
#include "bsp_abort.h"
#include <stdlib.h>

#ifdef _HAVE_MPI
#include "bspx_comm_mpi.h"
#endif 

#include "bspx_comm_seq.h"


void
bsp_intern_abort (const int err_number, const char *func,
                  const char *file, int line)
{
  const char *messages[] = ERR_MESSAGES;
  fprintf (stderr, "%s at %s:%d: %s\n", func, file, line, 
            messages[err_number - 1]);
  fflush (stderr);
  _BSP_ABORT (err_number);
}


