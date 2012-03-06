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


/** @file bsp_tbb.cpp

Declaration of global objects used in conjunction with TBB.

@author Peter Krusche
*/

#include "bsp_config.h"

#include <tbb/task_scheduler_init.h>

namespace bsp {
	tbb::task_scheduler_init * g_task_scheduler_init = NULL;

};

extern "C" {

	/** Create task scheduler with a specified number of threads */
	void bsp_init_tbb (int nthreads) {
		if (nthreads <= 0) {
			bsp::g_task_scheduler_init = new tbb::task_scheduler_init();
		} else {
			bsp::g_task_scheduler_init = new tbb::task_scheduler_init(nthreads);
		}
	}

	/** exit TBB */
	void bsp_exit_tbb () {
		delete bsp::g_task_scheduler_init;
	}

};

