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
#include <stdexcept>
#include <tbb/scalable_allocator.h>

#include "bsp_tools/Avector.h"

#include "bsp_level1.h"

#include "bsp_cpp/Shared/SharedVariableSet.h"

#include "SerializedDataset.h"


/** Delete all shared pointers */
bsp::SharedVariableSet::~SharedVariableSet() {
	for (std::map<std::string, Shared**>::iterator it = reduce_svl.begin(); 
		it != reduce_svl.end(); ++it) {
		for (int p = 0; p < ::bsp_nprocs() + 1; ++p) {
			delete it->second[p];
		}
		delete [] it->second;
	}
	for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
		it != svl.end(); ++it) {
		delete it->second;
	}
}


/** find reduce slot */
bsp::Shared ** bsp::SharedVariableSet::get_reduce_slot(const char * id) {
	std::map<std::string, Shared**>::iterator it = reduce_svl.find(std::string (id));
	if (it == reduce_svl.end()) {
		return NULL;
	} else {
		return it->second;
	}
}

/** initialize a reduce slot */
void bsp::SharedVariableSet::init_reduce_slot (const char * id, bsp::Shared ** v) {
#ifdef _DEBUG
	bsp::Shared ** s = get_reduce_slot(id);
	if (s != NULL) {
		throw std::runtime_error( "Reduce slot initialized twice." );
	}
#endif
	std::string n(id);
	reduce_svl[ n ] = v;
	v[0]->add_child ( svl[ n ] );
	for (int p = 1; p <= bsp_nprocs(); ++p) {
		v[0] -> add_child ( v[p] );
	}
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

/** Clear all child variable connections */
void bsp::SharedVariableSet::clear_all_children () {
	for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
		it != svl.end(); ++it) {
			it->second->reset_children();
	}
}

/** initialize a single slot */
void bsp::SharedVariableSet::initialize(const char * slot, int master_node) {
	std::map<std::string, Shared*>::iterator it = svl.find(slot);
	ASSERT(it != svl.end());
	
	/** if we initialize from a node, we need to broadcast 
	 *  all data first */
	if (bsp_nprocs() > 1 && master_node >= 0) {
		bsp::bsp_broadcast(master_node, it->second);
	}
	
	it->second->initialize();
}

/** reduce a single slot */
void bsp::SharedVariableSet::reduce(const char * slot, bool global) {
	if ( ::bsp_nprocs() > 1 && global ) {
		std::set<std::string> s;
		s.insert(slot);
		reduce_impl(s);
	} else {
		std::map<std::string, Shared*>::iterator it = svl.find(slot);
		ASSERT(it != svl.end());

		it->second->reduce();
	}
}

/** run all initializers */
void bsp::SharedVariableSet::initialize_all( int master_node ) {
	/** if we initialize from a node, we need to broadcast 
	 *  all data first */
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

	// then, initialize everything locally
	for (std::set<std::string>::iterator it = initialize_list.begin(); 
		it != initialize_list.end(); ++it) {
		svl[*it]->initialize();
	}
}

/** run all reducers */
void bsp::SharedVariableSet::reduce_all() {
	reduce_impl(reduce_list);
}

/** Reduction implementation */
void bsp::SharedVariableSet::reduce_impl(std::set<std::string> const & _reduce_list) {
	SerializedDataset sds ((int)_reduce_list.size());

	for (std::set<std::string>::const_iterator it = _reduce_list.cbegin(); 
		it != _reduce_list.cend(); ++it) {
			bsp::Shared * p = svl[*it];
			p->reduce();
			sds.add_elem(*it, p);
	}

#ifdef _HAVE_MPI
	if (bsp_nprocs() > 1) {
		int myelems [2] = {
			(int)sds.elements(),
			(int) sds.size()
		};

		int * elems = (int* ) bsp_malloc ( bsp_nprocs(), 2*sizeof(int) );
		MPI_Allgather(&myelems, 2, MPI_INT, elems, 2, MPI_INT, bsp_communicator);

		int size = 0;
		int * sizes =  (int* ) bsp_malloc ( bsp_nprocs(), sizeof(int) );
		int * offsets =  (int* ) bsp_malloc ( bsp_nprocs(), sizeof(int) );
		for (int p = 0; p < bsp_nprocs(); ++p) {
			offsets[p] = size;
			sizes[p] = elems[2*p+1];
			size+= elems[2*p+1];
		}

		char * target = (char*)bsp_malloc(size, 1);

		MPI_Allgatherv(sds.get_data(), myelems[1], MPI_BYTE, target, sizes, offsets, MPI_BYTE, bsp_communicator);

		int q = 1;
		for (int p = 0; p < bsp_nprocs(); ++p) {
			if (p == bsp_pid())
				continue;
			SerializedDataset other_ds(target + offsets[p], elems[2*p], sizes[p]);
			other_ds.restart();

			for(int k = 0; k < other_ds.elements(); ++k ) {
				std::string name = other_ds.next_name();
				std::map<std::string, bsp::Shared*>::iterator it  = svl.find(name);
				std::map<std::string, bsp::Shared**>::iterator itr = reduce_svl.find(name);

				if (it != svl.end() && itr != reduce_svl.end() ) {
					other_ds.get_elem(itr->second[q]);
				}
#ifdef _DEBUG
				else 
					std::cerr << "WARNING: inconsistent reduce lists, entry: " << name << std::endl;
#endif
			}
			q++;
		}

		bsp_free(target);
		bsp_free(offsets);
		bsp_free(sizes);
		bsp_free(elems);
	}
#endif

	for (std::set<std::string>::const_iterator it = _reduce_list.cbegin(); 
		it != _reduce_list.cend(); ++it) {
			bsp::Shared ** p = reduce_svl[*it];
			p[0]->make_neutral();
			p[0]->reduce();
			p[0]->initialize();
	}

}

