/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __Context_H__
#define __Context_H__

#include <tbb/task.h>

#include "TaskMapper.h"

namespace bsp {

	/**
	 * BSP context. Subclass this to run parallel steps.
	 * 
	 * Implementation in bsp_context.cpp
	 */
	class Context {
	public:
		Context (TaskMapper * _mapper = NULL) : mapper(*_mapper), impl (NULL) {}

		virtual ~Context() {}

		/** 
		 * This will be called by the factory after construction
		 */ 
		void initialize_context (int bsp_pid, Context * );

		/** 
		 * This will be called by the factory when the context is destroyed
		 */ 
		void destroy_context ();

		/**
		 * This is called when the class is initialized by the factory after 
		 * construction.
		 */
		inline virtual void init () { }

		/**
		 * BSPWWW interface embedded in context.
		 */

		inline int bsp_nprocs() const {
			if (impl != NULL) { 
				return mapper.nprocs();
			} else {
				return ::bsp_nprocs();
			}
		}

		inline int bsp_pid() const {
			if (impl != NULL) { 
				return pid;
			} else {
				return ::bsp_pid();
			}
		}

		inline int bsp_local_pid() const {
			if (impl != NULL) { 
				return local_pid;
			} else {
				throw std::runtime_error("You cannot call bsp_local_pid at node level.");
			}
		}

		/** 
		 *  When this object was created through the context factory, 
		 *  bsp_sync() will throw an error, we need to use BSP_SYNC 
		 *  to split tasks up.
		 *  
		 *  @param local this will determine whether we call a global or local synchronization
		 *   
		 *  If this is called with local == true, then we synchronize only 
		 *  communication between context tasks.
		 *  
		 *  Otherwise, we assume that we are at node level and call 
		 *  ::bsp_sync()
		 */
		void bsp_sync( bool local = false );

		/** @name DRMA */
		/*@{*/
		void bsp_push_reg (const void *, size_t);
		void bsp_pop_reg (const void *);
		void bsp_put (int, const void *, void *, long int, size_t);
		void bsp_get (int, const void *, long int, void *, size_t);
		/*@}*/

		/** @name BSMP */
		/*@{*/
		void bsp_send (int, const void *, const void *, size_t);
		void bsp_qsize (int * , size_t * );
		void bsp_get_tag (int * , void * );
		void bsp_move (void *, size_t);
		void bsp_set_tagsize (size_t *);
		/*@}*/

		/** @name High Performance */
		/*@{*/
		void bsp_hpput (int, const void *, void *, long int, size_t);
		void bsp_hpget (int, const void *, long int, void *, size_t);
		int bsp_hpmove (void **, void **);
		/*@}*/

		/** We store the parent context, and add this function
		 * to retrieve it later. May return NULL for top-level contexts.
		 */
		inline bsp::Context * get_parent_context() {
			return parentcontext;
		}

		/************************************************************************/
		/* Helpers for context injection into a scope, and generic creation     */
		/************************************************************************/

		/** FUN is a function taking a Context* as its parameter */
		typedef void (*FUN)(Context *);

		/** Execute will run the runme function with this as it's argument */
		inline void execute () {
			runme (this);
		}

		FUN runme;				/** We replace this when we want to run a derived 
								    class's run method on data stored in this context.
									Remember, you can't add any new data members in 
									such derived classes, the mechanism here only
									allows to inject data and inherited things upwards.

									See below for declaration of run_as, which 
									can be used to initialize runme using subclassed
									objects.

									*/

		/** This function executes _runme in all contexts in our mapper */
		inline void run_in_context ( Context::FUN _runme ) {
			for (int j = 0;	j < mapper.procs_this_node(); ++j ) {
				mapper.get_context (j).runme = _runme;
			}
			ComputationTask & root = *new( tbb::task::allocate_root() ) 
				ComputationTask ( mapper );
			tbb::task::spawn_root_and_wait (root);
		}

		/** impl helper */
		inline void * get_impl() { return impl; }

		inline void set_task_mapper(TaskMapper & _mapper) {
			mapper = _mapper;
		}

		/** TBB Task to run a context in a mapper */
		class ComputationTask : public tbb::task {
		public:
			ComputationTask( TaskMapper & _mapper, int _pid = -1 ) : 
			  mapper (_mapper), my_pid (_pid) {}

			  tbb::task * execute() {
				  if (my_pid < 0) {
					  tbb::task_list tl;

					  for (int t = 0, e=mapper.procs_this_node(); t < e; ++t) {
						  ComputationTask & tsk = *new ( allocate_child() ) 
							  ComputationTask ( mapper, t );
						  tl.push_back(tsk);
					  }
					  set_ref_count( mapper.procs_this_node() + 1 );
					  spawn_and_wait_for_all(tl);
				  } else {
					  mapper.get_context(my_pid).execute();
				  }
				  return NULL;
			  }
		private:
			TaskMapper & mapper;
			int my_pid;
		};


	private:
		Context * parentcontext;	///< this is the parent context 
		TaskMapper&  mapper;		///< The process mapper object for this context
		int pid;		///< The global pid of this computation
		int local_pid;	///< The local pid of this computation

		void * impl;    ///< Implementation specific stuff
	};

	/**
	 * Factory template which passes TaskMapper, pid and parent context 
	 * to the constructor of a given type, and casts the created pointer
	 * to Context *.
	 */

	template <class runnablecontext> 
	class ContextFactory : public AbstractContextFactory {
	public:
		ContextFactory ( Context * _parent ) : 
			parent (_parent) {}

		inline Context * create ( TaskMapper & mapper, int bsp_pid ) {
			Context * p = new runnablecontext (&mapper);
			p->initialize_context( bsp_pid, parent );
			return p;
		}

		inline void destroy ( Context * t ) {
			t->destroy_context();
			delete (runnablecontext * ) t;
		}

	private:
		Context * parent;
	};

};


#endif // __Context_H__
