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
		/** 
		 * This will be called by the factory after construction
		 */ 
		void initialize_context (TaskMapper * tm, int bsp_pid, Context * parent = NULL)  {
			mapper = tm;
			parentcontext = parent;
			local_pid = tm->global_to_local_pid(bsp_pid);
			pid = bsp_pid;
			runme = NULL;	// this really needs to be initialized
			init ();
		}

		/**
		 * This is called when the class is inialized by the factory after 
		 * construction.
		 */
		virtual void init () {

		}

		/**
		 * BSPWWW interface embedded in context.
		 */

		int bsp_nprocs() const {
			return mapper->nprocs();
		}

		int bsp_pid() const {
			return pid;
		}

		int bsp_local_pid() const {
			return local_pid;
		}

		/** We store the parent context, and add this function
		 * to retrieve it later. Can return NULL for top-level contexts.
		 */
		bsp::Context * get_parent_context() {
			return parentcontext;
		}

		/************************************************************************/
		/* Helpers for context injection into a scope, and generic creation     */
		/************************************************************************/

		/** FUN is a function taking a Context* as its parameter */
		typedef void (*FUN)(Context *);

		/** Execute will run the runme function with this as it's argument */
		void execute () {
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
		Context * parentcontext;	
		TaskMapper * mapper;  ///< The process mapper object
		int pid;		///< The global pid of this computation
		int local_pid;	///< The local pid of this computation
	};


	/**
	 * as_class must be subclassed from Context and 
	 * have a member function run();
	 */
	template <class as> 
	static void run_context_as (Context * thiz) {
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
		Context * create ( TaskMapper * tm, int bsp_pid, Context * parent) {
			Context * p = new runnablecontext();
			p->initialize_context(tm, bsp_pid, parent);
			return p;
		}

		void destroy ( Context * t ) {
			delete (runnablecontext * ) t;
		}
	};

};


#endif // __Context_H__
