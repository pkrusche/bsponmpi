/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bspcpp/simplebsp.h"

#include <iostream>

void print_info (bsp::RunnableContext * ctx, int counter) {
	using namespace std;

	if (!ctx) {
		bsp_abort("FUCK!");
	}

	if (ctx->bsp_parent() != NULL) {
		cout << "Hi " << 
				counter << 
				"! I am sub-process number " << 
				ctx->bsp_pid() << " out of " << ctx->bsp_nprocs() 
				<< " I live on node " << bsp_pid()
				<< endl;
	} else {
		cout << "Hi " << 
			counter << 
			"! I am process number " << 
			ctx->bsp_pid() << " out of " << ctx->bsp_nprocs() 
			<< " I live on node " << bsp_pid()
			<< endl;
	}
}


BSP_BEGIN();

using namespace std;

BSP_ONLY( 0 ) cout << "Hi, I am proc 0. I am special." << endl;

BSP_CONTEXT_BEGIN ( 
	BSP_CONTEXT_VAR ( int counter )
);
	using namespace std;

	BSP_PARALLEL_BEGIN ( 
		bsp_nprocs() * 3 // recursively increase number of processors
	);

	// this is done at node level 
	cout << "t:" << bsp_nthreads() << " p:" << bsp_nprocs() << endl;

	BSP_SUPERSTEP_BEGIN() {
		counter = 1;
		print_info(bsp_context(), counter);

		BSP_PARALLEL_BEGIN ( 
			bsp_nprocs() // recursively increase number of processors
		);

		BSP_SUPERSTEP_BEGIN() {
			counter = 1;
			print_info(bsp_context(), counter);
		}
		BSP_SUPERSTEP_END();

		BSP_SUPERSTEP_BEGIN() {
			counter = 2;
			print_info(bsp_context(), counter);
		} BSP_SUPERSTEP_END();

		BSP_PARALLEL_END ();

		counter++;

	} BSP_SUPERSTEP_END();

	BSP_SUPERSTEP_BEGIN() {
		counter++;
		print_info(bsp_context(), counter);
	} BSP_SUPERSTEP_END();

	
	

BSP_PARALLEL_END ();

BSP_CONTEXT_END ();

BSP_END();
