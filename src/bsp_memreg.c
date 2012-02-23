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

/** @file bsp_memreg.c
    Implements methods on MemoryRegister
    @author Wijnand Suijlen */

#include "bsp_abort.h"
#include "bsp_memreg.h"
#include "bsp_alloc.h"

/** Initializes a MemoryRegister object 
    @param table Reference to a MemoryRegister object
    @param nprocs Number of processors
    @param rows Initial number of rows
    @param src_proc Rank of the local processor
  */  
void
memoryRegister_initialize (ExpandableTable * RESTRICT table, const unsigned int nprocs, 
                           const unsigned int rows, const unsigned int src_proc)
{
  union SpecInfo info;
  info.reg.removed = bsp_calloc (rows, sizeof (int));
  info.reg.numremov = 0;
  info.reg.memoized_data_iter = NULL;
  info.reg.memoized_end = NULL;
  info.reg.memoized_srccol=0;
  info.reg.memoized_src_proc = src_proc;
  fixedElSizeTable_initialize (table, nprocs, rows, sizeof (MemRegElement), info);
}

/** destructor of MemoryRegister
@param table Reference to a MemoryRegister object 
*/
void
memoryRegister_destruct (ExpandableTable * RESTRICT table)
{
  bsp_free(table->info.reg.removed);
  expandableTable_destruct (table);
}

/** increases the size of a MemoryRegister
 @param table Reference to a MemoryRegister object
 @param rows Number of rows to add
 */
void
memoryRegister_expand (ExpandableTable * RESTRICT table, const unsigned int rows)
{
  union SpecInfo info;
  int *newremoved = bsp_calloc (rows + table->rows, sizeof (int));
  unsigned int i;
  for (i = 0 ; i < table->rows; i++) /* copy old values */
    newremoved[i] = table->info.reg.removed[i];
  bsp_free(newremoved);
  info.reg.removed = newremoved;
  info.reg.numremov = table->info.reg.numremov;
  info.reg.memoized_data_iter = NULL; /* should be calculated */
  info.reg.memoized_end=NULL; /* should be calculated */
  info.reg.memoized_srccol =
    table->info.reg.memoized_src_proc * (rows + table->rows);
  info.reg.memoized_src_proc = table->info.reg.memoized_src_proc;

  expandableTable_expand (table, rows, &info);
}

/** Used as parameter of memoryRegister_push(). 
  @param info MemoryRegister info
  @param rows Old number of rows
  @param newrows New number of rows
  */
static void
newMemRegInfoAtPush (union SpecInfo * RESTRICT info, unsigned int rows, unsigned int newrows)
{
  int *newremoved = bsp_calloc(newrows, sizeof(int));
  unsigned int i;
  for (i = 0; i < rows; i ++)
    newremoved[i] = info->reg.removed[i];
  bsp_free(info->reg.removed);
  info->reg.removed = newremoved;
  
  info->reg.memoized_data_iter = NULL;
  info->reg.memoized_end = NULL;
  info->reg.memoized_srccol=0;
  /*  info->reg.numremov = stays the same ; */
}

/** Adds an address of memory location of in a certain processor
  @param table Reference to a MemoryRegister object
  @param proc Processor rank  of the memory location
  @param pointer Address
 */ 
void 
memoryRegister_push (ExpandableTable *RESTRICT table, const unsigned int proc, 
                     const char * const RESTRICT pointer)
{
  table->info.reg.memoized_data_iter = NULL;

  fixedElSizeTable_push (table, proc, &newMemRegInfoAtPush, &pointer);
}

/** Removed an adress from a MemoryRegister
  @param table Reference to a MemoryRegister
  @param proc Rank of local processor
  @param pointer Local registered address
  */
void
memoryRegister_pop (ExpandableTable * RESTRICT table, const unsigned int proc,
                     const char * const RESTRICT pointer)
{
  int count, col;
  const MemRegElement * RESTRICT array;
  
  table->info.reg.memoized_data_iter = NULL;
  
  col = table->rows * proc;
  array = (MemRegElement *) table->data + col;
  for (count = table->used_slot_count[proc]-1; count >= 0; count--)
    {
       if (array[count] == pointer && !table->info.reg.removed[count])
         {
           table->info.reg.removed[count] = 1;
           table->info.reg.numremov++;
           return;
         }
    }
  bsp_intern_abort (ERR_POP_REG_WITHOUT_PUSH, __func__, __FILE__,__LINE__);
  return;
}

/** Really removes popped elements from the table. Popped elements are marked
 * for removal, but are not actually removed. 
  @param table Reference to a MemoryRegister 
  */
void
memoryRegister_pack (ExpandableTable * RESTRICT table)
{
  int i, count = 0, j;
  unsigned int displ;
  MemRegElement * RESTRICT array;

  /* count */
  count = table->used_slot_count[0] - table->info.reg.numremov; 
  
  /* allocate memory and copy data */
  for (i = 0; i < (signed)table->nprocs; i++)	/* set newcount & newbytecount */
    {
      displ = 0;
      array = (MemRegElement *) table->data + i * table->rows;
      for (j = 0; j < (signed)table->used_slot_count[i]; j++)
        {
	  if ( table->info.reg.removed[j] )
	    {
	      displ++;
	      continue;
	    }  
          array[j - displ] = array[j];
	}
      table->used_slot_count[i] = count ;
    }
   
  memset(table->info.reg.removed, 0, sizeof(int) * table->rows);
  table->info.reg.numremov = 0;
 
  /* set memoized variables */
  /*  table->info.reg.memoized_src_proc stays the same */
  table->info.reg.memoized_srccol =
    table->rows * table->info.reg.memoized_src_proc;
  table->info.reg.memoized_end = 
    ((MemRegElement *) table->data) + table->info.reg.memoized_srccol;
  table->info.reg.memoized_data_iter =
    table->info.reg.memoized_end 
      + table->used_slot_count[table->info.reg.memoized_src_proc] -1;
}


