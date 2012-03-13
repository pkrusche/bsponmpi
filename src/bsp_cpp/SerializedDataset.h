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


/** @file SerializedDataset.h

@author Peter Krusche
*/

#ifndef __SerializedDataset_H__
#define __SerializedDataset_H__

#include "bsp_cpp/Shared/Shared.h"
	
/** helper class to serialize many data items into 
 *  a single dataset which can be broadcasted to all
 *  nodes */
class SerializedDataset {
public:
	/** initialize directly from buffer */
	SerializedDataset(void * xdata, int _nelem, int _size): data(), offsets (data.data) {
		// size must divide evenly by 8, otherwise, this is most
		// likely not what we're looking for.
		ASSERT ((_size & 7) == 0);
		data.data_is_mine = false;
		data.data = (uint64_t*)xdata;
		data.size = _size / sizeof(uint64_t);
		nelem     = _nelem;
		current_el = 0;
	}

	/** initialize with fixed element size */
	SerializedDataset (int _nelem) : data(2 + _nelem), offsets (data.data) {
		ASSERT (sizeof (uint64_t) == 8);
		current_el = 0;
		nelem = _nelem;
		data.resize(2 + nelem);
		data_pos = 2 + nelem;
		memset (data.data, 0, sizeof(uint64_t)*(nelem + 2));
	}

	/** add an element to buffer 
	 * 
	 * we add the element's id first, then the element data.
	 * 
	 */
	void add_elem (std::string id, bsp::Shared * s) {
		size_t sz = s->serialized_size();
		size_t total_sz = sz + 2*sizeof(uint64_t) + id.size();
		
		size_t next = data_pos + ((total_sz + 31) >> 3);
		if (next >= data.exact_size()) {
			data.resize(next+1);
		}

		uint64_t * p = data.data + data_pos;
		
		*p++ = id.size();
		memcpy(p, id.c_str(), id.size());
		p+= ((id.size() + 7) >> 3);

		*p++ = sz;
		s->serialize(p, sz);

		ASSERT ( (unsigned) (p - data.data) <= next);

		offsets[current_el++] = data_pos;
		data_pos = next;
	}

	/** broadcast from master node */
	void broadcast (int master_node) {
		using namespace bsp;
		/** broadcast data from master node */
		bsp_broadcast(master_node, data_pos);

		if (bsp_pid() == master_node) {
			/** transfer element size and current element
			 *  element size must be first element so we 
			 *  know where to look for it */
			data.data[nelem] = data.data[0];
			data.data[0] = nelem;
			data.data[nelem+1] = current_el;
		}

		if (data.exact_size() < data_pos + 1) {
			data.resize(data_pos + 1);
		}

		bsp_broadcast(master_node, data.data, data_pos + 1);

		nelem = data.data[0];
		data.data[0] = data[(int)nelem];
		data[(int)nelem] = 0;
		current_el = data[(int)nelem + 1];
	}

	/** restart data insertion/pickup */
	void restart () {
		current_el = 0;
		data_pos = offsets[0];
	}

	/** next name */
	std::string next_name () {
		uint64_t len = data.data[data_pos];
		return std::string((char*)(data.data + data_pos + 1), len);
	}

	/** next element */
	void get_elem (bsp::Shared * el) {
		ASSERT (current_el < nelem);
		uint64_t * start = data.data + offsets[current_el];
		uint64_t idlen = *start;
		start++;
		start += ((idlen + 7) >> 3);
		el->deserialize(start+1, *start );
		++current_el;
		data_pos = offsets[current_el];
	}

	/** return number of elements */
	size_t elements() {
		return (size_t)nelem;
	}

	/** return data size in bytes */
	size_t size() {
		return data.exact_size() * sizeof(uint64_t);
	}

	/** get the data pointer */
	void * get_data() {
		return data.data;
	}

private:
	utilities::AVector<uint64_t> data;
	uint64_t data_pos;

	/** all these are stored in data */
	uint64_t * & offsets;
	uint64_t nelem;
	uint64_t current_el;
};

#endif // __SerializedDataset_H__

