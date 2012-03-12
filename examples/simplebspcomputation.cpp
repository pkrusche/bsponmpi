/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_cpp/bsp_cpp.h"

#include <iostream>

#include <tbb/spin_mutex.h>

/** this is a helper to make sure output doesn't get garbled */
tbb::spin_mutex output_mutex;


static bsp_global_handle_t h;

/** When declaring BSP computations, we first declare a 
 *  Context. A context holds all variables which persist 
 *  over multiple supersteps.
 *  
 *  Inside a Context we have access to bspwww primitives, except
 *  for bsp_sync. The reason is that bsp_sync needs to be called
 *  at node level, and our virtual processors are implemented as
 *  TBB tasks. To do this, we split the code into chunks which
 *  are run through the scheduler. This splitting is done using 
 *  the BSP_BEGIN(), BSP_SYNC() and BSP_END() mechanism shown 
 *  in the run() method.
 *  
 */

class MyContext : public bsp::Context {
public:
	MyContext() : counter (0) { }

	void init() {
		counter = ((MyContext*)get_parent_context())->counter;
	}

protected:

	/**
	 * We can make functions which are called from within the 
	 * supersteps, but they need to be class member to get access
	 * to the correct bsp_... functions.
	 */
	void print_info () {
		using namespace std;
		tbb::spin_mutex::scoped_lock l (output_mutex);
		cout << "Hi, I am processor " << bsp_pid()+1 << " of " << bsp_nprocs() << endl;
	}

	/** Members must be public or protected, since BSP_BEGIN et al. 
	 *  rely on them being accessible to subclasses */
	int counter;
};

/** Once we have a context, we define a computation on that context */
class MyComputation : public bsp::Computation<MyContext> {
public:
	MyComputation (int _processors) : processors (_processors) {}
	
	void run() {
		using namespace std;

		/** we can still do stuff on node level here. like allocate 
		 *  global memory. Note that bsp_pushreg doesn't fall into 
		 *  this category, this, we want to do after BSP_BEGIN (if
		 *  the memory should be local to tasks rather than nodes, 
		 *  that is).
		 */
		h = bsp_global_alloc(processors * sizeof(int));
		::bsp_sync();

		BSP_BEGIN(processors)

		/** 
		 * Things from here on are task-level SPMD. 
		 * You'll have as many processes as allocated in the task mapper
		 * also, we inject all members of MyContext into the current
		 * scope
		 */

		print_info();
		counter = bsp_pid();

		bsp_global_put(&counter, h, sizeof(int) * (bsp_nprocs()-1-bsp_pid()), sizeof(int));

		BSP_SYNC();

		bsp_global_get(h, sizeof(int) * (bsp_pid()), &counter, sizeof(int));

		BSP_SYNC();

		{
			tbb::spin_mutex::scoped_lock l (output_mutex);
			cout << "Hi, I am processor " << bsp_pid()+1 << " of " << bsp_nprocs() << ", I have read value " <<
				counter << " from the global memory " << endl;
		}

		BSP_END();

		bsp_global_free(h);
	}
	
private:
	int processors;
};


/**
 * Main function
 */
int main (int argc, char** argv) {
	using namespace std;
	bsp_init (&argc, &argv);

	// Things from here on are node-level SPMD. 
	// You'll have as many processes as there are
	// available via MPI.
	int processors = 2;

	/** This is how we read and parse command line options */
	try {
		using namespace bsp;
		using namespace boost::program_options;
		options_description opts;
		opts.add_options()
			("help", "produce a help message")
			("procs,p", value<int>()->default_value(2),
			"How many processors to recursively create.")
			;
		variables_map vm;

		bsp_command_line(argc, argv, opts, vm);

		processors = vm["procs"].as<int>();
	} catch (std::exception e) {
		string s = e.what();
		s+= "\n";
		bsp_abort(s.c_str());
	}
	
	MyComputation comp(processors);
	comp.run();

	bsp_end();
}
