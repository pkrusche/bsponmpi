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


/** @file bsp_contextimpl_mpi.h

Implementation header for all 

@author Peter Krusche
*/

#ifndef __bsp_contextimpl_mpi_H__
#define __bsp_contextimpl_mpi_H__

#include "bspx.h"

#include <map>
#include <queue>

#include "bsp_cpp/TaskMapper.h"
#include "bsp_tools/Avector.h"

namespace bsp {

	struct MemoryRegister {
		const void **		pointers;
		size_t				nbytes;
		size_t				serial;
	};

	struct MemoryRegister_Reg {
		const void * data;
		size_t size;
		size_t serial;
		bool   push;
	};

	class ContextImpl {
	public:

		enum {
			MAX_REGISTER_REQS = 0xfffffff,	///< maximum number of memory register (de-)registrations per superstep
		};

		ContextImpl(TaskMapper * tm, int local_pid);
		~ContextImpl();

		static void bsp_sync (bsp::TaskMapper * mapper);

		void bsp_push_reg (const void *, size_t);
		void bsp_pop_reg (const void *);
		void bsp_put (int, const void *, void *, long int, size_t);
		void bsp_get (int, const void *, long int, void *, size_t);

		void bsp_send (int, const void *, const void *, size_t);
		void bsp_qsize (int * , size_t * );
		void bsp_get_tag (int * , void * );
		void bsp_move (void *, size_t);
		void bsp_set_tagsize (size_t *);

		void bsp_hpput (int, const void *, void *, long int, size_t);
		void bsp_hpget (int, const void *, long int, void *, size_t);
		int bsp_hpmove (void **, void **);

	private:

		/************************************************************************/
		/* Synchronization steps                                                */
		/************************************************************************/

		/** process memory register registrations 
		 *
		 * @param reg_req_size : number of push and pop requests.
		 */

		static void process_memoryreg_ops(bsp::TaskMapper * mapper, int reg_req_size);

		/** process get requests */
		static void process_get_requests();

		int local_pid; ///< local pid 

		/** remember the task mapper */
		TaskMapper * mapper;

		/** We reimplement BSPonMPI's registration mechanism here
		 *  using C++ maps. 
		 */

		bool any_hp;

		std::map<const void*, MemoryRegister> memory_register_map; /**< all valid memory registers, indexed by ptr 
															     in this context */

		std::queue< MemoryRegister_Reg > reg_requests;	/**< new registrations are buffered here */

		static utilities::AVector <unsigned int> h_send;	/**< superstep input element counts */
		static utilities::AVector <unsigned int> h_recv;	/**< superstep output element counts */
	};

};

#endif // __bsp_contextimpl_mpi_H__
