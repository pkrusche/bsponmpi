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
	 * Simple new/delete factory for a context running on local_pid.
	 * We will pass the pointer to the procmapper in param as 
	 * a void pointer to avoid a class declaration cycle.
	 */
	template < typename _t >
	class AbstractFactory {
	public:

		typedef _t context_t;

		virtual _t * create ( int local_pid , void * param ) = 0;

		virtual void destroy ( _t * t) = 0;
	};

	/**
	 * @brief Simple blocked assignment of pids to processors 
	 */
	template <class _contextfactory>
	class ProcMapper {
	public:
		typedef typename _contextfactory::context_t context_t;

		ProcMapper(_contextfactory * factory, 
			int _processors, context_t * _parent_context = NULL) : 
		  processors (_processors ){
			using namespace std;
			max_procs_per_node = ICD(processors, ::bsp_nprocs());
			procs_per_group = _processors;
			if( max_procs_per_node * (::bsp_pid()+1) > processors ) {
				procs_on_this_node =  
					max(processors - max_procs_per_node * (signed) ::bsp_pid(), 0);
			} else {
				procs_on_this_node = max_procs_per_node;
			}

			if (!_parent_context) {
				destroy_parent_context = true;
				parent_context = factory->create( -1, this );
			} else {
				destroy_parent_context = false;
				parent_context = _parent_context;
			}

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
			if (destroy_parent_context) {
				contextfactory->destroy ( parent_context );
			}
		}

		int nprocs () const {
			return processors;
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
		
		context_t * get_parent_context() {
			return parent_context;
		}

	private:
		int processors;
		int procs_on_this_node;
		int max_procs_per_node;
		int procs_per_group;
		
		_contextfactory * contextfactory;
		context_t * parent_context;
		bool destroy_parent_context;

		tbb::concurrent_vector< context_t * > context_store;

	};
};

#endif // ProcMapper_h__
