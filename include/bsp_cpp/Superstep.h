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

/** When running BSP contexts, we need to create a
 *  TaskMapper and a factory. This is the same everywhere,
 *  so we provide a shortcut.
 *  
 *  This must be called in a member function within the declared 
 *  context class, before the first BSP_BEGIN is executed.
 */

#define BSP_SETUP_CONTEXT(context, processors)					\
	typedef context ContextData;								\
	set_task_mapper( bsp::TaskMapperPtr (						\
		new bsp::TaskMapper (									\
			processors,											\
			bsp::ContextFactoryPtr (							\
				new bsp::ContextFactory<context> (this)			\
			)													\
		)														\
	) );

/**
 * BSP_BEGIN () macro 
 * 
 */

#define BSP_BEGIN() 							\
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
		bsp::run_context(get_task_mapper(),		\
		&MyRC::run_as);							\
	}											\
	bsp_sync();	

/**
 * We need to be able to sync *after* all the tasks
 * have finished, which is why we are replacing 
 * the standard bsp_sync with this construct.
 */
#define BSP_SYNC()	BSP_END(); BSP_BEGIN()



#endif
