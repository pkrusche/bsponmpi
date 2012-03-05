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

#ifndef BSP_EXPTABLE_H
#define BSP_EXPTABLE_H

/** @file bsp_exptable.h 
Defines struct's, prototypes and inlined functions of the ExpandableTable
and FixedElSizeTable 'classes' and some struct's of
DeliveryTable, RequestTable and MemoryRegister.    
@see \ref Implementation in the \ref Architecture description
@author Wijnand Suijlen
*/  

#define ALIGNED_TYPE double

/** gives the maximum of x and y */
#define MAX(x,y)   ( ((x) > (y))?(x):(y))

/** gives the minimum of x and y */
#define MIN(x,y)   ( ((x) < (y))?(x):(y))

#include <stdlib.h>
#include <memory.h>
#include "bsp_config.h"

#include "bsp_alloc.h"

#include "bspx_comm.h"

/** calculate how many slots are necessary to contain a number of bytes
@param bytes Number of bytes
@param slot_size Size of the slots in bytes
*/
inline static int no_slots(const int bytes, const int slot_size)
{
	return (bytes + slot_size - 1) / slot_size;
}  

/** calculates the maximum value of 
array[0], array[stride], array[2*stride], ..., array[n]. 
@param array Reference to array
@param n     Size of array
@param stride Stride. 
@return maximum value 
*/
inline static unsigned int 
	array_max(const unsigned int * _array, int n, int stride)
{
	int i;
	unsigned int result = 0;
	for (i = 0; i < n; i+=stride)
		result = MAX(_array[i], result);
	return result  ;
}  


/** @name MemoryRegister data structures*/
/*@{*/
/** Data element stored in a MemoryRegister. It is a pointer to a
* registered memory area */
typedef char * MemRegElement;

/** Additional data needed by a MemoryRegister object */
typedef struct
{
	int numremov;             /**< number of pointers popped */
	int * RESTRICT removed;   /**< boolean array of popped pointers */
	int memoized_src_proc;   /**< Rank of this processor */
	const MemRegElement * memoized_data_iter;
	const MemRegElement * memoized_end;
	int memoized_srccol;
} MemRegInfo;
/*@}*/

/** Additional data needed by a RequestTable */
typedef struct
{
	/** expected amount of data to be returned: The sum of \c size in every 
	* column. */
	unsigned int * RESTRICT data_sizes;
} ReqInfo;

/** @name RequestTable data structures */
/*@{*/
/** Data element stored in a RequestTable object. A RequestTable stores only
* 'bsp_get()' operations */
typedef struct
{
	/** size of requested data */
	int size;
	/** offset to registered memory location of requested data */
	int offset; 
	/** remote pointer to source */
	char *src;
	/** local pointer to destination*/
	char *dst;  
} ReqElement;
/*@}*/

/** @name VarElSizeTable data structures */
/*@{*/
/** type of action */
typedef enum _ItemType
{ it_popreg, it_pushreg, it_put, it_get, it_send, it_settag } ItemType;

/** additional info for a bsp_put() */
typedef struct
{
	/** remote pointer to destination */
	char * dst;
} PutObject;

/** additional info for a bsp_send() */
typedef struct
{
	unsigned int payload_size; /**< Size of the payload */
} SendObject;

/** Additional info for a bsp_push_reg() */
typedef struct
{
	/** memory location to be registered */
	const char *address;
} PushRegObject;

/** Additional info for a bsp_pop_Reg() */
typedef struct
{
	/** address to be removed from the register */
	const char *address;
} PopRegObject;

/** Additional info for a bsp_set_tagsize() */
typedef struct
{
	/** new tag size */
	int tag_size;
} SetTagObject;

/** Data structure for additional info in a VarSizeElement */
union _DInfo 
{
	PutObject put;
	SendObject send;
	PushRegObject push;
	PopRegObject pop;
	SetTagObject settag;
} ;

/** Data element stored in a DeliveryTable */
typedef struct
{
	unsigned int size;  /**< Size of the variable-length data stored after this structure */
	unsigned int next; /**< Pointer to next element of same type */
	union _DInfo info; /**< Specific info of an element */
} DelivElement;
/*@}*/

/** Additional data for a DeliveryTable */
typedef struct
{
	/** where is first object? e.g. start[0][put] gives offset of first put
	* object (in slots).*/ 
	unsigned int * * RESTRICT start;

	/** how many of which type? e.g. count[0][put] gives number of puts in
	* processor column 0 */
	unsigned int * * RESTRICT count; 

	/** where is the last pushed element of some type? e.g. end[0][put] gives
	* the offset of the last pushed 'put' in processor column 0 */
	unsigned int * * RESTRICT end;
} DelivInfo;

/** Datastructure storing specific info */
union SpecInfo
{
	MemRegInfo reg;
	DelivInfo deliv;
	ReqInfo   req;
};

/** @name ExpandableTable */
/*@{*/

