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

/** @file bsp_reqtable.c
    Implement the 'execute' method on a RequestTable
    @author Wijnand Suijlen
  */

#include "bsp_reqtable.h"
#include "bsp_alloc.h"
#include "bsp_delivtable.h"


/** Executes the data requests. Reads all data requests and translates them in
 * data delivery: bsp_put() 
 @param table Reference to RequestTable
 @param deliv Reference to DeliveryTable
 */
void
requestTable_execute (const ExpandableTable * RESTRICT table, ExpandableTable * RESTRICT deliv)
{
	ReqElement * RESTRICT element;
	DelivElement delivery;
	char * RESTRICT pointer;
	unsigned int i, j;

	for (i = 0; i < table->nprocs; i++)
	{
		element = (ReqElement *) table->data + i * table->rows;
		for (j = 0; j < table->used_slot_count[i]; j++) 
		{
			delivery.size = element[j].size;
			delivery.info.put.dst = element[j].dst;
			pointer = deliveryTable_push(deliv, i, &delivery, it_put);
			memcpy(pointer, element[j].src + element[j].offset, delivery.size);
		}  
	}
}
