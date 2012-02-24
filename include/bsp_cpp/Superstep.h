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
		ComputationTask( procmapper_t & _mapper, int _my_local_pid = -1 ) : 
			mapper (_mapper), my_local_pid (_my_local_pid) {}

		tbb::task * execute() {
			if (my_local_pid < 0) {
				set_ref_count(mapper.procs_this_node()+1);
				for (int t = 0; t < mapper.procs_this_node(); ++t) {
					ComputationTask<procmapper_t> & tsk = *new ( allocate_child() ) 
						ComputationTask<procmapper_t>( mapper, t );
					spawn(tsk);
				}
				wait_for_all();
			} else {
				mapper.get_context(my_local_pid)->run();				
			}
			return NULL;
		}
	private:
		procmapper_t & mapper;
		int my_local_pid;
	};


	template <class _procmapper_t>
	void run_superstep(_procmapper_t & mapper) {
		using namespace tbb;
		
		// we create a root task which will create child tasks for
		// all the other local processors

		ComputationTask<_procmapper_t> & root = *new( task::allocate_root() ) 
			ComputationTask<_procmapper_t>( mapper );
		task::spawn_root_and_wait (root);
	}
	

};


#endif // bsp_cpp_superstep_h__
