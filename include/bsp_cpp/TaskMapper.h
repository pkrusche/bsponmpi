/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef ProcMapper_h__
#define ProcMapper_h__

#include <algorithm>

#include <tbb/concurrent_vector.h>

#include <boost/shared_ptr.hpp>

#include "bsp.h"
#include "bsp_tools/utilities.h"

#include "Context.h"

namespace bsp {


	/**
	 * Basic balanced task mapper which works for tasks of approximately 
	 * the same execution time. Tasks are distributed in equal numbers 
	 * across all nodes.
	 * 
	 */
	class TaskMapper {
	public:
		TaskMapper (int _processors, 
			AbstractContextFactory * factory
		) : 
			processors (_processors) {
			max_procs_per_node = ICD(processors, ::bsp_nprocs());
			if( max_procs_per_node * (::bsp_pid()+1) > processors ) {
				procs_on_this_node =  
					max(processors - max_procs_per_node * (signed) ::bsp_pid(), 0);
			} else {
				procs_on_this_node = max_procs_per_node;
			}
		}

		/**
		 * Number of processors in this mapper
		 */

		int nprocs () const {
			return processors;
		}

		/**
		 * Number of maximum processors per node
		 */

		int procs_per_node() const {
			return max_procs_per_node;
		}

		/**
		 * Number of processors on this node
		 */

		int procs_this_node() const {
			return procs_on_this_node;
		}

		/**
		 * Take a local processor id, translate to global id
		 */

		int local_to_global_pid(int local_pid) const {
			return ::bsp_pid() * max_procs_per_node + local_pid;
		}

		/**
		 * Take a global processor, translate to local
		 * 
		 * Returns -1 if the global pid given is run on a different
		 * node.
		 */

		int global_to_local_pid(int global_pid) const {
			int p = global_pid - ::bsp_pid() * procs_per_node();
			if (p < 0 || p >= procs_on_this_node)	{
				return -1;
			} else {
				return p;
			}
		}



	private:
		std::vector<context_t *> context_store;
		AbstractContextFactory * contextfactory;

		int processors;
		int procs_on_this_node;
		int max_procs_per_node;
	};

	/**
	 * Processor mapper. The processor mapper manages superstep contexts using 
	 * a task mapper which gives an assignment of tasks to processors.
	 * 
	 * The template parameter contextfactory must produce context objects
	 * which 
	 * 
	 */
	template <class _contextfactory, class _taskmapper = TaskMapper>
	class ProcMapper {
	public:
		typedef ProcMapper<_contextfactory, _taskmapper> my_type;
		typedef typename _contextfactory::context_t context_t;

		ProcMapper() : 
		  processors (_processors ) {
			using namespace std;

			context_store.resize(procs_on_this_node);
			for (size_t j = 0; j < procs_on_this_node; ++j) {
				context_store[j] = factory->create( (int)j, this );
			}
			contextfactory = factory;
		}

		virtual ~ProcMapper () {
			for (size_t j = 0; j < context_store.size(); ++j) {
				contextfactory->destroy ( context_store[j] );
			}
		}
		
		/**
		 * Split 
		 */
		void split ( int parent, int _processors, _contextfactory * factory ) {
			bsp_level l;
			l.context_store.
		}

		/**
		 * Combine processors from a previous split level, return 
		 */
		void combine () {

		}

		/**
		 * Run a superstep at the current call level.
		 */
		void superstep() {
			using namespace tbb;

			if (task_hierarchy)

			class ComputationTask : public tbb::task {
			public:
				ComputationTask( my_type & _mapper, int pid = -1 ) : 
					mapper (_mapper), my_pid (_pid) {}

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
				my_type & mapper;
				int my_pid;
			};


			// we create a root task which will create child tasks for
			// all the other local processors

			ComputationTask & root = *new( task::allocate_root() ) 
				ComputationTask ( mapper );
			
			task::spawn_root_and_wait (root);
			bsp_sync();
		}


	private:

		struct bsp_level {
			bsp_level (int processors, _contextfactory * factory) : 
				mapper ( new _taskmapper (processors) ),
				contextfactory (factory) {

				context_store.resize(mapper->procs_this_node());
				for (int j = 0, k = context_store.size(); j < k; ++j) {
					context_store[j] = contextfactory->create (  );
				}
			}

			boost::shared_ptr<_taskmapper> mapper;
		};

		std::stack<bsp_level> task_hierarchy;
		
	};
};

#endif // ProcMapper_h__
