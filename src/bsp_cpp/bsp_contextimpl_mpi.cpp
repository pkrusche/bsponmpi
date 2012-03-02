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


/** @file bsp_contextimpl_mpi.cpp

@author Peter Krusche
*/

#include "bsp_config.h"

#include "bsp_contextimpl_mpi.h"

#include "bspx.h"

#ifdef _HAVE_MPI
#include <mpi.h>
#endif

tbb::spin_mutex bsp::ContextImpl::context_mutex;

bsp::ContextImpl::ContextImpl(int nprocs, int rank) {
	bspx_init_bspobject(&bsp, nprocs, rank);
	any_hp = false;
}

void bsp::ContextImpl::sync_hpops() {
#ifdef _HAVE_MPI
	// free all MPI windows.
	for ( std::map<void*, MPI_Win>::iterator it = hp_map.begin(), it_end = hp_map.end();
		it != it_end; ++it
		) {
			MPI_Win_fence(0, it->second);
	}
#endif
}

bsp::ContextImpl::~ContextImpl() {
#ifdef _HAVE_MPI
	// free all MPI windows.
	for ( std::map<void*, MPI_Win>::iterator it = hp_map.begin(), it_end = hp_map.end();
			it != it_end; ++it
		) {
		MPI_Win_fence(0, it->second);
		MPI_Win_free (it->second);
	}
#endif
	bspx_destroy_bspobject(&bsp);
}