/** a table with \a nprocs number columns and \a rows number of rows, where
* each row has a height of \a slot_size bytes. This table can be communicated
* to the other processors by means of expandableTable_comm(): each columns is
* send to processors with rank equal to the column number */
typedef struct _ExpandableTable
{
	unsigned int nprocs;	/**< number of processors, or columns */
	unsigned int rows;    /**< number of elements (VarSizeElement, MemRegElement,
						  ReqElement) allocated per processor */
	/** number of slots used in table per column */
	unsigned int * RESTRICT used_slot_count;
	/** size of the slots */
	unsigned int slot_size;

	/** Specific information dependent which class is derived from this on */
	union SpecInfo info;

	/** pointer to actual table */
	char *data;

	/** MPI_Alltoall offsets (this can't be done on the stack */
	int * RESTRICT offset;
	/** MPI_Alltoall counts (this can't be done on the stack */
	int * RESTRICT bytes;

} ExpandableTable;


void 
	expandableTable_comm (const ExpandableTable * RESTRICT send, 
	ExpandableTable * RESTRICT recv, BSPX_CommFn communicator );

/* intializes an ExpandableTable object
@param table Reference to an ExpandableTable object
@param nprocs Number of processors you have / columns you want to have
@param rows Number of rows to allocate 
@param elsize Slot size in bytes
@param info Object specific info
*/   
static inline void
	expandableTable_initialize (ExpandableTable * RESTRICT table, 
	const unsigned int nprocs, const unsigned int rows,
	const int unsigned elsize, const union SpecInfo info)
{
	table->nprocs = nprocs;
	table->rows = rows;
	table->slot_size = elsize;
	table->info = info;
	table->data = (char*)bsp_malloc (nprocs * rows, elsize);

	table->used_slot_count = (unsigned int * ) bsp_calloc (nprocs, sizeof (unsigned int));
	table->offset = (int * ) bsp_calloc (nprocs, sizeof (unsigned int));
	table->bytes = (int * ) bsp_calloc (nprocs, sizeof (unsigned int));
}

/** clears contents of the table 
@param table Reference to an ExpandableTable object 
*/
static inline void
	expandableTable_reset (ExpandableTable * RESTRICT table)
{
	memset (table->used_slot_count, 0, sizeof (unsigned int) * table->nprocs);
}

/** frees memory taken by an ExpandableTable object 
@param table Reference to an ExpandableTable object
*/
static inline void
	expandableTable_destruct (ExpandableTable * RESTRICT table)
{
	bsp_free (table->data);
	bsp_free (table->used_slot_count);
	bsp_free(table->bytes);
	bsp_free(table->offset);
}  

/** Add some additional rows to the table
@param table Reference to an ExpandableTable object
@param rows Number of rows to add
@param newinfo The object specific information may have to be changed. This
can be supplied via this parameter
*/
static inline void
	expandableTable_expand (ExpandableTable * RESTRICT table, const unsigned int rows,
	const union SpecInfo * RESTRICT newinfo)
{
	unsigned int newrows = rows + table->rows;
	char *newdata = (char*)
		bsp_malloc( newrows * table->nprocs, table->slot_size);
	unsigned int i;

	for (i = 0; i < table->nprocs; i++)
		memcpy( newdata + i * newrows * table->slot_size, 
		table->data + i * table->rows * table->slot_size,
		table->used_slot_count[i] * table->slot_size);

	bsp_free(table->data);
	table->data = newdata;
	table->rows = newrows;
	table->info = *newinfo;
}
/*@}*/


/** @name FixedElSizeTable member functions
A table which contains elements of fixed size. Descendants are
MemoryRegister */
/*@{*/
/** initializes an FixedElSizeTable
@param table Reference to an ExpandableTable
@param nprocs Number of processors
@param rows Number of rows
@param elsize Size of an element
@param info addation info
*/ 
static inline void
	fixedElSizeTable_initialize (ExpandableTable * RESTRICT table, const unsigned int nprocs, 
	const unsigned int rows, const unsigned int elsize,
	const union SpecInfo info)
{
	expandableTable_initialize (table, nprocs, rows, elsize, info);
}

/** adds an element for a specific processor to the table and expands the
* table when necessary. 
@param table Reference to a FixedSizeElement
@param proc Destination processor
@param changeinfo Reference to the function which changes the table info in
case of table expansion
@param element pointer to data element
*/

static inline void 
	fixedElSizeTable_push (ExpandableTable * RESTRICT table, const unsigned int proc,
	void (*changeinfo) (union SpecInfo *RESTRICT,  unsigned int, unsigned int), 
	const void *RESTRICT element)
{
	int j;

	/* add table if necessary */
	if (table->used_slot_count[proc] == table->rows)
	{
		union SpecInfo info = table->info;
		(*changeinfo) (&info, table->rows, 2*table->rows );
		expandableTable_expand (table, table->rows , &info);
	}

	/* add pointer */
	j = table->used_slot_count[proc] + proc * table->rows;
	memcpy (table->data + j * table->slot_size, 
		element, table->slot_size);

	table->used_slot_count[proc] ++;
}
/*@}*/

#endif
