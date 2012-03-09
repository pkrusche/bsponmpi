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


/** @file HeaderQueue.h

A queue for message fixed size headers.

@author Peter Krusche
*/

#ifndef __HeaderQueue_H__
#define __HeaderQueue_H__

#include "bsp.h"

extern "C" {
#include "../bsp_exptable.h"
};


namespace utilities {

	template <class _header>
	class HeaderQueue {
	public:
		HeaderQueue() {
			SpecInfo info;
			fixedElSizeTable_initialize(
				&tbl, 1, BSP_REQTAB_MIN_SIZE, sizeof(_header), info );
			start = 0;
		}
		~HeaderQueue() {
			expandableTable_destruct(&tbl);
		}

		/** clear the queue, remove all headers */
		inline void clear () {
			tbl.used_slot_count[0] = 0;
			start = 0;
		}

		/** reset the queue, free memory  */
		inline void reset() {
			clear();
			expandableTable_resetrowcount(&tbl, BSP_REQTAB_MIN_SIZE);
		}

		/** return a reference to a new queue element */
		inline _header & enqueue () {
			_header * h = (_header*)fixedElSizeTable_push2(&tbl, 0);
			return *h;
		}

		/** length of the queue */
		inline int qsize () {
			return ((int) tbl.used_slot_count[0]) - ((int) start);
		}

		/** get a reference to the queue head */
		inline _header & head () {
			ASSERT (start < tbl.used_slot_count[0]);
			return  *((_header*)( (tbl.data + start * tbl.slot_size)));
		}

		/** return true if queue is empty */
		inline bool empty() {
			return start >= tbl.used_slot_count[0];
		}

		/** get next queue element, return true if there is one */
		inline void next() {
			++start;
		}

	private:
		ExpandableTable tbl;
		size_t start;
	};

}

#endif // __HeaderQueue_H__
