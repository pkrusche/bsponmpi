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
#include "bsp_reqtable.h"
#include "bsp_delivtable.h"

#define NPROCS 2

int 
main(int argc, char *argv[])
{
  ExpandableTable reqtab, delivtab;
  ReqElement req1, req2;
  int a = 3, b = 10;
  int x = -1, y = -2;
  req2.size = req1.size = sizeof(int);
  req2.offset = req1.offset = 0;
  req2.src=(char *) &a ; req1.src = (char *) &b;
  req2.dst=(char *) &x ; req1.dst = (char *) &y;
 
  requestTable_initialize(&reqtab, NPROCS, 1);
  deliveryTable_initialize(&delivtab, NPROCS, 1);
  requestTable_push(&reqtab, 0, &req1);
  requestTable_push(&reqtab, 0, &req2);

  requestTable_execute(&reqtab, &delivtab);
  deliveryTable_execute(&delivtab, NULL, NULL, 0);

  assert(x == 3);
  assert(y == 10);

  requestTable_destruct(&reqtab);
  deliveryTable_destruct(&delivtab);
  return 0;
}  
