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

#include "bsp_memreg.h"
#include "bsp_alloc.h"

/* NPROCS should be defined as an integer at least 2 */
#define NPROCS 2

int
main (int argc, char *argv[])
{
  int testint = 2;
  char testchar = 'a';
  char *teststring = "bladiebla";
  double testdouble = 3.14159265358979;
  
  int i;
  
  ExpandableTable memreg;
  memoryRegister_initialize(&memreg, NPROCS, 1, 0 );

  for (i = 0; i < NPROCS; i++)
    memoryRegister_push(&memreg, i, (char *) (&testint + i));
  
  for (i = 0; i < NPROCS; i++)
    {
       memoryRegister_push(&memreg, i, &testchar + i);
       memoryRegister_push(&memreg, i, (char *) (&teststring + i));
    }  
 
  for (i = 0; i < NPROCS; i++)
    memoryRegister_push(&memreg, i, (char *) (&testdouble + i));
  
  for (i = 0; i < NPROCS; i++)
    {
      assert(memoryRegister_find(&memreg, 0, i, (char *) &testint) == (char*)(&testint + i ));
      assert(memoryRegister_find(&memreg, 0, i, &testchar) ==&testchar + i);
      assert(memoryRegister_find(&memreg, i, 0, (char*)(&teststring+i)) ==
                                               (char*)  &teststring );
      assert(memoryRegister_find(&memreg, i, 1, (char *) (&testdouble + i)) ==
                                              (char *) (&testdouble + 1));
    } 
 
  memoryRegister_pop(&memreg, 0, (char *) &teststring);
  memoryRegister_pop(&memreg, 1, &testchar + 1);
  for (i = 0; i < NPROCS; i++)
    memoryRegister_push(&memreg, i, (char *) (&testint + 2*i));

  for (i = 0; i < NPROCS; i++)
    assert(memoryRegister_find(&memreg, 0, i, (char *) &testint) == (char *)(&testint + 2*i) );

  memoryRegister_pack(&memreg);
  memoryRegister_pack(&memreg);
  memoryRegister_pack(&memreg);
  for (i = 0; i < NPROCS; i++)
    {
      assert(memoryRegister_memoized_find(&memreg, i, (char *)&testint)== (char*)(&testint + 2*i ));
      assert(memoryRegister_find(&memreg, i, 1, (char *)(&testdouble + i)) ==
                                              (char *)(&testdouble + 1));
    } 

  
  memoryRegister_destruct(&memreg);
  return 0;
}  

