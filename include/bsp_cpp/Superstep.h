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
	template <class procmapper_t>
	class ComputationTask : public tbb::task {
	public:
		ComputationTask( procmapper_t & _mapper ) : 
			mapper (_mapper) {}

		tbb::task * execute() {
			computation->run();
			if (destroy) {
				delete computation;
			}
			return NULL;
		}
	private:
		procmapper_t & mapper;
	};


	template <class _procmapper_t>
	void run_superstep(_procmapper_t & mapper) {
		using namespace tbb;
		int nprocs = procmapper.procs_this_node();
		// normally, we create a root task which will create child tasks for
		// all the other local processors

		ComputationTask<my_type> & root = *new( task::allocate_root() ) ComputationTask(mapper, false);
		singletons::root_task->spawn (*my_root);
		root.set_ref_count(nprocs+1);

		for (int t = 1; t < nprocs; ++t) {
			tsk = new ( my_root->allocate_child() ) 
				ComputationTask<my_type>(new my_type(*this, t), true);
			my_root->spawn(*tsk);
		}
		if (my_root != NULL) {
			my_root->wait_for_all();
			singletons::root_task->destroy (*my_root);
			my_root = NULL;				
		}
	}
	

};


#endif // bsp_cpp_superstep_h__
