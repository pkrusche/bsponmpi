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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "bspx.h"
#include "bsp_memreg.h"
#include "bsp_mesgqueue.h"
#include "bsp_delivtable.h"
#include "bsp_reqtable.h"
#include "bsp_private.h"
#include "bsp_alloc.h"
#include "bsp_abort.h"

/** Create buffers within a BSP object
 *
  @param bsp The BSPObject to use (user handles allocation). 
 */

inline void bspx_init_bspobject (BSPObject * bsp, int nprocs, int rank) {
	int d = 0;
	bsp->nprocs = nprocs;
	bsp->rank = rank;

	bsp->send_index = (unsigned int *)bsp_malloc(3 * bsp->nprocs, sizeof(unsigned int));
	bsp->recv_index = (unsigned int *)bsp_malloc(3 * bsp->nprocs, sizeof(unsigned int));

	/* initialize data structures */
	memoryRegister_initialize(&bsp->memory_register, bsp->nprocs, BSP_MEMREG_MIN_SIZE,
		bsp->rank);
	messageQueue_initialize (&bsp->message_queue);
	deliveryTable_initialize(&bsp->delivery_table, bsp->nprocs, BSP_DELIVTAB_MIN_SIZE);
	requestTable_initialize(&bsp->request_table, bsp->nprocs, BSP_REQTAB_MIN_SIZE);
	deliveryTable_initialize(&bsp->delivery_received_table, bsp->nprocs, 
		BSP_DELIVTAB_MIN_SIZE);
	requestTable_initialize(&bsp->request_received_table, bsp->nprocs,
		BSP_REQTAB_MIN_SIZE);

	bsp->global_array_last = 0;
	bsp->global_overflow = 0;

	/* save starting time */
	bsp->begintime = 0; // bsp->begintime is used in bsp_time(), so must be initialized
	bsp->begintime = bsp_time();
}

/** Ends the SPMD code. This function must be called after all SPMD code
    has been executed. The code after this call is executed by processor 0
    only. 
	* @param bsp The BSPObject to destroy. 
	*/    
inline void bspx_destroy_bspobject (BSPObject * bsp) {
	/* clean up datastructures */
	memoryRegister_destruct (&bsp->memory_register);
	deliveryTable_destruct(&bsp->delivery_table);
	requestTable_destruct(&bsp->request_table);
	deliveryTable_destruct(&bsp->delivery_received_table);
	requestTable_destruct(&bsp->request_received_table);

	bsp_free(bsp->recv_index);
	bsp_free(bsp->send_index);
}
/*@}*/

/** @name Superstep */
/*@{*/

/** Execute superstep data transfers. */ 
void bspx_sync (BSPObject * bsp, BSPX_CommFn0 infocomm, BSPX_CommFn communicator ) {
	unsigned int maxreqrows = 0, maxdelrows = 0, p;
	unsigned int any_gets = 0; 
	/* any_gets is a boolean value, whether there are
	   any gets to performed. If there are no gets,
	   then one MPI_Alltoall doesn't have to be
	   executed */
	/* reset message buffer */
	messageQueue_sync(&bsp->message_queue);
	requestTable_reset(&bsp->request_received_table);
	deliveryTable_reset(&bsp->delivery_received_table);

	/* communicate information */
	for (p = 0; p < (unsigned)bsp->nprocs; p++)
		any_gets |= bsp->request_table.used_slot_count[p];

	for (p = 0; p < (unsigned)bsp->nprocs; p++)
	{
		bsp->send_index[3*p    ] = bsp->request_table.used_slot_count[p];
		bsp->send_index[3*p + 1] = bsp->delivery_table.used_slot_count[p];
		bsp->send_index[3*p + 2] = any_gets;
	}  

	infocomm (	bsp->send_index, 3*sizeof(unsigned int), 
				bsp->recv_index, 3*sizeof(unsigned int)
	);

	/* expand buffers if necessary */
	maxreqrows = array_max(bsp->recv_index, 3*bsp->nprocs, 3);
	for (p = 0; p < (unsigned)bsp->nprocs; p++)
		maxdelrows = MAX( bsp->recv_index[1 + 3*p] + 
		bsp->request_table.info.req.data_sizes[p], maxdelrows);

	if ( bsp->request_received_table.rows < maxreqrows ) {
		maxreqrows = MAX(bsp->request_received_table.rows, maxreqrows);
		requestTable_expand(&bsp->request_received_table, maxreqrows);
	}  

	if (bsp->delivery_received_table.rows < maxdelrows )
	{
		maxdelrows = MAX(bsp->delivery_received_table.rows, maxdelrows);
		deliveryTable_expand(&bsp->delivery_received_table, maxdelrows );
	}  

	/* copy necessary indices to received_tables */
	for (p = 0; p < (unsigned)bsp->nprocs; p++) 
	{
		bsp->request_received_table.used_slot_count[p] = bsp->recv_index[3*p];
		bsp->delivery_received_table.used_slot_count[p] =
			bsp->recv_index[1 + 3*p] + bsp->request_table.info.req.data_sizes[p] ;
	}	

	/* Now we may conclude something about the communcation pattern */
	any_gets = 0;
	for (p = 0; p < (unsigned)bsp->nprocs; p++)   
		any_gets |= bsp->recv_index[3*p + 2];

	/* communicate & execute */
	if (any_gets) 
	{
		expandableTable_comm(&bsp->request_table, &bsp->request_received_table,
			communicator);
		requestTable_execute(&bsp->request_received_table, &bsp->delivery_table);
	}

	expandableTable_comm(&bsp->delivery_table, &bsp->delivery_received_table,
		communicator);
	deliveryTable_execute(&bsp->delivery_received_table, 
		&bsp->memory_register, &bsp->message_queue, bsp->rank);
	
	/* clear the buffers */			
	requestTable_reset(&bsp->request_table);
	deliveryTable_reset(&bsp->delivery_table);

	/* pack the memoryRegister */
	memoryRegister_pack(&bsp->memory_register);
}

