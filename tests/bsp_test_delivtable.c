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
#include "bsp_mesgqueue.h"
#include "bsp_memreg.h"
#include "bsp_delivtable.h"

#define NPROCS 2

int 
main(int argc, char *argv[])
{
  ExpandableTable memreg, deliv;
  MessageQueue mesgq;
  DelivElement sendobj, pushobj, popobj, settagobj, putobj;
  int a = 0, b = 10, i, *t;
  unsigned int k;
  const int index_size = 
    no_slots(3 * 6 * sizeof(unsigned int), sizeof(ALIGNED_TYPE));

 
  messageQueue_initialize(&mesgq);
  memoryRegister_initialize(&memreg, NPROCS, 1, 0);
  deliveryTable_initialize(&deliv, NPROCS, 1);
  
  for (i = 0; i < NPROCS; i++)
    memoryRegister_push(&memreg, i, (char *) &b);

  
  sendobj.size = 5;
  sendobj.info.send.payload_size = 5;

  pushobj.size = 0;
  pushobj.info.push.address = (char *) &a;
  
  popobj.size = 0;
  popobj.info.pop.address = (char *) &b;
  
  settagobj.size =0;
  settagobj.info.settag.tag_size = 1;
  
  putobj.size = sizeof(int);
  putobj.info.put.dst = (char *) &a;
  k = 0;
  t = deliveryTable_push(&deliv, 0, &putobj, it_put);
  *t = b;
  assert(deliv.rows >= deliv.used_slot_count[0]);
  assert(deliv.info.deliv.start[0][it_put] == index_size );
  assert(deliv.info.deliv.count[0][it_put] == 1);
  assert(deliv.info.deliv.end[0][it_put] == index_size);
  
  k+=index_size + no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE)) + 1;
  deliveryTable_push(&deliv, 0, &pushobj, it_pushreg);
  assert(deliv.rows >= deliv.used_slot_count[0]);
  assert(deliv.info.deliv.start[0][it_pushreg] == k);
  assert(deliv.info.deliv.count[0][it_pushreg] == 1);
  assert(deliv.info.deliv.end[0][it_pushreg] == k);

  k += no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE));
  t = deliveryTable_push(&deliv, 0, &sendobj, it_send);
  memcpy(t, "Hoi!", 5);
  assert(deliv.rows >= deliv.used_slot_count[0]);
  assert(deliv.info.deliv.start[0][it_send] == k);
  assert(deliv.info.deliv.count[0][it_send] == 1);
  assert(deliv.info.deliv.end[0][it_send] == k);
  
  k+= no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE)) +
      no_slots(sendobj.size, sizeof(ALIGNED_TYPE));

  for (i = 1; i< NPROCS; i++) 
    {
      deliveryTable_push(&deliv, i, &pushobj, it_pushreg);
      assert(deliv.rows >= deliv.used_slot_count[i]);
      assert(deliv.info.deliv.start[i][it_pushreg] == index_size);
      assert(deliv.info.deliv.count[i][it_pushreg] == 1);
      assert(deliv.info.deliv.end[i][it_pushreg] == index_size);
    }  

  deliveryTable_push(&deliv, 0, &popobj, it_popreg);
  assert(deliv.rows >= deliv.used_slot_count[0]);
  assert(deliv.info.deliv.start[0][it_popreg] == k);
  assert(deliv.info.deliv.count[0][it_popreg] == 1);
  assert(deliv.info.deliv.end[0][it_popreg] == k);

  k += no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE));
  deliveryTable_push(&deliv, 0, &settagobj, it_settag);
  assert(deliv.rows >= deliv.used_slot_count[0]);
  assert(deliv.info.deliv.start[0][it_settag] == k);
  assert(deliv.info.deliv.count[0][it_settag] == 1);
  assert(deliv.info.deliv.end[0][it_settag] == k);

  deliveryTable_execute(&deliv, &memreg, &mesgq, 0);

  assert(a == 10);
  assert(b == 10);
  assert(memoryRegister_find(&memreg, 0,1, (char *) &a) == (char *) &a);

  memoryRegister_destruct(&memreg);
  deliveryTable_destruct(&deliv);
  return 0;
}  
