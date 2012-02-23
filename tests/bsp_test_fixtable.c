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

#define NPROCS 2

/* NPROCS should be defined at least 2 */
void doNothingHere(union SpecInfo *info, unsigned int bla, unsigned int bl) { }

int main(int argc, char *argv[])
{
  ExpandableTable table;
  union SpecInfo info;
  char a = 'a', b = 'b';
  
  memset(&info, 0, sizeof(union SpecInfo));
  fixedElSizeTable_initialize(&table, NPROCS, 1, 1, info);
  assert(table.rows == 1);
  fixedElSizeTable_push(&table, 0, &doNothingHere, &a);
  assert(table.data[0] == 'a');
  assert(table.used_slot_count[0] == 1);
  assert(table.used_slot_count[1] == 0);
 
  fixedElSizeTable_push(&table, 1, &doNothingHere,  &a); 
  assert(table.data[0] == 'a');
  assert(table.used_slot_count[0] == 1);
  assert(table.data[table.rows] == 'a');
  assert(table.used_slot_count[1] == 1);
  
  fixedElSizeTable_push(&table, 1, &doNothingHere, &b); 
  assert(table.data[table.rows + 1] == 'b');
  assert(table.used_slot_count[1] == 2);
  assert(table.rows == 2);
  assert(table.data[0] == 'a');
  assert(table.used_slot_count[0] == 1);
  assert(table.data[table.rows] == 'a');
  
  expandableTable_destruct(&table);
  return 0;
}