/** Reset buffer sizes 
  As messages are buffered, the buffers will not be reset to their standard size
  unless this function is called. 

  If memory-usage is essential, this should be called after bsp_sync. Note that this
  function also deletes all undelivered bsmp messages.

  The only memory that is "lost" to the application is through memory register 
  registrations, we do not reset the size of bsp->memory_register, so the amount
  of memory used by this is equal to the maximum necessary amount over all supersteps.
  This should be negligible, since we only store a void pointer per each processor
  and bsp_pushreg operation (programs which do millions of pushreg's have bigger 
  problems than can be fixed by this function).

*/
void bspx_resetbuffers(BSPObject * bsp) {
	requestTable_resetrowcount(&bsp->request_table, BSP_REQTAB_MIN_SIZE);
	requestTable_resetrowcount(&bsp->request_received_table, BSP_REQTAB_MIN_SIZE);
	deliveryTable_resetrowcount(&bsp->delivery_table, BSP_DELIVTAB_MIN_SIZE);
	deliveryTable_resetrowcount(&bsp->delivery_received_table, BSP_DELIVTAB_MIN_SIZE);
	messageQueue_sync (&bsp->message_queue);
}

/*@}*/

/** @name DRMA */
/*@{*/
/** Makes the memory location with specified size available for DRMA
 * operations at the next and additional supersteps. 
 * @param bsp The BSPObject to use. 
 * @param ident pointer to memory location
 * @param size of memory block
 * @note In this version of BSPonMPI the parameter \a size is ignored
 * @see bsp_pop_reg()
 */
inline void bspx_push_reg (BSPObject * bsp, const void *ident, size_t size)
{
	int i;
	DelivElement element;
	element.size = 0;
	element.info.push.address = ident;
	for (i=0 ; i < bsp->nprocs; i++)
		deliveryTable_push(&bsp->delivery_table, i, &element, it_pushreg);
}

/** Deregisters the memory location 
* @param bsp The BSPObject to use. 
  @param ident pointer to memory location
  @see bsp_push_reg()
  */
inline void bspx_pop_reg (BSPObject * bsp, const void *ident)
{
	DelivElement element;
	element.size = 0;
	element.info.pop.address = ident;
	deliveryTable_push(&bsp->delivery_table, bsp->rank, &element, it_popreg);
}  

