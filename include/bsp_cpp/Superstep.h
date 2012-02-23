/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef bsp_cpp_superstep_h__
#define bsp_cpp_superstep_h__

#include <string>
#include <string.h>

#ifndef ASSERT
#define ASSERT(x)
#endif

#include <tbb/task.h>
#include <tbb/mutex.h>
#include <tbb/atomic.h>
#include <tbb/tbb_thread.h>

#include "bsp_global_drma.h"

#include "bsp_tools/singletons.h"
#include "ParameterFile.h"
#include "ProcMapper.h"
#include "Context.h"

namespace bsp {

	/**
	 * @brief This class runs a specific computation pid in a tbb task
	 */
	template <class _computation>
	class ComputationTask : public tbb::task {
	public:
		ComputationTask(_computation * __computation, bool _destroy = false) : 
			computation(__computation), destroy (_destroy) {}

		tbb::task * execute() {
			computation->run();
			if (destroy) {
				delete computation;
			}
			return NULL;
		}
		bool destroy;
		_computation * computation;
	};

	/**
	 * @brief main BSP computation class
	 * 
	 */
	template <class procmapper_t>
	class Superstep {
	public:
		typedef Superstep < procmapper_t > my_type;

		/************************************************************************/
		/* Use this constructor on the node level                               */
		/************************************************************************/

		Superstep(procmapper_t & c, int _local_pid = 0) : procmapper (c) {
			using namespace singletons;
			local_pid = _local_pid;
			firstone = false;
			if (!root_task) {
				firstone = true;
				root_task = new (tbb::task::allocate_root()) tbb::empty_task;
			}
		}

	private:

		/************************************************************************/
		/* We use this constructor to make copies for running using TBB         */
		/************************************************************************/

		Superstep(Superstep const & c, int _local_pid) : procmapper(c.procmapper)
		{
			using namespace singletons;
			firstone = false;
			if (!root_task) {
				firstone = true;
				singletons::root_task = new (tbb::task::allocate_root()) tbb::empty_task;
			}
			my_root = 0;
			local_pid = _local_pid;
		}

	public:

		void start() {
			ComputationTask<my_type> *tsk = NULL;
			int nprocs = procmapper.procs_this_node();
			singletons::root_task-> set_ref_count(nprocs+1);
			// normally, we create a root task which will create child tasks for
			// all the other local processors

			my_root = new ( singletons::root_task->allocate_child() ) 
				ComputationTask<my_type>(new my_type(*this, 0), true);
			singletons::root_task->spawn (*my_root);

			for (int t = 1; t < nprocs; ++t) {
				tsk = new ( my_root->allocate_child() ) 
					ComputationTask<my_type>(new my_type(*this, t), true);
				my_root->spawn(*tsk);
			}
		}

		void join() {
			if (my_root != NULL) {
				my_root->wait_for_all();
				singletons::root_task->destroy (*my_root);
				my_root = NULL;				
			}
		}
		
		void run () {
			typename procmapper_t::context_t * ctx = procmapper.get_context(local_pid);
			ctx->run();
		}

		bool is_root() {
			return procmapper.bsp_local_pid() == 0;
		}

	private:
		bool firstone; ///< indicates whether we are the first task
		int local_pid; ///< we save the local pid this job actually runs
		procmapper_t & procmapper; ///< the procmapper object which stores all the contexts
		tbb::task * my_root;
	};

};


#endif // bsp_cpp_superstep_h__
