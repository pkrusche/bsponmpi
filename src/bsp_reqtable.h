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
#ifndef BSP_REQTABLE_H
#define BSP_REQTABLE_H

/** @file bsp_reqtable.h
defines prototypes and implementations of methods on RequestTable.
RequestTable is a communication buffer for handling data requests
(the request part of a bsp_get() ).
@author Wijnand Suijlen
*/  

#include "bsp_config.h"
#include "bsp_exptable.h"
#include <stdlib.h>

void requestTable_execute (const ExpandableTable *RESTRICT , ExpandableTable * RESTRICT);

/** Initializes a RequestTable object 
@param table Reference to a RequestTable
@param nprocs Number of processors
@param rows Number of rows
*/
static inline void
	requestTable_initialize (ExpandableTable * RESTRICT table, const unsigned int nprocs,
	const unsigned int rows)
{
	union SpecInfo info;
	info.req.data_sizes = (unsigned int * ) bsp_calloc( nprocs, sizeof(unsigned int));
	fixedElSizeTable_initialize (table, nprocs, rows, sizeof (ReqElement), info);
}

/** Destructor of RequestTable 
@param table Reference to a RequestTable */
static inline void
	requestTable_destruct (ExpandableTable *RESTRICT  table)
{
	bsp_free(table->info.req.data_sizes);
	expandableTable_destruct (table);
}

static inline void
	requestTable_reset(ExpandableTable *RESTRICT table)
{
	memset(table->info.req.data_sizes, 0, table->nprocs * sizeof(unsigned int));
	expandableTable_reset(table);
}  

static inline void
	requestTable_resetrowcount(ExpandableTable *RESTRICT table, const int rows)
{
	memset(table->info.req.data_sizes, 0, table->nprocs * sizeof(unsigned int));
	expandableTable_resetrowcount(table, rows);
}  


/** Add additional rows to RequestTable
@param table Reference to a RequestTable
@param rows Number of rows to add
*/
static inline void
	requestTable_expand (ExpandableTable *RESTRICT  table, const unsigned int rows)
{
	union SpecInfo info = table->info;
	expandableTable_expand (table, rows, &info);
}


static void
	newReqInfoAtPush (union SpecInfo *RESTRICT info, unsigned int rows, unsigned int bla)
{
}

/** Adds a data request element to the table
@param table Reference to RequestTable
@param proc Processor rank whereto the request is send
@param element Description of data request
*/
static inline void
	requestTable_push (ExpandableTable * RESTRICT table, const unsigned int proc, const ReqElement * element)
{
	table->info.req.data_sizes[proc] += element->size + sizeof(DelivElement);
	fixedElSizeTable_push (table, proc, &newReqInfoAtPush, element);
}

#endif
