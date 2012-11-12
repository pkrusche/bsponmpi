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


/** @file bsp_context_ts.h

@author Peter Krusche
*/

#ifndef __bsp_context_ts_H__
#define __bsp_context_ts_H__

#ifdef __cplusplus

#include <tbb/task_scheduler_init.h>
#include <tbb/spin_mutex.h>

namespace bsp {
	/** Mutex to make access to message buffers thread safe */
	extern tbb::spin_mutex * g_context_mutex;
	extern tbb::task_scheduler_init * g_tsinit;
};

/** Thread safety helper */
#define TSLOCK() tbb::spin_mutex::scoped_lock l (*bsp::g_context_mutex)

extern "C" {

#endif /* __cplusplus */

void init_tbb();
void exit_tbb();

#ifdef __cplusplus
};
#endif

#endif // __bsp_context_ts_H__

