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


/** @file bsp_contextimpl_memreg.cpp

Implementation of memory registers for Contexts

@author Peter Krusche
*/

#include "bsp_config.h" 

#include <algorithm>
#include <stdexcept>

#include "bsp_contextimpl.h"

#include "bsp_tools/Avector.h"

/** Sync memory register operations */
void bsp::ContextImpl::process_memoryreg_ops(TaskMapper * mapper, int reg_req_size) {
	int ppn = mapper->procs_per_node();
	utilities::AVector<MemoryRegister_Reg> vs, vr;
	vs.resize(g_bsp.nprocs * reg_req_size * ppn);
	vr.resize(g_bsp.nprocs * reg_req_size * ppn);

	for (int lp = 0; lp < mapper->procs_this_node(); ++lp) {
		ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(lp)->get_impl());
		int o = lp * reg_req_size;
		int r = 0;
		while (!cimpl->reg_requests.empty()) {
			for (int p = 0; p < g_bsp.nprocs; ++p) {
				vs[o + r + p * reg_req_size * ppn ] = cimpl->reg_requests.front();
			}
			cimpl->reg_requests.pop();
			++r;
		}
	}
	_BSP_COMM0 (
		vs.data, reg_req_size * ppn * sizeof (MemoryRegister_Reg), 
		vr.data, reg_req_size * ppn * sizeof (MemoryRegister_Reg) );

	for (int req = 0; req < reg_req_size; ++req) {
		size_t serial = -1;
		size_t size = -1;
		int push_or_pop = -1;

		for (int llp = 0; llp < mapper->procs_this_node(); ++llp) {
			ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(llp)->get_impl());
			int my_local_pid = cimpl->local_pid;

			MemoryRegister reg;

			reg.pointers = boost::shared_array<const void*>(new const void * [mapper->nprocs()]);

			MemoryRegister_Reg & my_r (vr[ g_bsp.rank*reg_req_size*ppn + my_local_pid*reg_req_size + req ]);

			for (int gp = 0; gp < mapper->nprocs(); ++gp) {
				int p = mapper->global_to_node(gp), lp = mapper->global_to_local(gp);

				MemoryRegister_Reg & r (vr[ p*reg_req_size*ppn + lp*reg_req_size + req ]);

				// Check ordering 
				if (serial == -1) {
					serial = r.serial;
				} else {
					if (serial != r.serial) {
						throw std::runtime_error("bsp_sync(): memory register setup sequence mismatch. Pushreg/popreg operations must be collectively called in the correct order.");
					}
				}

				// Check size
				if (size == -1) {
					size = r.size;
				} else {
					if (size != r.size) {
						throw std::runtime_error("bsp_sync(): memory register setup size mismatch. Pushreg/popreg operations must register blocks of the same size.");
					}
				}

				// check push or pop
				if (push_or_pop == -1) {
					push_or_pop = r.push ? 1 : 0;
				} else {
					if (r.push && push_or_pop != 1) {
						throw std::runtime_error("bsp_sync(): pushreg/popreg mismatch.");
					}
					if (!r.push && push_or_pop != 0) {
						throw std::runtime_error("bsp_sync(): pushreg/popreg mismatch.");
					}
				}

				// save pointer
				reg.pointers[gp] = r.data;
			}

			reg.nbytes = size;
			reg.serial = serial;

			if (push_or_pop == 1) {
				if (cimpl->memory_register_map.find(my_r.data) != cimpl->memory_register_map.end()) {
					throw std::runtime_error("bsp_sync(): detected duplicate pushreg for the same address.");
				}
				cimpl->memory_register_map[my_r.data] = reg; 
				for (int gp = 0; gp < mapper->nprocs(); ++gp) {
					memoryRegister_push(&cimpl->memory_register, gp, (const char*)reg.pointers[gp]);
				}
			} else {
				memoryRegister_pop(&cimpl->memory_register, cimpl->global_pid, (const char*) my_r.data);
				std::map<const void *, MemoryRegister>::iterator it = 
					cimpl->memory_register_map.find(my_r.data);
				
				if (it == cimpl->memory_register_map.end()) {
					throw std::runtime_error("bsp_sync(): mismatched popreg.");
				}
				cimpl->memory_register_map.erase(it);
			}
		}
	}
	for (int llp = 0; llp < mapper->procs_this_node(); ++llp) {
		ContextImpl * cimpl = (ContextImpl *)(mapper->get_context(llp)->get_impl());
		memoryRegister_pack(&cimpl->memory_register);
	}
}
