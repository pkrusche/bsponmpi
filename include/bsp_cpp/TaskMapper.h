/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef TaskMapper_h__
#define TaskMapper_h__

#include <algorithm>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "bsp.h"
#include "bsp_tools/utilities.h"

namespace bsp {

	class TaskMapper;
	class Context;

	/**
	 * Factory interface for creating BSP contexts.
	 * 
	 * Override this for your context class.
	 * 
	 * Subclassed objects need a default constructor, initialisation
	 * can be performed by overriding init () and reading data
	 * e.g. from the parent context.
	 * 
	 */
	class AbstractContextFactory {
	public:
		/**
		 * create a context for a given bsp pid
		 */
		virtual Context * create ( TaskMapper *, int bsp_pid, Context * ) = 0;

		/**
		 * destroy a context
		 */
		virtual void destroy ( Context * t ) = 0;
	};



	/**
	 * Basic balanced task mapper which works for tasks of approximately 
	 * the same execution time. Tasks are distributed in equal numbers 
	 * across all nodes.
	 * 
	 */
	class TaskMapper {
	public:
		TaskMapper (int _processors, 
			AbstractContextFactory * factory,
			Context * parent = NULL
		) : contextfactory(factory),
			processors (_processors) {
			max_procs_per_node = ICD(processors, ::bsp_nprocs());
			if( max_procs_per_node * (::bsp_pid()+1) > processors ) {
				procs_on_this_node =  
					max(processors - max_procs_per_node * (signed) ::bsp_pid(), 0);
			} else {
				procs_on_this_node = max_procs_per_node;
			}

			context_store.resize(procs_on_this_node);			
			for (int i = 0, i_end = (int)context_store.size(); i < i_end; ++i ) {
				context_store[i] = factory->create( this, local_to_global_pid(i), parent );
			}
		}

		virtual ~TaskMapper () {
			for (size_t i = 0, i_end = context_store.size(); i < i_end; ++i ) {
				contextfactory->destroy(context_store[i]);
			}
		}

		/**
		 * Number of processors in this mapper
		 */

		virtual int nprocs () const {
			return processors;
		}

		/**
		 * Number of maximum processors per node
		 */

		virtual int procs_per_node() const {
			return max_procs_per_node;
		}

		/**
		 * Number of processors on this node
		 */

		virtual int procs_this_node() const {
			return procs_on_this_node;
		}

		/**
		 * Take a local processor id, translate to global id
		 */

		virtual int local_to_global_pid(int local_pid) const {
			return ::bsp_pid() * max_procs_per_node + local_pid;
		}


		/**
		 * Take a local processor id and node id, translate to global id
		 */

		virtual int local_to_global_pid(int node, int local_pid) const {
			return node * max_procs_per_node + local_pid;
		}

		/**
		 * Take a global processor, translate to local
		 * 
		 * Returns -1 if the global pid given is run on a different
		 * node.
		 */

		virtual int global_to_local_pid(int global_pid) const {
			int p = global_pid - ::bsp_pid() * procs_per_node();
			if (p < 0 || p >= procs_on_this_node)	{
				return -1;
			} else {
				return p;
			}
		}

		/**
		 * Get the context for a local process
		 */
		virtual Context & get_context(int local_pid) {
			return *(context_store[local_pid]);
		}

		/** 
		 * Find out where a given global processor context is held. 
		 */
		virtual void where_is (int global_pid, int & node, int & local_pid) {
			node = global_pid / max_procs_per_node;
			local_pid = global_pid - node * max_procs_per_node;
		}

	private:
		std::vector<Context *> context_store;
		AbstractContextFactory * contextfactory;

		int processors;
		int procs_on_this_node;
		int max_procs_per_node;
	};
};

#endif 

