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

#ifndef BSP_DELIVTABLE_H
#define BSP_DELIVTABLE_H

/** @file bsp_delivtable.h
Defines the prototypes of the methods on a DeliveryTable object.

DeliveryTable provides two functionalities: 
-# It is a queue for actions which have to be performed during the next
bsp_sync(), e.g. a bsp_set_tagsize() or a bsp_pop_reg()
-# It is a communication buffer for data which has to be sent during the
next bsp_sync(), e.g. a bsp_put() 

These are the same if you consider sends and puts to be actions as well: A
put or send is a memory copy action on the remote processor.  Actions like
bsp_set_tagsize() which do not need communication are placed in the column
of the local processor (the column number is equal to the rank of the
local processor).

Each action consists of a tag and a payload. The tag is a \ref
DelivElement, the payload data can be anything as long it is not bigger
as described in the tag. The tag stores information about the \c size,
action specific information and where to find the \c next action of the same
type. The tag does not store what type of action it represents, because
DeliveryTable knows where to find the first action of a certain type
(\ref DelivInfo ) .
Subsequent actions are referenced by the \c next value in the tag. In
other words: Each column stores 6 linked lists of actions (pushreg,
popreg, put, get, send, settag). Information about these linked lists (the
arrays referenced in \ref DelivInfo) are stored at the top of the columns.
This way, the information is automatically communicated to the receiving
processors.

You may wonder why these linked lists are necessary. It also works without
these lists if you store the type of action in the tag. However, when
executing a column, it has to be determined what type each action is. This
results in a switch-case statement in a tight loop. Branches are expensive
and therefore they have been made unnecessary by these linked lists.

@author Wijnand Suijlen
*/  

#include "bsp_config.h"
#include "bsp_exptable.h"
#include "bsp_mesgqueue.h"

void
	deliveryTable_execute (ExpandableTable *RESTRICT , ExpandableTable *RESTRICT ,
	MessageQueue *RESTRICT, const int );

/** initializes a DeliveryTable object 
@param table Reference to a DeliveryTable
@param nprocs Number of processors to allocate memory for  
@param rows Number of rows to allocate 
*/

static inline void
	deliveryTable_initialize (ExpandableTable * RESTRICT table, const int nprocs, const int rows)
{
	union SpecInfo info;
	int p;
	const int index_size = 
		no_slots(3 * 6 * sizeof(unsigned int), sizeof(ALIGNED_TYPE));

	/* allocate memory */
	info.deliv.count = (unsigned int * * ) bsp_malloc( nprocs, sizeof(unsigned int *));
	info.deliv.start = (unsigned int * * ) bsp_malloc( nprocs, sizeof(unsigned int *));
	info.deliv.end = (unsigned int * * ) bsp_malloc(nprocs, sizeof(unsigned int *));
	expandableTable_initialize (table, nprocs, rows + index_size , sizeof(ALIGNED_TYPE), info);

	/* point the pointers to the correct places: the top each column */
	for (p = 0; p < nprocs; p++) 
	{
		table->info.deliv.start[p] = (unsigned int *)
			((char *) table->data + p * table->rows * sizeof(ALIGNED_TYPE)) ;
		table->info.deliv.count[p] = (unsigned int *) 
			((char *) table->data + p * table->rows * sizeof(ALIGNED_TYPE)) + 6 ;
		table->info.deliv.end[p] = (unsigned int *) 
			((char *) table->data + p * table->rows * sizeof(ALIGNED_TYPE)) + 12 ;

		memset((ALIGNED_TYPE *) table->data + p * table->rows , 0, sizeof(ALIGNED_TYPE) * index_size );
		/* don't overwrite this information */
		table->used_slot_count[p] = index_size;	 
	}  
}

