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

#ifndef __BSP_RUNNER_H__
#define __BSP_RUNNER_H__

#include "Context.h"
#include "TaskMapper.h"

#include <tbb/task.h>
#include <tbb/task_scheduler_init.h>

namespace bsp {

	class ComputationTask : public tbb::task {
	public:
		ComputationTask( Context * _ctx ) : 
		  ctx(_ctx) {}

		  tbb::task * execute() {
			  ctx->execute_step ();
			  return NULL;
		  }

	private:
		Context * ctx;
	};

	/** TBB Task to run a context in a mapper */
	class ComputationSpawnTask : public tbb::task {
	public:
		ComputationSpawnTask( TaskMapper * _mapper ) : mapper (_mapper) {}

		tbb::task * execute() {
			int e = mapper->procs_this_node();

			tbb::task_list tl;

			for (int t = 0; t < e; ++t) {
				ComputationTask & tsk = *new ( allocate_child() ) 
					ComputationTask ( mapper->get_context(t) );
				tl.push_back(tsk);
			}

			set_ref_count( e + 1 );
			spawn_and_wait_for_all(tl);
			
			return NULL;
		}
	private:
		TaskMapper * mapper;
	};


	/** BSP Computation runner.
	 * 
	 * Given a BSP computation defined in class _context, this runner will set
	 * up a task manager and run the computation.
	 * 
	 * Optionally, the runner can choose a (presumably) optimal number of processors,
	 * by default, this is assumed to be the number of MPI processes times TBB's 
	 * default number of threads.
	 * 
	 */
	template <class _context>
	class Runner : public _context {
	public:
		typedef _context bsp_context_t;

		/** Create a runner
		 * 
		 * @param processors The number of processors (optional)
		 */
		Runner (int processors = -1)  {
			// automatic number of processors
			if (processors < 0) {
				processors = tbb::task_scheduler_init::default_num_threads() * ::bsp_nprocs();
			}

			factory = ContextFactoryPtr (
				new ContextFactory< bsp_context_t >
				(this) );
			_context::set_task_mapper ( new bsp::TaskMapper (processors, factory) );
		}

		/** Destructor: destroy task mapper */
		~Runner () {
			delete _context::get_mapper();
		}

		/** Run a computation, preceded by variable initialisation,
		 *  followed by variable reducing */
		void run( int master_node = 0 ) {
			ASSERT (this->bsp_is_node_level());

			try {
				_context::parentcontext = this;
				this->initialize_shared ( master_node );

				bsp_context_t::run();

				this->reduce_shared ();
			} catch ( tbb::captured_exception e ) {
				throw std::runtime_error (e.what());
			}
		}

		/**
		 * Run a computation's superstep
		 */ 
		void execute () {
			ASSERT (this->bsp_is_node_level());

			if (_context::mapper->procs_this_node() > 1) {
				ComputationSpawnTask & root = *new( tbb::task::allocate_root() ) 
					ComputationSpawnTask ( _context::mapper );

				tbb::task::spawn_root_and_wait (root);
			} else if (_context::mapper->procs_this_node() == 1) {
				// only one process? don't bother with tbb!
				for(int k = 0; k < _context::mapper->procs_this_node(); ++k) {
					_context::mapper->get_context(k)->execute_step();
				}
			} 
		}

	protected:
		ContextFactoryPtr factory;
	};

};

#define BSP_SCOPE(context)	typedef context bsp_context_t


#define BSP_BEGIN()		{								\
	struct __R : public bsp_context_t {					\
		void exec () {									


#define BSP_END() 										\
		}												\
		static void runme(bsp::Context * thiz) {		\
			((__R*)thiz)->exec();						\
		}												\
	};													\
	get_mapper()->set_next_step(&__R::runme);			\
	dynamic_cast<bsp::Runner<bsp_context_t>*>(			\
		get_parent_context())->execute();				\
	bsp_sync();											\
}



/** BSP_SYNC remembers where we were. */
#define BSP_SYNC() BSP_END() BSP_BEGIN() 

#endif
