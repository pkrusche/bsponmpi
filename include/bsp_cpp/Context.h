/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __Context_H__
#define __Context_H__

#include "TaskMapper.h"

namespace bsp {

	/**
	 * BSP context. Subclass this to run parallel steps.
	 * 
	 * Implementation in bsp_context.cpp
	 */
	class Context {
	public:
		Context () : impl (NULL) {}

		/** 
		 * This will be called by the factory after construction
		 */ 
		void initialize_context (TaskMapper * tm, int bsp_pid, Context * parent = NULL);

		/** 
		 * This will be called by the factory when the context is destroyed
		 */ 
		void destroy_context ();

		/**
		 * This is called when the class is inialized by the factory after 
		 * construction.
		 */
		inline virtual void init () { }

		/**
		 * BSPWWW interface embedded in context.
		 */

		inline int bsp_nprocs() const {
			if (impl != NULL) { 
				return mapper->nprocs();
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
		 *  Otherwise, we assume that we are at node level and call 
		 *  ::bsp_sync()
		 */
		inline void bsp_sync() const {
			if (impl != NULL) {
				throw std::runtime_error("When syncing in a Context, BSP_SYNC needs to be used rather than bsp_sync()");
			} else {
				::bsp_sync();
			}
		}

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
		void bsp_qsize (int * RESTRICT , size_t * RESTRICT );
		void bsp_get_tag (int * RESTRICT , void * RESTRICT );
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
			ASSERT(runme != NULL);
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

	private:
		Context * parentcontext;	///< this is the parent context 
		TaskMapper * mapper;  ///< The process mapper object
		int pid;		///< The global pid of this computation
		int local_pid;	///< The local pid of this computation
		void * impl;    ///< Implementation specific stuff
	};


	/**
	 * 
	 * Function to allow running different superstep's code on the 
	 * same data.
	 *
	 * as_class must be subclassed from Context and 
	 * have a member function run();
	 */
	template <class as> 
	static inline void run_context_as (Context * thiz) {
		((as *) thiz)->run();
	}

	/**
	 * Factory template which passes TaskMapper, pid and parent context 
	 * to the constructor of a given type, and casts the created pointer
	 * to Context *.
	 */

	template <class runnablecontext> 
	class ContextFactory : public AbstractContextFactory {
	public:
		inline Context * create ( TaskMapper * tm, int bsp_pid, Context * parent) {
			Context * p = new runnablecontext();
			p->initialize_context(tm, bsp_pid, parent);
			return p;
		}

		inline void destroy ( Context * t ) {
			t->destroy_context();
			delete (runnablecontext * ) t;
		}
	};

};


#endif // __Context_H__
