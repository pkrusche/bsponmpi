/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_cpp/simplebsp.h"

#include <iostream>

#include <tbb/spin_mutex.h>

tbb::spin_mutex output_mutex;

int recursive_processors = 2;

bsp_global_handle_t h;

class MyContext : public bsp::Context {
public:
	int counter;
};


/**
 * Main function
 */
int main (int argc, char** argv) {
	using namespace std;
	bsp_init (&argc, &argv);

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

	bsp_begin(MyContext, recursive_processors);

	// Things from here on are node-level SPMD. 
	// You'll have as many processes as there are
	// available via MPI.
	{
		tbb::spin_mutex::scoped_lock l (output_mutex);
		cout << "Hi, I am processor " << bsp_pid()+1 << " of " << bsp_nprocs() << endl;
	}

	counter = bsp_pid();

	bsp_global_put(&counter, h, sizeof(int) * (bsp_nprocs()-1-bsp_pid()), sizeof(int));
	
	bsp_sync();
	
	bsp_global_get(h, sizeof(int) * (bsp_pid()), &counter, sizeof(int));

	bsp_sync();

	{
		tbb::spin_mutex::scoped_lock l (output_mutex);
		cout << "Hi, I am processor " << bsp_pid()+1 << " of " << bsp_nprocs() << ", I have read value " <<
			counter << " from the global memory " << endl;
	}

	bsp_end();
}
