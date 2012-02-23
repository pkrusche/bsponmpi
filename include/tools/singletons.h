/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/
#ifndef task_scheduler_singleton_h__
#define task_scheduler_singleton_h__

#include <tbb/task_scheduler_init.h>
#include <tbb/mutex.h>
#include <tbb/task.h>

#include "bsp_cpp/ParameterFile.h"

namespace singletons {
	extern tbb::task_scheduler_init task_scheduler_init;
	extern tbb::mutex global_mutex;
	extern utilities::ParameterFile global_options;	
	extern tbb::task * root_task;
};

#endif // task_scheduler_singleton_h__
