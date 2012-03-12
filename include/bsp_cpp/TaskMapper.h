/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef TaskMapper_h__
#define TaskMapper_h__

#include <algorithm>
#include <vector>

#include <setjmp.h>

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
	 * from the parent context.
	 * 
	 */
	class AbstractContextFactory {
	public:
		virtual ~AbstractContextFactory() {}

		/**
		 * create a context for a given bsp pid
		 */
		virtual Context * create ( TaskMapper * , int bsp_pid ) = 0;

		/**
		 * destroy a context
		 */
		virtual void destroy ( Context * t ) = 0;

		/**
		 * get the parent context associated with this factory
		 */
		virtual Context * get_parent () = 0;
	};

	/** Shortcut to shared pointer */
	typedef boost::shared_ptr<AbstractContextFactory> ContextFactoryPtr;

	/**
	 * Basic balanced task mapper which works for tasks of approximately 
	 * the same execution time. Tasks are distributed in equal numbers 
	 * across all nodes.
	 * 
	 */
	class TaskMapper {
	public:
		TaskMapper (int _processors, 
			ContextFactoryPtr factory
		) : contextfactory(factory),
			processors (_processors), step(NULL) {
			using namespace std;

			where_is_node = new int [processors];
			where_is_local = new int [processors];
			where_is_local_here = new int [processors];
			
			procs_on_node = new int [::bsp_nprocs()];
			memset(procs_on_node, 0, sizeof(int) * ::bsp_nprocs());

			int my_node = ::bsp_pid();
			procs_on_this_node = 0;
			max_procs_per_node = 0;
			for (int p = 0; p < processors; ++p) {
				int n;
				int lp;
				where_is(processors, p, n, lp);
				where_is_node[p] = n;
				where_is_local[p] = lp;
				++procs_on_node[n];
				max_procs_per_node = max(max_procs_per_node, procs_on_node[n]);

				if (n == my_node) {
					where_is_local_here[p] = lp;
					++procs_on_this_node;
				} else {
					where_is_local_here[p] = -1;
				}
			}

			which_global = new int [bsp_nprocs() * max_procs_per_node];
			memset(which_global, -1, sizeof(int) * (bsp_nprocs() * max_procs_per_node));
			for (int p = 0; p < processors; ++p) {
				int n = where_is_node[p];
				int lp = where_is_local[p];
				which_global[n*max_procs_per_node + lp] = p;
			}

			// we create at least one context. We might not use this context,
			// but the context implementation must need to be called at least
			// once on every node to create static stuff
			using namespace std;
			context_store.resize( procs_on_this_node );
			for (int i = 0, i_end = (int)context_store.size(); i < i_end; ++i ) {
				context_store[i] = factory->create( this, local_to_global_pid(i) );
			}
		}

		virtual ~TaskMapper () {
			for (size_t i = 0, i_end = context_store.size(); i < i_end; ++i ) {
				contextfactory->destroy(context_store[i]);
			}
			delete [] where_is_node;
			delete [] where_is_local;
			delete [] where_is_local_here;
			delete [] procs_on_node;
			delete [] which_global;
		}

		/**
		 * Number of processors in this mapper
		 */

		inline int nprocs () const {
			return processors;
		}

		/**
		 * Number of maximum processors per node
		 */

		inline int procs_per_node() const {
			return max_procs_per_node;
		}

		/**
		 * Number of processors on this node
		 */

		inline int procs_this_node() const {
			return procs_on_this_node;
		}

		/**
		 * Take a local processor id, translate to global id
		 */

		inline int local_to_global_pid(int local_pid) const {
			return which_global[max_procs_per_node * ::bsp_pid() + local_pid];
		}


		/**
		 * Take a local processor id and node id, translate to global id
		 */

		inline int local_to_global_pid(int node, int local_pid) const {
			return which_global[max_procs_per_node * node + local_pid];
		}

		/**
		 * Take a global processor, translate to local
		 * 
		 * Returns -1 if the global pid given is run on a different
		 * node.
		 */

		inline int global_to_local_pid(int global_pid) const {
			return where_is_local_here[global_pid];
		}

		/**
		 * Get the context for a local process
		 */
		inline Context * get_context(int local_pid) {
			return context_store[local_pid];
		}

		/** On which node is a given logical processor running */
		inline const int & global_to_node (int global_pid) const {
			return where_is_node[global_pid];
		}

		/** What is the local pid of a logical processor */
		inline const int & global_to_local (int global_pid) const {
			return where_is_local[global_pid];
		}

		/** Running helper */
		typedef void (*CONTEXTRUNNER)(Context *);

		void set_next_step (CONTEXTRUNNER cr) {
			step = cr;
		}

		CONTEXTRUNNER get_next_step () {
			return step;
		}

	protected:
		/** 
		 * Find out where a given global processor context is held. 
		 */
		virtual void where_is (int processors, int global_pid, int & node, int & local_pid) {
			int mppn = ICD (processors, ::bsp_nprocs());

			node = global_pid / mppn;
			local_pid = global_pid - node * mppn;
		}

	private:
		CONTEXTRUNNER step;

		std::vector<Context *> context_store;
		ContextFactoryPtr contextfactory;

		int * where_is_node;		///< which node contains logical processor p
		int * where_is_local;		///< which context contains logical processor p
		int * where_is_local_here;	///< on this node, which context contains logical processor p (-1 when remote)
		int * procs_on_node;		///< how many processors on given node
		int * which_global;			///< which global processor is represented by local pid 

		int processors;				///< overall number of logical processors
		int procs_on_this_node;		///< how many logical processors on our node
		int max_procs_per_node;		///< max over procs_on_node
	};

};

#endif 

