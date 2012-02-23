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

#include <assert.h>
#include "bsp_exptable.h"

void clean(union SpecInfo info) { }

int main(int argc, char *argv[])
{
  ExpandableTable table, recv;
  union SpecInfo info;
  int i, j;
  int NPROCS;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &NPROCS);
  /* initialize */
  memset(&info, 0, sizeof(union SpecInfo));
  expandableTable_initialize(&table, NPROCS, 5, 1, info);
  expandableTable_initialize(&recv, NPROCS, 100, 1,info);
  assert(NPROCS == table.nprocs);
  assert(5 == table.rows);
  assert(1 == table.slot_size);
  assert(table.data!=NULL);
  /* write in the data array */
  for(i =0; i < NPROCS*5; i++)
    ((char *)table.data)[i] = 'a';
    
  for(i=0; i < NPROCS; i++) 
    {
      assert(0 == table.used_slot_count[i]);
      table.used_slot_count[i] = 5;
    }
 
  /* expand */
  expandableTable_expand(&table, 10, info);
  assert(15 == table.rows);
  assert(1 == table.slot_size);
  assert(NULL != table.data);
  for (i=0; i < NPROCS*15; i++)
    ((char *)table.data)[i] = i % 15 < 5?'a':'b';  
  for (i=0; i < NPROCS; i++)
    {
      assert(5 == table.used_slot_count[i]);
      table.used_slot_count[i] += i;
      recv.used_slot_count[i] = table.used_slot_count[i];
    }
  
  expandableTable_comm(&table, &recv, MPI_COMM_WORLD);
  assert(NPROCS == recv.nprocs);
  assert(1 == recv.slot_size);
  assert(recv.data != NULL);
  for (i = 0; i < NPROCS ; i++) 
    { 
      for (j = 0; j < 5 + i; j++) 
        {
          assert( j < 5? 'a': 'b' == ((char *) recv.data)[i*recv.rows + j]);
        }  
    }	
  
  /* reset */ 
  expandableTable_reset(&table);
  for(i=0; i < NPROCS; i++) 
    {
      assert(0 == table.used_slot_count[i]);
    }  
  expandableTable_destruct(&table);
  expandableTable_destruct(&recv);
  // do some silly things with it
  table.nprocs = 10;
  table.rows = 1000;

  MPI_Finalize();
  return 0;
}

