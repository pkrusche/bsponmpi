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

#include "bsp_exptable.h"
#include "bsp_alloc.h"
#include "bsp_abort.h"
/** @file bsp_exptable.c
    Implements the communication method on an ExpandableTable 
    @author Wijnand Suijlen */
    
/** communicates the table to the other processor. Each column of the table
 * corresponds to a chunk of data which is to be send to the processor with
 * rank equal to the column number. The received data is equally ordered,
 * i.e.: each column corresponds to the data received from the processor with
 * rank equal to the column number.
   @param send Reference to a table which should be send to the other processors
   @param recv Reference to a table which could contain the data received.
   This table is expanded if necessary
   @param communicator MPI Communicator group which exchange the tables.
 */  
void 
expandableTable_comm (const ExpandableTable * RESTRICT send, 
                      ExpandableTable *RESTRICT  recv, MPI_Comm communicator)
{
  unsigned int i;

/* initalize recv offsets */
  for (i = 0; i < recv->nprocs; i++)
    {
      recv->offset[i] = i * recv->rows * recv->slot_size;
      send->offset[i] = i * send->rows * send->slot_size;
      send->bytes[i] = send->used_slot_count[i] * send->slot_size;
      recv->bytes[i] = recv->used_slot_count[i] * recv->slot_size;
    }
  /* the next thing to do is to walk the send table and actually send the 
   * data */
  MPI_Alltoallv (send->data, send->bytes, send->offset, MPI_BYTE,
      recv->data, recv->bytes, recv->offset, MPI_BYTE, communicator);
}


