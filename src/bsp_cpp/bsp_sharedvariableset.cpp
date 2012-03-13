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

#include "bsp_level1.h"

#include "bsp_cpp/Shared/SharedVariableSet.h"

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
void bsp::SharedVariableSet::add_inputs (bsp::SharedVariableSet & vs) {
	for (std::map<std::string, Shared*>::iterator it = vs.svl.begin(); 
	it != vs.svl.end(); ++it) {
		std::map<std::string, Shared*>::iterator it2 = svl.find(it->first);
		if (it2 != svl.end()) {
			it2->second->add_child(it->second);
		}
	}
}
		
/** run all initializers */
void bsp::SharedVariableSet::initialize_all() {
	
#ifdef _DEBUG

	/** in debug mode, we check if all the sizes are the same */
	struct equal_size {
		size_t operator() (size_t l, size_t r) {
			if (l != r) {
				throw std::runtime_error("bsp::SharedVariableSet size check failed.");
			}
			return l;
		}
	};

	size_t size = initialize_list.size();
	bsp_fold <size_t, equal_size> (size, size);

#endif

	/** Step 1: initialize */
	for (std::set<std::string>::iterator it = initialize_list.begin(); 
		it != initialize_list.end(); ++it) {

	}


	for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
		it != svl.end(); ++it) {
		it->second->initialize();
	}
	
}

/** run all reducers */
void bsp::SharedVariableSet::reduce_all() {
	for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
		it != svl.end(); ++it) {
		it->second->reduce();
	}
}
