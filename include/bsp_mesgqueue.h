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

/** @file bsp_mesgqueue.h
    Defines methods on MessageQueue. MessageQueue is queue for holding BSMP
    messages. 
    @author Wijnand Suijlen
  */  
#ifndef BSP_MESGQUEUE_H
#define BSP_MESGQUEUE_H
#include "bsp_config.h"
#include "bsp_exptable.h"

/** MessageQueue stores where to find messages in a received DeliveryTable */
typedef struct {
  /** Tag size of the received messages */
  unsigned int recv_tag_size;
  /** Tag size of the messages to be sent */
  unsigned int send_tag_size;
  /** Number of received messages in queue */
  unsigned int n_mesg;
  /** Sum of the payload sizes of the received messages in queue */
  unsigned int accum_size;
  /** pointer to the first received message in queue */
  ALIGNED_TYPE * RESTRICT head;
} MessageQueue;  



/** Initializes a MessageQueue. 
*/  
static inline void
messageQueue_initialize (MessageQueue * RESTRICT mesgq)
{
  mesgq->recv_tag_size = 0;
  mesgq->send_tag_size = 0;
  mesgq->n_mesg = 0;
  mesgq->accum_size = 0;
  mesgq->head = NULL;
}

/** Clears the queue and sets the new tag size */
static inline void
messageQueue_sync(MessageQueue * RESTRICT mesgq)
{
  mesgq->recv_tag_size = mesgq->send_tag_size;
  mesgq->n_mesg = 0;
  mesgq->accum_size = 0;
  mesgq->head = NULL ;
}

#endif
