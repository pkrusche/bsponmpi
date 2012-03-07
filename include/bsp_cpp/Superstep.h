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
 */

#define BSP_SCOPE(context_type, context, p)		\
	typedef context_type __bspmy_type;			\
	context_type & __bspcx (context);			\
	bsp::ContextFactoryPtr factory (			\
		new bsp::ContextFactory<context_type>	\
			(&context) );						\
	bsp::TaskMapper tm (p, factory);			\
	context.set_task_mapper(tm);

#define BSP_BEGIN()	{							\
		class MyRC : public __bspmy_type {		\
		public:									\
			void run () {

#define BSP_END()								\
			} 									\
			static void run_as(Context * thiz) {\
			( (MyRC*)thiz )->run();				\
			}									\
		};										\
		__bspcx.run_in_context(&MyRC::run_as);	\
	}											\
	__bspcx.bsp_sync();	

/**
 * We need to be able to sync *after* all the tasks
 * have finished, which is why we are replacing 
 * the standard bsp_sync with this construct.
 */
#define BSP_SYNC()	BSP_END() BSP_BEGIN()



#endif