/** Puts a block of data in the memory of some other processor at the next
 * superstep. This function is buffered, i.e.: the contents of \a src
 * is copied to a buffer and transmitted at the next bsp_sync() 
 * @param bsp The BSPObject to use. 
 * @param pid rank of destination (remote) processor
 * @param src pointer to source location on source (local) processor
 * @param dst pointer to destination location on source processor. Translation
              of addresses is performed with help of earlier calls to bsp_push_reg()
   @param offset offset from \a dst in bytes (comes in handy when working with arrays)
   @param nbytes number of bytes to be copied
   @see bsp_push_reg()
*/
inline void bspx_put (BSPObject * bsp, int pid, const void *src, void *dst, long int offset, size_t nbytes)
{
	/* place put command in buffer */
	char * RESTRICT pointer;
	DelivElement element;
	element.size = (unsigned int) nbytes;
	element.info.put.dst = 
		memoryRegister_memoized_find(&bsp->memory_register, pid, dst) + offset;
	pointer = deliveryTable_push(&bsp->delivery_table, pid, &element, it_put);
	memcpy(pointer, src, nbytes);
}


/** Gets a block of data from the memory of some other processor at the next
 * superstep. This function is buffered, i.e.: The data is retrieved from the
 * destionation processor at the start of the next bsp_sync(). Translation of
 * the \a src pointer is performed with help of earlier calls to
 * bsp_push_reg()
 * @param bsp The BSPObject to use. 
 * @param pid Ranks of the source (remote) processor 
 * @param src Pointer to source location using a pointer to a local memory
 *            region 
 * @param offset offset from \a src in bytes
 * @param dst Pointer to destination location 
 * @param nbytes Number of bytes to be received
 * @see bsp_push_reg()
*/
inline void bspx_get (BSPObject * bsp, int pid, const void *src, long int offset, void *dst, size_t nbytes)
{
	ReqElement elem;
	elem.size = (unsigned int )nbytes;
	elem.src = 
		memoryRegister_memoized_find(&bsp->memory_register, pid, src);
	elem.dst = dst;
	elem.offset = offset;

	/* place get command in buffer */
	requestTable_push(&bsp->request_table, pid, &elem);
}
/*@}*/

/** @name BSMP */
/*@{*/

/** Sends message to a processor. You may supply a tag and and a payload. The
 * default size of the tag is 0. To change the tag size you can use
 * bsp_set_tagsize().
 @param bsp The BSPObject to use. 
 @param pid Rank of the destination processor
 @param tag pointer to the tag
 @param payload pointer to the payload
 @param payload_nbytes size of the payload
 */
inline void bspx_send (BSPObject * bsp, int pid, const void *tag, const void *payload, size_t payload_nbytes)
{
	DelivElement element;
	char * RESTRICT pointer;
	element.size = (unsigned int )payload_nbytes + bsp->message_queue.send_tag_size;
	element.info.send.payload_size = (unsigned int )payload_nbytes;
	pointer = deliveryTable_push(&bsp->delivery_table, pid, &element, it_send);
	memcpy( pointer, tag, bsp->message_queue.send_tag_size);
	memcpy( pointer + bsp->message_queue.send_tag_size, payload, payload_nbytes);
}

/** Gives the number of messages and the sum of the payload sizes in queue.
* @param bsp The BSPObject to use. 
  @param nmessages pointer to an int. The value of this integer will be set to
  the number of messages in queue
  @param accum_nbytes pointer to an int. The value of this integer will be set
  to the sum of payload sizes in all messages. 
*/
inline void bspx_qsize (BSPObject * bsp, int * RESTRICT nmessages, size_t * RESTRICT accum_nbytes)
{
	*nmessages = bsp->message_queue.n_mesg;
	*accum_nbytes = bsp->message_queue.accum_size;
}

/** Retrieves the tag and payload size of the current message in
 * queue.
 * @param bsp The BSPObject to use. 
   @param status the size of the payload of the current message, or when the
   queue is empty -1
   @param tag a pointer to a memory location big enough to contain a tag.
 */  
           
inline void bspx_get_tag (BSPObject * bsp, int * RESTRICT status , void * RESTRICT tag)
{
	if (bsp->message_queue.n_mesg == 0)
		*status = -1;
	else
	{
		ALIGNED_TYPE * RESTRICT current_tag = 
			bsp->message_queue.head + 
			no_slots( sizeof(DelivElement), sizeof(ALIGNED_TYPE));
		DelivElement * RESTRICT message = (DelivElement *) bsp->message_queue.head;	
		*status = message->size;
		memcpy(tag, current_tag, bsp->message_queue.recv_tag_size ); 
	}
}