/** clears a DeliveryTable object 
@param table Reference to a DeliveryTable
*/
static inline void
	deliveryTable_reset(ExpandableTable * RESTRICT table)
{
	/* reset the index */
	unsigned int p;
	const int index_size = 
		no_slots(3 * 6 * sizeof(unsigned int), sizeof(ALIGNED_TYPE));

	expandableTable_reset(table);
	for (p = 0; p < table->nprocs; p++)
	{
		memset((ALIGNED_TYPE *) table->data + p * table->rows , 0, sizeof(ALIGNED_TYPE) * index_size );
		table->used_slot_count[p] = index_size;
	}  

}

/** return true (1) if a given delivery table is empty
 @param table Reference to a DeliveryTable
 */
static inline int deliveryTable_empty(ExpandableTable * RESTRICT table) {
	const unsigned int index_size = 
		no_slots(3 * 6 * sizeof(unsigned int), sizeof(ALIGNED_TYPE));
	unsigned int p;
	for (p = 0; p < table->nprocs; p++)
	{
		if ( table->used_slot_count[p] > index_size ) {
			return 0;
		}
	}
	return 1;
}

/** Frees memory allocated by a DeliveryTable 
@param table Reference to a DeliveryTable */
static inline void
	deliveryTable_destruct (ExpandableTable * RESTRICT table)
{
	bsp_free(table->info.deliv.start);
	bsp_free(table->info.deliv.count);
	bsp_free(table->info.deliv.end);
	expandableTable_destruct (table);
}

/** Expands a DeliveryTable.  
@param table Reference to a DeliveryTable object
@param rows Number of rows which should be added to this table
*/

static inline void
	deliveryTable_expand (ExpandableTable * RESTRICT table, const int rows)
{
	unsigned int p;

	union SpecInfo info = table->info;
	expandableTable_expand (table, rows, &info);

	for (p = 0; p < table->nprocs; p++) 
	{
		table->info.deliv.start[p] = (unsigned int *)
			((char *) table->data + p * table->rows * sizeof(ALIGNED_TYPE)) ;
		table->info.deliv.count[p] = (unsigned int *) 
			((char *) table->data + p * table->rows * sizeof(ALIGNED_TYPE)) + 6 ;
		table->info.deliv.end[p] = (unsigned int *) 
			((char *) table->data + p * table->rows * sizeof(ALIGNED_TYPE)) + 12 ;
	}  
}


/** Adds an element to the table and expands the table when necessary. The
* payload may be copied to the address referenced by the returned pointer. 
@param table Reference to a VarElSizeTable
@param proc Destination processor
@param element Element to be added
@param type Type of element
@return pointer to memory location in which data can be copied
*/
static inline void *
	deliveryTable_push (ExpandableTable *RESTRICT  table, const int proc,
	const DelivElement *RESTRICT element, const ItemType type)
{
	const unsigned int slot_size = sizeof(ALIGNED_TYPE);
	const unsigned int tag_size = no_slots(sizeof(DelivElement), slot_size);
	const unsigned int object_size = tag_size + no_slots(element->size, slot_size);
	ALIGNED_TYPE * RESTRICT pointer;

	int free_space = table->rows - table->used_slot_count[proc];

	if ((signed)object_size > free_space) 
	{
		int space_needed = MAX(table->rows, object_size - free_space);
		deliveryTable_expand(table, space_needed);
	}  

	/* manage deliveryTable info */
	if (table->info.deliv.count[proc][type] == 0)
		table->info.deliv.start[proc][type] = table->used_slot_count[proc];
	else
	{
		pointer = (ALIGNED_TYPE *) table->data + table->info.deliv.end[proc][type] + proc * table->rows;
		((DelivElement *) pointer)->next = table->used_slot_count[proc] - table->info.deliv.end[proc][type];
	}

	table->info.deliv.end[proc][type] = table->used_slot_count[proc];
	table->info.deliv.count[proc][type]++;

	pointer = (ALIGNED_TYPE *) table->data + table->used_slot_count[proc] + proc * table->rows;
	* (DelivElement *) pointer = *element;
	pointer+=tag_size;
	/* increment counters */
	table->used_slot_count[proc] += object_size;

	return pointer ;  
}



#endif
