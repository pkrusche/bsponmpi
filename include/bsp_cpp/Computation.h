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


/** @file Computation.h

 We give a base class for BSP computations. 

@author Peter Krusche
*/

#ifndef __BSP_COMPUTATION_H__
#define __BSP_COMPUTATION_H__

#include "Context.h"
#include "TaskMapper.h"
#include "Shared.h"

namespace bsp {
	
	struct _bsp_end {};
	struct _bsp_sync {};
	
	/** BSP Computation base class.
	 * 
	 * When defining BSP computations, 
	 * 
	 */
	template <class _context>
	class Computation : public _context {
	public:
		typedef Computation<_context> bsp_context_t;

		Computation () : factory (
			new ContextFactory<bsp_context_t>
				(this) 
			) {}
		
		void bsp_end() {
			throw _bsp_end();
		}
				
		bool bsp_cont;
		ContextFactoryPtr factory;
	};
	
#define BSP_BEGIN(processors)	{					\
	bsp::TaskMapper tm (processors, factory);		\
	set_task_mapper(tm);							\
	bsp_cont = true;								\
	class MyRC : public bsp_context_t 		{		\
	public:											\
		void run () {								\
	try {											\
	ASSERT(mapper);									\
	if(mapper->jmp_buf( local_pid ).get() == NULL) {\
		boost::shared_ptr< 							\
			bsp::TaskMapper::jump_buffer> p ( 		\
			new bsp::TaskMapper::jump_buffer);		\
		mapper->jmp_buf( local_pid ) = p; 			\
		setjmp(p->env);								\
	} else { 										\
		longjmp( (mapper->jmp_buf( local_pid )->env ), 	\
			1 ); 									\
	}

#define BSP_END() 									\
	} catch (bsp::_bsp_sync b) {					\
		( (bsp::Computation<bsp_context_t> *) parentcontext )->bsp_cont = true; \
	} catch (bsp::_bsp_end e) {						\
		( (bsp::Computation<bsp_context_t> *) parentcontext )->bsp_cont = false; \
	}												\
		}											\
		static void run_as(Context * thiz) {		\
		( (MyRC*)thiz )->run();						\
		}											\
	};												\
	while (bsp_cont) {								\
		run_in_context(&MyRC::run_as);				\
		bsp_sync();									\
	}												\
}

	
/** BSP_SYNC remembers where we were. */
#define BSP_SYNC() 									\
	do {											\
		ASSERT(mapper->jmp_buf(local_pid).get() != 0); \
		if (!setjmp(mapper->jmp_buf(local_pid)->env)) { \
			throw bsp::_bsp_sync();					\
		}											\
	} while(0)
	
};

#endif