/** Dequeue the current message.
* @param bsp The BSPObject to use. 
  @param payload A pointer to a memory location big enough to contain the
  payload or \a reception_nbytes
  @param reception_nbytes The maximum number of bytes to copy
*/  
inline void bspx_move (BSPObject * bsp, void *payload, size_t reception_nbytes)
{
	DelivElement * RESTRICT message = (DelivElement *) bsp->message_queue.head;	
	int copy_bytes = MIN((unsigned)reception_nbytes, message->size);
	char * RESTRICT current_payload =
		(char *) bsp->message_queue.head + 
		sizeof(ALIGNED_TYPE) * 
		no_slots( sizeof(DelivElement), sizeof(ALIGNED_TYPE)) +
		bsp->message_queue.recv_tag_size;
	memcpy(payload, current_payload, copy_bytes);

	bsp->message_queue.head += message->next;
	bsp->message_queue.n_mesg --;
	bsp->message_queue.accum_size -= message->size;
}

/** Sets the tag size at the next superstep.
* @param bsp The BSPObject to use. 
  @param tag_nbytes pointer to an int which should contain the size of the tag
  in bytes. It becomes current tag size.
 */ 
inline void bspx_set_tagsize (BSPObject * bsp, size_t *tag_nbytes)
{
	DelivElement element;
	element.info.settag.tag_size = (unsigned int )*tag_nbytes;
	element.size = 0;

	deliveryTable_push(&bsp->delivery_table, bsp->rank, &element, it_settag);
	*tag_nbytes = bsp->message_queue.send_tag_size;
}

/*@}*/


/** @name High Performance */
/*@{*/

/** Dequeue the current message in an unbuffered way. 
* @param bsp The BSPObject to use. 
  @param tag_ptr a pointer to reference of memory location which will contain
                the tag 
  @param payload_ptr a pointer to a reference of a memory location which will
                contain the payload
  @return the payload size of the dequeued message
*/  
inline int bspx_hpmove (BSPObject * bsp, void **tag_ptr, void **payload_ptr)
{
	if (bsp->message_queue.n_mesg == 0)
		return -1;
	else
	{
		ALIGNED_TYPE * RESTRICT current_tag =
			(ALIGNED_TYPE *) bsp->message_queue.head + 
			no_slots(sizeof(DelivElement), sizeof(ALIGNED_TYPE));
		char * RESTRICT current_payload =
			(char *) current_tag + bsp->message_queue.recv_tag_size;
		DelivElement * RESTRICT message =
			(DelivElement *) bsp->message_queue.head;
		int size = message->info.send.payload_size;	 
		*tag_ptr     = current_tag;
		*payload_ptr = current_payload;
		bsp->message_queue.head += message->next;
		bsp->message_queue.n_mesg--;
		bsp->message_queue.accum_size -= size;
		return size;
	}
}

/** Puts a block of data in the memory of some other processor at the next
 * superstep. This function is buffered, i.e.: the contents of \a src
 * is transmitted at the next bsp_sync(). Copying might start instantly.
 * @param bsp The BSPObject to use. 
 * @param pid rank of destination (remote) processor
 * @param src pointer to source location on source (local) processor
 * @param dst pointer to destination location on source processor. Translation
              of addresses is performed with help of earlier calls to bsp_push_reg()
   @param offset offset from \a dst in bytes (comes in handy when working with arrays)
   @param nbytes number of bytes to be copied
   @see bsp_push_reg()
*/
inline void bspx_hpput (BSPObject * bsp, int pid, const void * src, void * dst, long int offset, size_t nbytes) {
	bspx_put(bsp, pid, src, dst, offset, nbytes);
}

/** Gets a block of data from the memory of some other processor at the next
 * superstep. This function is unbuffered, i.e.: The data is retrieved from the
 * destination processor at any point from the call. Translation of
 * the \a src pointer is performed with help of earlier calls to
 * bsp_push_reg()
 * @param bsp The BSPObject to use. 
 * @param pid Ranks of the source (remote) processor 
 * @param src Pointer to source location using a pointer to a local memory
 *            region 
 * @param offset offset from \a src in bytes
 * @param dst Pointer to destination location 
 * @param nbytes Number of bytes to be received
 * @see bsp_push_reg()
*/
inline void bspx_hpget (BSPObject * bsp, int pid, const void * src, long int offset, void * dst, size_t nbytes) {
	bspx_get(bsp, pid, src, offset, dst, nbytes);
}

/*@}*/
