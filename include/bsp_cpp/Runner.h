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
				tbb::task_list tl;

				for (int t = 0, e=mapper->procs_this_node(); t < e; ++t) {
					ComputationTask & tsk = *new ( allocate_child() ) 
						ComputationTask ( mapper, t );
					tl.push_back(tsk);
				}
				set_ref_count( mapper->procs_this_node() + 1 );
				spawn_and_wait_for_all(tl);
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
	inline void run_context ( TaskMapper * mapper, Context::FUN _runme ) {
		for (int j = 0;	j < mapper->procs_this_node(); ++j ) {
			mapper->get_context (j).runme = _runme;
		}
		ComputationTask & root = *new( tbb::task::allocate_root() ) 
			ComputationTask ( mapper );
		tbb::task::spawn_root_and_wait (root);
	}

};

#endif // __Runner_H__
