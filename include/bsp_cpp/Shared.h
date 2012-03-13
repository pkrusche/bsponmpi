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


/** @file Shared.h

Helper class to allow value sharing between contexts.

@author Peter Krusche
*/

#ifndef __BSP_SHARED_H__
#define __BSP_SHARED_H__ 

#include <iostream>
#include <stdexcept>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <tbb/parallel_reduce.h>
#include <tbb/parallel_for.h>

namespace bsp {
	
	class Context;
	
	/** this class is used to declare variables shared between 
	    parent and child contexts 
		
		The two functions initialize() and reduce() can be
		overloaded and will be used to 
	*/
	class Shared {
	public: 
		void add_input ( const SharedVar * s ) {
			vars.push_back(s);
		}

		virtual void reduce() {}
		virtual void initialize()  {}
		
	protected:
		std::vector<SharedVar*> vars;
	};
		
	/** this class implements sharing of an equal number of variables. */
	class SharedVariableSet {
	public:
		/** add a shared variable to a given id */
		void add_var (const char * id, SharedVar * v) {
			ASSERT (svl.find(std::string(id)) == svl.end());
			svl[std::string(id)] = v;
		}

		/** will add all variables in vs as inputs to all matching variables in 
		    this set */
		void add_inputs (SharedVariables & vs) {
			for (std::map<std::string, Shared*>::iterator it = vs.svl.begin(); 
			it != vs.svl.end(); ++it) {
				std::map<std::string, Shared*>::iterator it2 = svl.find(it->first);
				if (it2 != svl.end()) {
					it2->second->add_input(it->second);
				}
			}
		}
		
		/** run all initializers */
		void initialize_all() {
			for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
				it != svl.end(); ++it) {
				it->second->initialize();
			}			
		}

		/** run all reducers */
		void reduce_all() {
			for (std::map<std::string, Shared*>::iterator it = svl.begin(); 
				it != svl.end(); ++it) {
				it->second->reduce();
			}
		}
	private:
		std::map<std::string, Shared*> svl;
	};

	/** Shared variable implementation */
	template <class _init, class _red>
	class SharedVariable {
	public:
		void initialize() {
			_init i (vars);
			tbb::parallel_for( tbb::blocked_range<int> (0, vars.size() - 1), i );
		}
		
		void reduce() {
			_red r (vars);
			tbb::parallel_reduce( tbb::blocked_range<int> (0, vars.size() - 1), i );
		}		
	};
};

#include "Shared/Initializers.h"
#include "Shared/Reducers.h"

#endif
