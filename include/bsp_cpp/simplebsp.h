/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __SIMPLEBSP_H__
#define __SIMPLEBSP_H__

#include <list>
#include <vector>

#include "bsp_cpp.h"
#include <tbb/spin_rw_mutex.h>

namespace bsp {
	class RunnableContext;

};

static bsp::RunnableContext * bsp_parent() {	
	return NULL;									
}										

static bsp::RunnableContext * bsp_context() {		
	return NULL;									
}

namespace bsp {
	class CustomRunnable {
	public:
		typedef void (*FUN)(CustomRunnable *);
		void run() { 
			__runfn (this);  
		}
		FUN __runfn;
	};

	typedef bsp::SimpleFactory < CustomRunnable > datafactory_t;
	typedef bsp::ProcMapper< datafactory_t > procmapper_t;

	class RunnableContext : public bsp::BSPContext< procmapper_t >,
		public CustomRunnable {	
	public:
		RunnableContext ( procmapper_t & pm, int local_pid, RunnableContext * _parent = NULL ) :
			bsp::BSPContext< procmapper_t > (pm, local_pid), parent (_parent) {
		}

		RunnableContext * bsp_parent() {
			return parent;
		}

		RunnableContext * bsp_context() {
			return this;
		}
	private:
		RunnableContext * parent;
	}; 

	void update_mapper ( procmapper_t & mapper, CustomRunnable::FUN fun ) {
		for (int j = 0;	j < mapper.procs_this_node(); ++j ) {
			CustomRunnable * c = mapper.get_context (j);
			c->__runfn = fun;
		}
	}
};

#define BSP_BEGIN() 									\
	static int g_argc; 									\
	static char ** g_argv; 								\
	void runner(void) { 								\
		using namespace bsp;							\
		bsp_begin(-1); 									\
		bspcpp_read_global_options();					\

#define BSP_COMMAND_LINE(args, vm) do { bsp::bsp_command_line(g_argc, g_argv, args, vm); } while(0)

#define BSP_END()						\
		bsp_end(); 						\
	} 									\
int main(int argc, char ** argv) { 		\
	if(!singletons::task_scheduler_init.is_active()) {	\
		singletons::task_scheduler_init.initialize();	\
	}									\
	g_argc = argc; 						\
	g_argv = argv; 						\
	bsp_init(runner, argc, argv); 		\
	runner(); 							\
	return 0; 							\
}

#define BSP_CONTEXT_BEGIN(...) {				\
												\
class ContextData : public RunnableContext {	\
public:											\
	ContextData (procmapper_t * pm, int local_pid) : \
		RunnableContext (*pm, local_pid, bsp_parent()) {}		\
	__VA_ARGS__									\
												\
	ContextData * bsp_parent() {				\
		return (ContextData * )					\
			RunnableContext::bsp_parent();		\
	}											\
	ContextData * bsp_context() {				\
		return (ContextData * )					\
			RunnableContext::bsp_context();		\
	}											\
};												\
												\
class CastingFactory :							\
	public bsp::SimpleFactory < CustomRunnable > {	\
	CustomRunnable * create ( int local_pid,	\
		void * param ) {						\
		return new ContextData (				\
		(procmapper_t *) param, local_pid );	\
	}											\
	void destroy (CustomRunnable * v) {			\
		delete (ContextData *) v;				\
	}											\
};


// to insert context variable use: BSP_CONTEXT_VAR (type name)
#define BSP_CONTEXT_VAR(...) \
	__VA_ARGS__;

// end }
#define BSP_CONTEXT_END() }


#define BSP_PARALLEL_BEGIN(_prs)	{			\
	CastingFactory factory;						\
	procmapper_t mapper(&factory, _prs);		\

#define BSP_PARALLEL_END()	}

// each superstep gets a block.
#define BSP_SUPERSTEP_BEGIN() { 				\
	class MyRC : public ContextData {			\
	public:										\
		void runme () {							\

// end run, end class, and then run it
#define BSP_SUPERSTEP_END()						\
		} 										\
		static void run_as (CustomRunnable * ctx) { \
			( (MyRC*)ctx )->runme();			\
		}										\
	}; 											\
												\
	update_mapper (mapper, &MyRC::run_as);		\
	run_superstep<procmapper_t> (mapper); bsp_sync(); 		\
}

#define BSP_ONLY(pid) if ( bsp_pid() == pid )

#endif
