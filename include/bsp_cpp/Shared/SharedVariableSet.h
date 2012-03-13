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

#include "Shared.h"

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
		 * To be able to reduce on a node-level, we need an additional 
		 * slot for reducing values from all other nodes. 
		 * 
		 * This function is used to initialize that slot.
		 * 
		 * @param id the slot id
		 * @param v the values (pointer to an array of bsp_nprocs() - 1 values)
		 * 
		 * @see __shr_init_reduce_slot
		 * 
		 */
		void init_reduce_slot (const char * id, Shared ** v);

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
		
		/** run all initializers
		 *  (node-level collective)
		 *  
		 *  @param master_node the node from which to broadcast values. -1 to use node-local values
		 *  
		 */
		void initialize_all(int master_node);

		/** run all reducers 
		 *  (node-level collective)
		 *  
		 */
		void reduce_all();

	private:
		std::map<std::string, Shared*> svl; ///< slot to variable mapping
		std::map<std::string, Shared**> reduce_svl; ///< in each slot, we also have one additional variable for node-level reducing
		std::set<std::string> initialize_list; ///< list of all slots that will be initialized
		std::set<std::string> reduce_list; ///< list of all slots that will be reduced
	};

};

#endif // __SharedVariableSet_H__
