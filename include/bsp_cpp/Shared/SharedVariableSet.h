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


/** @file SharedVariableSet.h

@author Peter Krusche
*/

#ifndef __SharedVariableSet_H__
#define __SharedVariableSet_H__

#include <set>
#include <map>
#include <string>

#include "../TaskMapper.h"
#include "Shared.h"
#include "SharedVariable.h"

namespace bsp {

	/** This class implements sharing of an equal number of variables. 
	 * 
	 *  This can be seen as an extension of the BSP level 1 functionality
	 *  in BSPlib. A shared variable set is a set of shared variables 
	 *  ( @see bsp::Shared ) which get initialized and reduced simultaneously
	 *  through a node-level collective call.
	 *  
	 *  It is implemented on top of bsp_broadcast and bsp_fold.
	 * 
	 */
	class SharedVariableSet {
	public:
		~SharedVariableSet();

		/** 
		 * Get the reduce slot for a slot id.
		 * 
		 * @return the reduce slot for id, or NULL
		 * 
		 */
		Shared ** get_reduce_slot (const char * id);

		/** Add a shared variable to a given slot id 
		 *
		 * Variables are connected by slot ids. All variables with the
		 * same slot will be initialized and reduced together.
		 *
		 * @param id the slot id to attach the variable to
		 * @param v the variable to attach (will be deleted upon destruction of this set)
		 * @param init true if variable should be used when initializing
		 * @param reduce true if variable should be used when reducing
		 * 
		 */
		void add_var (const char * id, Shared * v, bool init, bool reduce);

		/** will add all variables in vs as inputs to all matching variables in 
		    this set 
			
			This will match all variables with slots in vs to variables with 
			the same slot in this.

			All variables in vs which have a matching slot in this will be attached
			to this.
		 */
		void add_as_children (SharedVariableSet & vs);

		/**
		 * Clear all child connections
		 */
		void clear_all_children ();

		/**
		 * Initialize a single slot variable
		 * 
		 * @param slot the slot
		 * @param master_node the node to initialize from
		 * @param _mapper the task mapper
		 */
		void initialize(const char * slot, int master_node, TaskMapper * _mapper);

		/**
		 * Reduce a single slot variable
		 * 
		 * @param slot the slot
		 * @param global true when reducing between all nodes
		 * @param _mapper the task mapper
		 */
		void reduce(const char * slot, bool global, TaskMapper * _mapper);

		/** run all initializers
		 *  (node-level collective)
		 *  
		 *  @param master_node the node from which to broadcast values. -1 to use node-local values
		 * @param _mapper the task mapper
		 *  
		 */
		void initialize_all(int master_node, TaskMapper * _mapper);

		/** run all reducers 
		 *  (node-level collective)
		 * 
		 * @param _mapper the task mapper
		 *  
		 */
		void reduce_all(TaskMapper * _mapper);

		/** Function to add a shared variable to a SharedVariableSet
		 *  for initialisation only.
		 *  This is a friend function so we can keep SharedVariableSet 
		 *  independant of the SharedVariable templates (we only store
		 *  pointers to the Shared interface)
		 */
		template <class _t>
		friend inline void sharedvariable_init ( 
			bsp::SharedVariableSet & set, const char * id , _t & value ) {
			set.add_var(
				id, 
				new bsp::SharedVariable< 
						_t, 
						bsp::ReduceFirst<_t> 
				> (value, value), 
			true, false );
		}

		/** Function to add a shared variable to a SharedVariableSet
		 *  for initialisation and reduction.
		 *  This is a friend function so we can keep SharedVariableSet 
		 *  independant of the SharedVariable templates (we only store
		 *  pointers to the Shared interface)
		 */
		template <class _t, template<class> class _r >
		friend inline void sharedvariable_init_reduce( 
			bsp::SharedVariableSet & set,
			const char * id , 
			_t & value, 
			bool also_init) { 

			bsp::SharedVariable< 
						_t, 
						_r <_t> 
				> * my_var = new bsp::SharedVariable< 
						_t, 
						_r <_t> 
				> (value);

			set.add_var(
				id, 
				my_var, 
			also_init, true );

			if ( ::bsp_nprocs() > 1 ) {
				bsp::Shared ** psp = new bsp::Shared * [::bsp_nprocs() + 1 ];
		
				for (int p = 0; p <= ::bsp_nprocs() ; ++p) {
					// this one is assigned in init_reduce_slot
					if (p == ::bsp_pid()+1) {
						psp[p] = my_var;
					} else {
						psp[p] = new bsp::SharedVariable< 
									_t, 
									_r <_t> 
								> ();					
					}
				}

				set.init_reduce_slot( id, psp );
			}
		}

	private:

		/** 
		 * To be able to reduce on a node-level, we need an additional 
		 * slot for reducing values from all other nodes. 
		 * 
		 * This function is used to initialize that slot.
		 * 
		 * @param id the slot id
		 * @param v the values (pointer to an array of bsp_nprocs() - 1 values)
		 * 
		 * @see sharedvariable_init
		 * @see sharedvariable_init_reduce
		 * 
		 */
		void init_reduce_slot (const char * id, Shared ** v);

		/** Reduction implementation */
		void reduce_impl(std::set<std::string> const &, TaskMapper *);

		std::map<std::string, Shared*> svl; ///< slot to variable mapping
		std::map<std::string, Shared**> reduce_svl; ///< in each slot, we also have one additional variable for node-level reducing
		std::set<std::string> initialize_list; ///< list of all slots that will be initialized
		std::set<std::string> reduce_list; ///< list of all slots that will be reduced
	};

#define SHARE_VARIABLE_IR(set, reduce, var, ...) do { 							\
	bsp::sharedvariable_init_reduce<__VA_ARGS__, reduce > (set, #var, var, true);	\
} while(0)

#define SHARE_VARIABLE_R(set, reduce, var, ...) do { 							\
	bsp::sharedvariable_init_reduce<__VA_ARGS__, reduce > (set, #var, var, false);	\
} while(0)

#define SHARE_VARIABLE_I(set, var, ...) do { 								\
	bsp::sharedvariable_init<__VA_ARGS__> (set, #var, var);											\
} while(0)


};

#endif // __SharedVariableSet_H__
