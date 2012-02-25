/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_cpp/simplebsp.h"

#include <iostream>

#include <tbb/spin_mutex.h>

tbb::spin_mutex output_mutex;

void print_info (bsp::RunnableContext * ctx, int counter) {
	using namespace std;
/*

	tbb::spin_mutex::scoped_lock l(output_mutex);

	if (!ctx) {
		bsp_abort("Can't print NULL context.");
	}

*/
/*
	std::string prefix = "";
	for(int j = 0; j < ctx->bsp_call_level(); ++j) {
		prefix+= "\t";
	}

	if (ctx->bsp_parent() != NULL) {
*/
/*
		cout << prefix << "Hi " << 
				counter << 
				"! I am sub-process number " << 
				ctx->bsp_pid() << " out of " << ctx->bsp_nprocs() 
				<< " I live on node " << bsp_pid()
//				<< "; my parent is process number " << ctx->bsp_parent()->bsp_pid()
				<< endl;
	} else {
		cout << prefix << "Hi " << 
			counter << 
			"! I am process number " << 
			ctx->bsp_pid() << " out of " << ctx->bsp_nprocs() 
			<< " I live on node " << bsp_pid()
			<< endl;
	}
*/
}


BSP_BEGIN();

using namespace std;

BSP_ONLY( 0 ) cout << "Hi, I am proc 0. I am special." << endl;

BSP_CONTEXT_BEGIN ( 
	BSP_CONTEXT_VAR ( int counter )
);
	using namespace std;

	BSP_PARALLEL_BEGIN ( 
		bsp_nprocs() * 4 // recursively increase number of processors
	);

	// this is done at node level 
	cout << "t:" << bsp_nthreads() << " p:" << bsp_nprocs() << endl;

	BSP_SUPERSTEP_BEGIN() {
		counter = 1;
		print_info((bsp::RunnableContext*)bsp_context(), counter);
		cout << bsp_pid() << " " << this->bsp_call_level() << endl;

		BSP_PARALLEL_BEGIN ( 
			bsp_nprocs() // recursively increase number of processors
		);

		BSP_SUPERSTEP_BEGIN() {
			counter = 1;
			cout << bsp_pid() << " " << this->bsp_call_level() << endl;
			print_info((bsp::RunnableContext*)bsp_context(), counter);
		}
		BSP_SUPERSTEP_END();

		BSP_SUPERSTEP_BEGIN() {
			counter = 2;
			print_info((bsp::RunnableContext*)bsp_context(), counter);
		} BSP_SUPERSTEP_END();
		BSP_PARALLEL_BEGIN ( 
			bsp_nprocs() // recursively increase number of processors
			);

		BSP_SUPERSTEP_BEGIN() {
			counter = 1;
			cout << bsp_pid() << " " << this->bsp_call_level() << endl;
			print_info((bsp::RunnableContext*)bsp_context(), counter);
		}
		BSP_SUPERSTEP_END();

		BSP_SUPERSTEP_BEGIN() {
			counter = 2;
			print_info((bsp::RunnableContext*)bsp_context(), counter);
		} BSP_SUPERSTEP_END();

		BSP_PARALLEL_END ();

		BSP_PARALLEL_END ();

		counter++;

	} BSP_SUPERSTEP_END();

	BSP_SUPERSTEP_BEGIN() {
		counter++;
		print_info((bsp::RunnableContext*)bsp_context(), counter);
	} BSP_SUPERSTEP_END();

	
	

BSP_PARALLEL_END ();

BSP_CONTEXT_END ();

BSP_END();
