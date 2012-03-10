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

/** @file bsp_abort.h
 *  Defines the prototype of an `abort' function for internal usage:
 *  bsp_intern_abort() 
 *  @author Wijnand Suijlen
 */
#ifndef BSP_ABORT_H
#define BSP_ABORT_H


/** @name ErrorNumbers
 * Error number defintions and their translation
 */
/*@{*/
/** When a 'bsp_get' gets delivered. <= this is impossible */
#define ERR_GET_DELIVERED   5
/** used in MemoryRegister when the stack counter becomes to big */
#define ERR_STACK_COUNTER_OVERFLOW 4
/** used in MemoryRegister when an item could not be found */
#define ERR_POP_REG_WITHOUT_PUSH 3
/** when memory could no be allocated */
#define ERR_NOT_ENOUGH_MEMORY	2
/** when the user calls bsp_abort() */
#define ERR_BSP_ABORT 		1

/** Translation of error messages */
#define ERR_MESSAGES \
        { "abnormal program termination\n"\
	, "Not enough memory\n"\
	, "bsp_pop_reg without bsp_push_reg\n"\
	, "stack counter overflow"\
	, "a bsp_get() is delivered! contact the library maintainer"\
	}
/*@}*/

/** Is used by other library functions when a task could
 * not be successfully completed. 
 * @param err_number one of the Error numbers in ErrorNumbers
 * @param func the function name which caused the bsp_intern_abort 
 * @param file the file name  
 * @param line the line number 
 */
#ifdef __cplusplus
extern "C"
#endif
void 
bsp_intern_abort (const int err_number, const char *func, 
                  const char *file, int line);


#endif
