/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __Context_H__
#define __Context_H__

#include <stdexcept>
#include <list>

#include "TaskMapper.h"
#include "Shared/SharedVariableSet.h"
#include "Shared/SharedVariable.h"

namespace bsp {

	/**
	 * BSP context. Subclass this to run parallel steps.
	 * 
	 * Implementation in bsp_context.cpp
	 */
	class Context {
	public:		
		Context (TaskMapper * _mapper = NULL) : mapper(_mapper), 
			parentcontext (NULL), impl (NULL) { }

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
		 *  bsp_sync() is not valid, since we need to split tasks up into
		 *  individual supersteps ( only one thread should call ::bsp_sync()
		 *  in a given superstep ).
		 *  In debug mode, we check this using an assertion.
		 */
		void bsp_sync();

		void bsp_reset_buffers();

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
		void bsp_hpsend (int, const void *, const void *, size_t);
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

		/** returns true if execution is at task level */
		inline bool bsp_is_task_level () {
			return impl != NULL;
		}

		/** returns true if execution is at node level */
		inline bool bsp_is_node_level () {
			return impl == NULL;
		}

		/************************************************************************/
		/* Context variable sharing collective functions                        */
		/************************************************************************/

		/** @name Context Sharing */
		/*@{*/
		/** Shortcut to share variables in a context constructor */
#define CONTEXT_SHARED_INIT(var, ...)			\
	SHARE_VARIABLE_I(context_sharing, var, __VA_ARGS__);

		/** Shortcut to share reduction variables in a context constructor */
#define CONTEXT_SHARED_REDUCE(red, neutral, var, ...)				\
	SHARE_VARIABLE_R(context_sharing, bsp :: red, neutral, var, __VA_ARGS__);

		/** Shortcut to share variables in a context constructor */
#define CONTEXT_SHARED_BOTH(red, neutral, var, ...)			\
	SHARE_VARIABLE_IR(context_sharing, bsp :: red, neutral, var, __VA_ARGS__);

		/** Shortcut for initializing variables between supersteps */
#define BSP_BROADCAST(var, mn) do {				\
		ASSERT (bsp_is_node_level());			\
		context_sharing.initialize(#var, mn);	\
	} while (0);

		/** Shortcut to fold variables between supersteps */
#define BSP_FOLD(var) do {					\
	ASSERT (bsp_is_node_level());			\
	context_sharing.reduce(#var, true);		\
		} while (0);

		/** Shortcut to initialize single variables node-locally */
#define BSP_BROADCAST_LOCAL(var) do {		\
	ASSERT (bsp_is_node_level());			\
	context_sharing.initialize(#var, -1);	\
		} while (0);

		/** Shortcut to fold variables between supersteps node-locally */
#define BSP_FOLD_LOCAL(var) do {			\
	ASSERT (bsp_is_node_level());			\
	context_sharing.reduce(#var, false);	\
		} while (0);


		/** Initialize all variables that were shared with CONTEXT_SHARED_INIT or 
		 *  CONTEXT_SHARED_BOTH */
		inline void initialize_shared (int master_node) {
			ASSERT (bsp_is_node_level());
			context_sharing.initialize_all(master_node);
		}

		/** Reduce all context variables which were shared with CONTEXT_SHARED_REDUCE
		 *  or CONTEXT_SHARED_BOTH */
		inline void reduce_shared () {
			ASSERT (bsp_is_node_level());
			context_sharing.reduce_all();			
		}
		/*@}*/

		/************************************************************************/
		/* Helpers for context running                                          */
		/************************************************************************/

		virtual void run () = 0;
		
		inline void execute_step () {
			ASSERT (mapper->get_next_step());
			mapper->get_next_step() (this);
		}

		inline void set_task_mapper (TaskMapper * _m) {
			mapper = _m;
		}

		inline TaskMapper * get_mapper() {
			return mapper;
		}

		/************************************************************************/
		/* Implementation helper                                                */
		/************************************************************************/
		inline void * get_impl() { return impl; }

	protected:
		Context * parentcontext;	///< this is the parent context 		
		
		TaskMapper*  mapper;		///< The process mapper object for this context
				
		int pid;		///< The global pid of this computation
		int local_pid;	///< The local pid of this computation

		SharedVariableSet context_sharing; ///< Context variable sharing set
	private:

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
		typedef runnablecontext context_t;
		
		ContextFactory ( Context * _parent ) : 
			parent (_parent) {}

		inline Context * create ( TaskMapper * mapper, int bsp_pid ) {
			Context * p = new runnablecontext ();
			p->set_task_mapper(mapper);
			p->initialize_context( bsp_pid, parent );
			return p;
		}

		inline void destroy ( Context * t ) {
			t->destroy_context();
			delete (runnablecontext * ) t;
		}
		
		inline Context * get_parent () { return parent; }

	private:
		Context * parent;
	};

};


#endif // __Context_H__
