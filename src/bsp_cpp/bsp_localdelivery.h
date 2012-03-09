/*
BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
Copyright (C) 2006  Wijnand J. Suijlen, 2012 Peter Krusche

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


/** @file bsp_localdelivery.h

Implements local delivery. All functions in here are not thread safe.

@author Peter Krusche
*/

#ifndef __bsp_localdelivery_H__
#define __bsp_localdelivery_H__

#include <vector>
#include <algorithm>

#include "bsp_tools/MessageBuffer.h"

#include "HeaderQueue.h"

namespace bsp {
	

	/** a local memory delivery which will be
	 *  carried out once a superstep is over
	 */
	struct LocalMemoryDelivery {
		const char * src;
		char * dst;
		size_t nbytes;
	};

	/** in buffered deliveries we copy the data 
	 *  to a buffer when it's sent. */
	struct BufferedLocalMemoryDelivery {
		size_t offset;
		char * dst;
		size_t nbytes;
	};

	/** BSMP message headers */
	struct BSMessage {
		bool buffered;
		union {
			size_t offset;
			const void * data;
		} tag;
		union {
			size_t offset;
			const void * data;
		} src;
		size_t nbytes;
	};


	/** This is a bit like a single-column C++ version of expandableTable */
	class LocalDeliveryQueue {
	public:

		LocalDeliveryQueue () {
			message_send_buffer = message_buffer;
			message_send_queue = messages;
			message_move_buffer = message_buffer + 1;
			message_move_queue = messages + 1;
		}

		/** execute all queued deliveries */
		inline void execute () {
			// unbuffered deliveries.
			while (!hpputs.empty()) {
				LocalMemoryDelivery & d (hpputs.head());
				memcpy (d.dst, d.src, d.nbytes);
				hpputs.next();
			}

			// buffered deliveries.
			while (!puts.empty()) {
				BufferedLocalMemoryDelivery & d (puts.head());
				memcpy (d.dst, put_buffer.get(d.offset), d.nbytes);
				puts.next();
			}

			// clear put buffer
			put_buffer.clear();
		}

		/** enqueue a put operation */
		inline void put ( char * src, char * dst, size_t nbytes ) {
			BufferedLocalMemoryDelivery & d(puts.enqueue());
			d.dst = dst;
			d.nbytes = nbytes;
			d.offset = put_buffer.buffer(src, nbytes);
		}

		/** enqueue an unbuffered put operation */
		inline void hpput ( char * src, char * dst, size_t nbytes ) {
			LocalMemoryDelivery & d(hpputs.enqueue());
			d.dst = dst;
			d.src = src;
			d.nbytes = nbytes;
		}

		/** enqueue a BSMP message */
		inline void send( const void * tag, size_t tagsize, const void * data, size_t nbytes ) {
			BSMessage & m (message_send_queue->enqueue());
			m.buffered = true;
			m.nbytes = nbytes;
			m.src.offset = message_send_buffer->buffer(data, nbytes);
			m.tag.offset = message_send_buffer->buffer(tag, tagsize);
			bytes_sent+= nbytes;
		}

		/** enqueue a BSMP message (unbuffered) */
		inline void hpsend( const void * tag, const void * data, size_t nbytes ) {
			BSMessage & m (message_send_queue->enqueue());
			m.buffered = false;
			m.nbytes = nbytes;
			m.src.data = data;
			m.tag.data = tag;
			bytes_sent+= nbytes;
		}

		/** how many messages do we have queued */
		inline int bsmp_qsize () {
			return message_move_queue->qsize();
		}

		/** how many bytes are in the BSMP move queue */
		inline size_t bsmp_move_bytes () {
			return bytes_to_move;
		}

		/** Get the start address of next message from the start of the 
		 * queue, advance. 
		 * 
		 * Queue must not be empty, otherwise, result is undefined.
		 * */
		inline const void * bsmp_top_message () {
			ASSERT(!message_move_queue->empty());

			BSMessage & m (message_move_queue->head());
			return m.buffered ? message_move_buffer->get(m.src.offset) : m.src.data;
		}

		/** get tag of top message 
		 * Queue must not be empty, otherwise, result is undefined.
		 */
		inline const void * bsmp_top_tag ( ) {
			ASSERT(!message_move_queue->empty());
			BSMessage & m (message_move_queue->head());
			return m.buffered ? message_move_buffer->get(m.tag.offset) : m.tag.data;
		}

		/** get top message size
		* Queue must not be empty, otherwise, result is undefined.
		 ** */
		inline size_t bsmp_top_size () {
			ASSERT(!message_move_queue->empty());
			BSMessage & m (message_move_queue->head());
			return m.nbytes;
		}

		/** advance to next BSMP message 
		 * 
		 * @return true if such a message exists. false if queue is empty
		 */
		inline bool bsmp_advance () {
			if(!message_move_queue->empty()) {
				BSMessage & m (message_move_queue->head());
				bytes_to_move -= m.nbytes;
				message_move_queue->next();
				if (message_move_queue->empty()) {
					return false;
				}
				return true;
			}
			return false;
		}

		/** switch message buffers */
		void bsmp_messagequeue_sync() {
			using namespace std;
			swap (message_send_queue, message_move_queue);
			swap (message_send_buffer, message_move_buffer);

			bytes_to_move = bytes_sent;
			bytes_sent = 0;

			message_send_buffer->clear();
			message_send_queue->empty();
		}

		/** reset the buffer sizes */
		inline void reset_buffers () {
			put_buffer.clear();
			puts.reset();
			hpputs.reset();
			messages[0].reset();
			messages[1].reset();
			message_buffer[0].clear();
			message_buffer[1].clear();
			bytes_sent = 0;
			bytes_to_move = 0;
		}

	private:

		// buffered put requests
		utilities::HeaderQueue<BufferedLocalMemoryDelivery> puts;
		utilities::MessageBuffer put_buffer;

		// unbuffered put requests
		utilities::HeaderQueue<LocalMemoryDelivery> hpputs;

		// message double-buffer
		utilities::MessageBuffer message_buffer[2];
		utilities::HeaderQueue<BSMessage> messages[2];

		utilities::MessageBuffer * message_send_buffer;
		utilities::HeaderQueue<BSMessage> * message_send_queue;

		utilities::MessageBuffer * message_move_buffer;
		utilities::HeaderQueue<BSMessage> * message_move_queue;

		size_t bytes_sent;
		size_t bytes_to_move;

	};

};


#endif // __bsp_localdelivery_H__

