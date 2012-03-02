/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_cpp/bsp_cpp.h"

#include <iostream>

#include <tbb/spin_mutex.h>

tbb::spin_mutex output_mutex;
bsp_global_handle_t h;

class MyContext : public bsp::Context {
public:
	int counter;

	MyContext() : counter (0) {}

	void init() {
		counter = ((MyContext*)get_parent_context())->counter;
	}

	/**
	 * Run function to start execution
	 * 
	 * Here are a few things to note.
	 * 
	 * - the code in this function does not have to be inside the class.
	 *   Once MyContext is declared as a subclass of bsp::Context,
	 *   BSP_BEGIN(MyContext, tm) works IN ANY SCOPE. 
	 * - Debugging this is a little non-obvious. The code for each
	 *   superstep is injected into local subclasses of MyContext, 
	 *   and run on the MyContext objects belonging to each task. 
	 *   Therefore, the scope after BSP_BEGIN is actually not the same
	 *   scope as the one before. This is done as a workaround for until
	 *   all C++ compilers support lambda functions/contexts.
	 *  
	 */
	void run( int processors ) {
		using namespace std;
		bsp::ContextFactory<MyContext> factory;
		bsp::TaskMapper tm (processors, &factory, this);

		BSP_BEGIN(MyContext, tm);

		// Things from here on are task-level SPMD. 
		// You'll have as many processes as allocated in the task mapper
		// also, we inject all members of MyContext into the current
		// scope
		{
			tbb::spin_mutex::scoped_lock l (output_mutex);
			cout << "Hi, I am processor " << bsp_pid()+1 << " of " << bsp_nprocs() << endl;
		}
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
	}
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
	int recursive_processors = 2;

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

		recursive_processors = vm["procs"].as<int>();
	} catch (std::exception e) {
		string s = e.what();
		s+= "\n";
		bsp_abort(s.c_str());
	}
	h = bsp_global_alloc(recursive_processors*sizeof (int));
	bsp_sync();

	MyContext root;
	root.run( recursive_processors );

	bsp_end();
}
