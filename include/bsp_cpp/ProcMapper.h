/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef ProcMapper_h__
#define ProcMapper_h__

#include <algorithm>
#include <tbb/concurrent_vector.h>

#include "bsp.h"
#include "bsp_tools/utilities.h"

namespace bsp {

	/**
	 * Simple new/delete factory for a single value
	 */
	template < typename _t >
	class SimpleFactory {
	public:

		typedef _t context_t;

		virtual _t * create ( int /* local_pid */, void * /* param */ ) {
			return new _t;
		}

		virtual void destroy ( _t * t) {
			delete t;
		}
	};

	/**
	 * @brief Simple blocked assignment of pids to processors 
	 */
	template <class _contextfactory>
	class ProcMapper {
	public:
		typedef typename _contextfactory::context_t context_t;

		ProcMapper(_contextfactory * factory, int _processors, int _groups = 1) : 
		  processors(_processors*_groups), groups(_groups) {
			using namespace std;
			max_procs_per_node = ICD(processors, ::bsp_nprocs());
			procs_per_group = _processors;
			if( max_procs_per_node * (::bsp_pid()+1) > processors ) {
				procs_on_this_node =  
					max(processors - max_procs_per_node * (signed) ::bsp_pid(), 0);
			} else {
				procs_on_this_node = max_procs_per_node;
			}

			node_context = factory->create( -1, this );

			context_store.resize(procs_on_this_node);
			for (size_t j = 0; j < procs_on_this_node; ++j) {
				context_store[j] = factory->create( (int)j, this );
			}
			contextfactory = factory;
		}

		virtual ~ProcMapper () {
			for (size_t j = 0; j < procs_on_this_node; ++j) {
				contextfactory->destroy ( context_store[j] );
			}
			contextfactory->destroy ( node_context );
		}

		int nprocs () const {
			return processors;
		}

		int ngroups() const {
			return groups;
		}

		int procs_per_node() const {
			return max_procs_per_node;
		}

		int procs_this_node() const {
			return procs_on_this_node;
		}

		int local_to_global_pid(int local_pid) const {
			return ::bsp_pid() * max_procs_per_node + local_pid;
		}

		int local_to_global_group(int local_pid) const {
			return ::bsp_pid() * max_procs_per_node + local_pid;
		}

		int global_to_local_pid(int global_pid, int group = 0) const {
			int p = global_pid - ::bsp_pid() * procs_per_node();
			if (p < 0 || p >= procs_on_this_node)	{
				return -1;
			} else {
				return p;
			}
		}

		context_t * get_context(int local_pid) {
			return context_store[local_pid];
		}
		
		context_t * get_node_context() {
			return node_context;
		}

	private:
		int processors;
		int procs_on_this_node;
		int max_procs_per_node;
		int groups;
		int procs_per_group;
		
		_contextfactory * contextfactory;
		tbb::concurrent_vector< context_t * > context_store;
		context_t * node_context;
	};
};

#endif // ProcMapper_h__
