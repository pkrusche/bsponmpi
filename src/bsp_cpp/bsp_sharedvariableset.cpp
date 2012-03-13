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


/** @file bsp_sharedvariableset.cpp

@author Peter Krusche
*/

#include "bsp_config.h"

#include <iostream>
#include <tbb/scalable_allocator.h>

#include "bsp_tools/Avector.h"

#include "bsp_level1.h"

#include "bsp_cpp/Shared/SharedVariableSet.h"


/** helper class to serialize many data items into 
 *  a single dataset which can be broadcasted to all
 *  nodes */
class SerializedDataset {

public:
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

		if (data.exact_size() < sizeof(uint64_t)*(data_pos+1)) {
			data.resize(data_pos + 1);
		}

		bsp_broadcast(master_node, data.data, sizeof(uint64_t)*(data_pos+1));

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

		char * c = new char[len+1];
		c[len] = 0;
		memcpy(c, data.data + data_pos + 1, len);
		std::string s(c);

		return s;
	}

	/** next element */
	void get_elem (bsp::Shared * el) {
		ASSERT (current_el < nelem);
		uint64_t * start = data.data + offsets[current_el];
		start += 1 + *start;
		el->deserialize(start+1, *start );
		++current_el;
		data_pos = offsets[current_el];
	}

	/** return number of elements */
	size_t elements() {
		return (size_t)nelem;
	}
private:
	utilities::AVector<uint64_t> data;
	uint64_t data_pos;

	/** all these are stored in data */
	uint64_t * & offsets;
	uint64_t nelem;
	uint64_t current_el;
};


/** Delete all shared pointers */
bsp::SharedVariableSet::~SharedVariableSet() {
	for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
		it != svl.end(); ++it) {
		delete it->second;
	}
}


/** find reduce slot */
bsp::Shared * bsp::SharedVariableSet::get_reduce_slot(const char * id) {
	std::map<std::string, Shared*>::iterator it = reduce_svl.find(std::string (id));
	if (it == reduce_svl.end()) {
		return NULL;
	} else {
		return it->second;
	}
}

/** initialize a reduce slot */
void bsp::SharedVariableSet::init_reduce_slot (const char * id, bsp::Shared * v) {
	bsp::Shared * s = get_reduce_slot(id);
	if (s != NULL) {
		delete s;
	}
	reduce_svl[std::string (id)] = v;
}

/** add a shared variable to a given id */
void bsp::SharedVariableSet::add_var (const char * id, bsp::Shared * v, bool init, bool reduce) {
	std::string ids(id);
	ASSERT (svl.find(ids) == svl.end() || svl[ids] == v);
	ASSERT (init || reduce);
	svl[ids] = v;
	if ( init ) {
		initialize_list.insert(ids);
	}
	if ( reduce ) {
		reduce_list.insert(ids);
	}
}

/** will add all variables in vs as inputs to all matching variables in 
	this set */
void bsp::SharedVariableSet::add_as_children (bsp::SharedVariableSet & vs) {
	for (std::map<std::string, Shared*>::iterator it = vs.svl.begin(); 
	it != vs.svl.end(); ++it) {
		std::map<std::string, Shared*>::iterator it2 = svl.find(it->first);
		if (it2 != svl.end()) {
			it2->second->add_child(it->second);
		}
	}
}
		
/** run all initializers */
void bsp::SharedVariableSet::initialize_all( int master_node ) {
	/** if we initialize from a node, we need to broadcast 
	 *  all data first */
#ifdef _HAVE_MPI
	if (bsp_nprocs() > 1 && master_node >= 0) {
		SerializedDataset dataset ((int)initialize_list.size());

		/** Step 1: serialize all elements on master node */
		if (bsp_pid() == master_node) {
			for (std::set<std::string>::iterator it = initialize_list.begin(); 
				it != initialize_list.end(); ++it) {
				dataset.add_elem(*it, svl[*it]);
			}
		}

		/** send them to everyone */
		dataset.broadcast(master_node);

		dataset.restart();

		/** expand items everywhere but on the master node */
		if (bsp_pid() != master_node) {
			for (size_t el = 0; el < dataset.elements(); ++el) {
				std::map<std::string, bsp::Shared*>::iterator it = svl.find(dataset.next_name());
				if ( it != svl.end() ) {
					dataset.get_elem( it->second );
				}
#ifdef _DEBUG
				else 
					std::cerr << "WARNING: inconsistent initializer lists, entry: " << dataset.next_name() << std::endl;
#endif
			}
		}
	}
#endif

	// then, initialize everything locally
	for (std::set<std::string>::iterator it = initialize_list.begin(); 
		it != initialize_list.end(); ++it) {
		svl[*it]->initialize();
	}
}

/** run all reducers */
void bsp::SharedVariableSet::reduce_all() {
	SerializedDataset sds ((int)reduce_list.size());

	for (std::set<std::string>::iterator it = reduce_list.begin(); 
		it != reduce_list.end(); ++it) {
		bsp::Shared * p = svl[*it];
		p->reduce();

		if (get_reduce_slot(it->c_str()) != NULL) {
			sds.add_elem(*it, p);
		}
	}
	
	
}
