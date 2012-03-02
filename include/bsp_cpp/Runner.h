/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __Runner_H__
#define __Runner_H__

#include <tbb/task.h>

namespace bsp {

	/** TBB Task to run a context in a mapper */
	class ComputationTask : public tbb::task {
	public:
		ComputationTask( TaskMapper * _mapper, int _pid = -1 ) : 
		  mapper (_mapper), my_pid (_pid) {}

		  tbb::task * execute() {
			  if (my_pid < 0) {
				  set_ref_count(mapper->procs_this_node()+1);
				  for (int t = 0, e=mapper->procs_this_node(); t < e; ++t) {
					  ComputationTask & tsk = *new ( allocate_child() ) 
						  ComputationTask ( mapper, t );
					  spawn(tsk);
				  }
				  wait_for_all();
			  } else {
				  mapper->get_context(my_pid).execute();
			  }
			  return NULL;
		  }
	private:
		TaskMapper * mapper;
		int my_pid;
	};

	/**
	 * Process runner. 
	 * 
	 * Takes a (locally) subclass of Context and runs its
	 * run() function on all Context's stored in the processor
	 * mapper.
	 * 
	 */
	template <class ctx> 
	void run_context ( TaskMapper * mapper ) {
		for (int j = 0;	j < mapper->procs_this_node(); ++j ) {
			mapper->get_context (j).runme = &run_context_as<ctx>;
		}
		ComputationTask & root = *new( tbb::task::allocate_root() ) 
			ComputationTask ( mapper );
		tbb::task::spawn_root_and_wait (root);
	}

};

#endif // __Runner_H__
