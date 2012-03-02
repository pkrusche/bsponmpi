/***************************************************************************
 *   Copyright (C) 2012   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __SIMPLEBSP_H__
#define __SIMPLEBSP_H__

#include <list>
#include <vector>

#include "bsp_cpp.h"

/** Default parent context implementation.
 *  
 *  In recursive contexts, this will not be in scope, and the
 *  Function in Context will be called.
 */

static inline bsp::Context * get_parent_context() {
	return NULL;
}

/**
 * bsp_begin () macro 
 * 
 */

#define bsp_begin(context, p) {					\
	_bsp_sync();								\
	typedef context ContextData;				\
	bsp::ContextFactory<context> factory;		\
	bsp::TaskMapper tm (p, &factory,			\
				   get_parent_context());		\
	{											\
		class MyRC : public ContextData {		\
		public:									\
			void run () {

/** Save bsp_sync() */
static inline void _bsp_sync() {
	bsp_sync();
}

/**
 * We need to be able to sync *after* all the tasks
 * have finished, which is why we are replacing 
 * the standard bsp_sync with this construct.
 */
#define bsp_sync()								\
			} 									\
		};										\
		bsp::run_context<MyRC> (&tm);			\
	}											\
	tm.do_bsp_sync();							\
	{											\
		class MyRC : public ContextData {		\
		public:									\
			void run () {						

/** Save bsp_end() */
static inline void _bsp_end() {
	bsp_end();
}

#define bsp_end()								\
			} 									\
		};										\
												\
		bsp::run_context<MyRC> (&tm);			\
	}											\
	tm.do_bsp_sync();							\
	_bsp_end();									\
}


#endif

