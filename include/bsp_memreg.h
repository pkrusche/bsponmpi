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

/** @file bsp_memreg.h 
    Defines prototypes of methods on MemoryRegister. MemoryRegister is a table
    containing memory locations of variables on all processors. Memory
    locations on the same row are bsp_push_reg()'ed on all processors at the
    same time (at the same position in the code)
    @author Wijnand Suijlen
  */  
#ifndef BSP_MEMREG_H
#define BSP_MEMREG_H
#include <stdlib.h>
#include "bsp_config.h"
#include "bsp_abort.h"
#include "bsp_exptable.h"

void
memoryRegister_initialize (ExpandableTable * RESTRICT , const unsigned int,
                           const unsigned int, const unsigned int);

void memoryRegister_destruct (ExpandableTable * RESTRICT );

void memoryRegister_expand (ExpandableTable * RESTRICT, const unsigned int);

void memoryRegister_push (ExpandableTable * RESTRICT, const unsigned int, const char * const RESTRICT );

void memoryRegister_pop (ExpandableTable * RESTRICT, const unsigned int, const char * const RESTRICT );

void memoryRegister_pack (ExpandableTable * RESTRICT);

/** looks up a the address on a remote processor  which corresponds to an
 * address on the local processor
 @param table Reference to a MemoryRegister
 @param sp Rank of the local processor
 @param dp Rank of the remote processort
 @param pointer Local address of a registered memory location
 @return Remote address
 */
static inline MemRegElement
memoryRegister_find (const ExpandableTable * RESTRICT table, 
                     const unsigned int sp, const unsigned int dp,
		     const char * const pointer)
{
  int count;
  const MemRegElement * RESTRICT array;
  const unsigned int srccol = table->rows * sp;
  const unsigned int dstcol = table->rows * dp;
  array  = (MemRegElement *) table->data + srccol;
  for ( count = table->used_slot_count[sp]-1; count >= 0; count--)
    {
      if (array[count] == pointer && !table->info.reg.removed[count])
        return *(array + count - srccol + dstcol);
    }
  bsp_intern_abort (ERR_POP_REG_WITHOUT_PUSH, __func__, __FILE__, __LINE__);
  return NULL;
}

/**  looks up the address on a remote processor which corresponds to an
 * address on the local processor. This function is memoized and depends on
 * the MemoryRegisters state of being 'packed'. If not, this function will not
 * function properly
 @param table Reference to a MemoryRegister
 @param dp Rank of the remote processor
 @param pointer Local address of a registered memory location
 @return Remote address
*/
static inline MemRegElement
memoryRegister_memoized_find (const ExpandableTable * RESTRICT table,  
                              const unsigned int dp, const char * const pointer)
{
  const unsigned int dstcol = table->rows * dp;
  const MemRegElement * RESTRICT data_iter = table->info.reg.memoized_data_iter;
 
  /* WARNING: If element is not in register, then loop may not terminate */
  while(*data_iter != pointer)
    data_iter--;

  return *(data_iter - table->info.reg.memoized_srccol + dstcol);
}

#endif
