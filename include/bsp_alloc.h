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

/** @file bsp_alloc.h
    provides wrapper functions for malloc(), calloc() and free(). 
    @author Wijnand Suijlen
*/
#ifndef BSP_ALLOC_H
#define BSP_ALLOC_H

#include <stdlib.h>
#include "bsp_config.h"
#include "bsp_abort.h"

/** wrapper macro for bsp_mallocx() */
#define bsp_malloc( n, sz) bsp_mallocx((n), (sz), \
                          __func__, __FILE__, __LINE__ )

/** wrapper macro bsp_callocx() */
#define bsp_calloc( n, sz) bsp_callocx((n), (sz), \
                          __func__, __FILE__, __LINE__ )

/** allocates memory.  
    @note Use the macro's bsp_malloc() and bsp_calloc() instead
    @param n number of elements to be allocated
    @param sz size of one element
    @param func Function from where this function is called
    @param file File from where this function is calles
    @param line line number from where this function is called
    @return a pointer to a memory location big enough to contain the data or
            NULL when a region of size 0 is requested
  */	    
static inline void *
bsp_mallocx (const size_t n, const size_t sz, 
            const char *func, const char *file, int line)
{
  void *result;

  if (n * sz == 0)
    return NULL;
  
  result = (void *) malloc (n * sz);
  if (result == NULL)
    bsp_intern_abort (ERR_NOT_ENOUGH_MEMORY, func, file, line);
  
  return result;
}

/** allocates cleared (zeroed) memory.
  * @param n number of elements to be allocated
    @param sz size of one element
    @param func Function from where this function is called
    @param file File from where this function is calles
    @param line line number from where this function is called
    @return a pointer to a memory location big enough to contain the data or
            NULL when a region of size 0 is requested
  */	    
static inline void *
bsp_callocx (const size_t n, const size_t sz, 
             const char *func, const char *file, int line)
{
  void *result;

  if (n * sz == 0)
    return NULL;
    
  result = (void *) calloc (n, sz);
  if (result == NULL)
    bsp_intern_abort (ERR_NOT_ENOUGH_MEMORY, func, file, line);
   
  return result;
}

/** frees memory. 
  @param ptr A pointer created by a bsp_malloc() or a bsp_calloc()
  */
static inline void
bsp_free (void *ptr)
{
  if (ptr != NULL)
    free (ptr);
}

#endif
