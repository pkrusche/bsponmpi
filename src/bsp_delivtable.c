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

/** @file bsp_delivtable.c
Implements the methods on a DeliveryTable object
@author Wijnand Suijlen
*/

#include "bsp_exptable.h"
#include "bsp_memreg.h"
#include "bsp_mesgqueue.h"

/** Executes a DeliveryTable object, i.e.: performs all the actions to be
* taken when a DeliveryTable is received 
@param table Reference to a DeliveryTable
@param memreg Reference to a MemoryRegister
@param mesgq Reference to a MessageQueue
@param rank Rank of this processor
*/

void
	deliveryTable_execute (ExpandableTable * RESTRICT table, 
	ExpandableTable * RESTRICT memreg, 
	MessageQueue * RESTRICT mesgq, const int rank)
{
	unsigned int p, i;
	const DelivElement *RESTRICT element;
	DelivElement * RESTRICT message = NULL;
	const ALIGNED_TYPE * RESTRICT pointer;
	const unsigned int tag_size = 
		no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE));

	for (p = 0; p < table->nprocs; p++)
	{
		/* do put's */
		pointer = (ALIGNED_TYPE *) table->data + p * table->rows + 
			table->info.deliv.start[p][it_put] ;

		for (i = 0; i < table->info.deliv.count[p][it_put]; i++)
		{
			element = (DelivElement *) pointer;
			memcpy(element->info.put.dst, pointer + tag_size, element->size);
			pointer+=element->next;
		}  

		/* do pushreg's */	
		pointer = (ALIGNED_TYPE *) table->data + p * table->rows + 
			table->info.deliv.start[p][it_pushreg];

		for (i = 0; i < table->info.deliv.count[p][it_pushreg]; i++)
		{
			element = (DelivElement *) pointer;
			memoryRegister_push( memreg, p, element->info.push.address);
			pointer+=element->next;
		}  

		/* gather data about send's */
		pointer = (ALIGNED_TYPE *) table->data + p * table->rows + 
			table->info.deliv.start[p][it_send];
		/* link the last element of the previous column to the first of this
		* column */
		if (message != NULL) 
			message->next = ((unsigned long) pointer - (unsigned long) message) / sizeof(ALIGNED_TYPE);

		for (i = 0; i < table->info.deliv.count[p][it_send]; i++)
		{
			message = (DelivElement *) pointer;
			mesgq->accum_size += message->size;
			mesgq->n_mesg ++;
			pointer+=message->next;
		}  

	}   
	/* get's aren't supposed to be in this queue */

	/* do popreg's (they only appear in processor column 'rank') */
	pointer = (ALIGNED_TYPE *) table->data + rank * table->rows +
		table->info.deliv.start[rank][it_popreg];
	for (i = 0; i < table->info.deliv.count[rank][it_popreg]; i++)
	{
		element = (DelivElement *) pointer;
		memoryRegister_pop( memreg, rank, element->info.pop.address);
		pointer+=element->next;
	}

	/* do settag's (they only appear in processor column 'rank') */
	pointer = (ALIGNED_TYPE *) table->data + rank * table->rows +
		table->info.deliv.start[rank][it_settag];
	for (i = 0; i < table->info.deliv.count[rank][it_settag]; i++)
	{
		element = (DelivElement *) pointer;
		mesgq->send_tag_size = element->info.settag.tag_size;
		pointer+=element->next;
	}

	/* look for the first message in the message queue */
	for (p = 0; p < table->nprocs; p++)
	{
		if ( table->info.deliv.count[p][it_send] > 0 )
		{
			mesgq->head = (ALIGNED_TYPE *) table->data + p * table->rows 
				+ table->info.deliv.start[p][it_send];
			break;
		}
	}	  
}



