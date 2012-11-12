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


/** @file bsp_context_ts.cpp

@author Peter Krusche
*/

#include "bsp_context_ts.h"

namespace bsp {
	tbb::spin_mutex * g_context_mutex;
	tbb::task_scheduler_init * g_tsinit;
};

extern "C" void init_tbb() {
	bsp::g_tsinit = new tbb::task_scheduler_init(
		tbb::task_scheduler_init::default_num_threads()
	);
}

extern "C" void exit_tbb() {
	bsp::g_tsinit->terminate();
	delete bsp::g_tsinit;
}

