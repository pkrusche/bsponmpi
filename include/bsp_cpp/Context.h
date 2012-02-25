/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __Context_H__
#define __Context_H__

namespace bsp {

	template <class procmapper_t>
	class BSPContext {
	public:

		BSPContext (procmapper_t & pm, int _local_pid = 0) : procmapper(pm) {
			local_pid = _local_pid;
			pid = procmapper.local_to_global_pid(local_pid);
		}

		procmapper_t & get_procmapper() {
			return procmapper;
		}

		int procs_this_node () {
			return procmapper.procs_this_node();
		}

		int bsp_nprocs() const {
			return procmapper.nprocs();
		}

		int bsp_pid() const {
			return pid;
		}

		int bsp_local_pid() const {
			return local_pid;
		}

		void bsp_global_get(bsp_global_handle_t src, size_t offset, 
			void * dest, size_t size) {
				tbb::mutex::scoped_lock l(singletons::global_mutex);
				::bsp_global_get(src, offset, dest, size);
		}

		void bsp_global_put(const void * src, bsp_global_handle_t dest, 
			size_t offset, size_t size) {
				tbb::mutex::scoped_lock l(singletons::global_mutex);
				::bsp_global_put(src, dest, offset, size);
		}

		void bsp_global_hpget(bsp_global_handle_t src, size_t offset, 
			void * dest, size_t size) {
				tbb::mutex::scoped_lock l(singletons::global_mutex);
				::bsp_global_hpget(src, offset, dest, size);
		}

		void bsp_global_hpput(const void * src, bsp_global_handle_t dest, 
			size_t offset, size_t size) {
				tbb::mutex::scoped_lock l(singletons::global_mutex);
				::bsp_global_hpput(src, dest, offset, size);
		}

		typename procmapper_t::context_t * bsp_context () {
			return procmapper.get_context(local_pid);
		}

		typename procmapper_t::context_t * bsp_parent_context () {
			return procmapper.get_parent_context();
		}

		int bsp_call_level () {
			if (local_pid >= 0) {
				return procmapper.get_parent_context()->bsp_call_level() + 1;
			} else {
				return 0;
			}
		}

	private:
		procmapper_t & procmapper;  ///< The process mapper object
		int pid; ///< The global pid of this computation
		int local_pid; ///< The local pid of this computation
	};

};


#endif // __Context_H__
