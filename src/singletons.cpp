/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include <string>
#include <iostream>

#include "bsp_tools/singletons.h"

tbb::task_scheduler_init singletons::task_scheduler_init;
tbb::mutex singletons::global_mutex;
utilities::ParameterFile singletons::global_options;
tbb::task * singletons::root_task = NULL;