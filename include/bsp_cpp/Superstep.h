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


/** @file Superstep.h
 
 This file contains helper macros which allow
 declaring sequences of supersteps more easily.

@author Peter Krusche
*/

#ifndef __Superstep_H__
#define __Superstep_H__

#include <list>
#include <vector>

#include "bsp_cpp.h"

/**
 * BSP_BEGIN () macro 
 * 
 * 
 * 
 */

#define BSP_BEGIN(context, taskmapper) {		\
	bsp::TaskMapper & tm (taskmapper);			\
	typedef context ContextData;				\
	{											\
		class MyRC : public ContextData {		\
		public:									\
			void run () {

/**
 * We need to be able to sync *after* all the tasks
 * have finished, which is why we are replacing 
 * the standard bsp_sync with this construct.
 */
#define BSP_SYNC()								\
			} 									\
			static void run_as(Context * thiz) {\
				( (MyRC*)thiz )->run();			\
			}									\
		};										\
		bsp::run_context(&tm, &MyRC::run_as); 	\
	}											\
	Context::sync_contexts(&tm);				\
	{											\
		class MyRC : public ContextData {		\
		public:									\
			void run () {						

#define BSP_END()								\
			} 									\
			static void run_as(Context * thiz) {\
				( (MyRC*)thiz )->run();			\
			}									\
		};										\
												\
		bsp::run_context(&tm, &MyRC::run_as);	\
	}											\
	Context::sync_contexts(&tm);				\
}

#endif
