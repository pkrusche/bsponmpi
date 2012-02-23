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

/** @file bsp_private.h
 * contains a global variable which should not be available to the outside
 * world 
 * @author Wijnand Suijlen
 */

#ifndef BSP_PRIVATE_H
#define BSP_PRIVATE_H

#include "bsp_exptable.h"
#include "bsp_mesgqueue.h"

#define BSP_MAX_GLOBAL_ARRAYS 128

/** information to describe a BSP global array */
typedef struct _bsp_global_array_t {
    size_t array_size;
    void * local_slice;
    size_t local_size;
} bsp_global_array_t;

/** global variables used in bsp.c */
typedef struct _BSPObject
{
  double begintime;   /**< start time in bsp_begin() */
  int nprocs;	      /**< number of processors */
  int rank;	      /**< rank of this process */
  MPI_Comm 	communicator; /**< MPI Communicator of the BSP program */

  /** Message combining table for delivery of data, i.e.: bsp_put(),
   * bsp_send() and the delivery of bsp_get() */
  ExpandableTable delivery_table;
  /** Table in which received data is stored to be executed later */
  ExpandableTable delivery_received_table;
  /** Message combining table for just bsp_get() requests */
  ExpandableTable request_table;
  /** Table in which received requests are stored to be executed later */
  ExpandableTable request_received_table;
  /** Memory register. Tracks registered variables and memory locations to be
   * used in DRMA operations, i.e.: bsp_get() and bsp_put() */
  ExpandableTable memory_register;
  /** Mesage queue. Points to all received data initiated by a bsp_send() on
   * some processor. */
  MessageQueue message_queue;
  
  /** naive global array allocation */
  int global_array_last;
  /** global array allocation overflow indicator */
  int global_overflow;
  /** global array allocation */
  bsp_global_array_t global_arrays[BSP_MAX_GLOBAL_ARRAYS];

  /** send indices. these need to be stored here since they can't be
   *  put on the stack in a standard-conformant way */
  unsigned int * send_index;
  /** receive indices. these need to be stored here since they can't be
   *  put on the stack in a standard-conformant way */
  unsigned int * recv_index;
} BSPObject;


/** Packed global variables. To keep it private it is not included in bsp.h */
static BSPObject bsp;

#endif
