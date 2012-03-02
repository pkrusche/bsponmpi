/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_cpp/simplebsp.h"

#include <iostream>

#include <tbb/spin_mutex.h>

tbb::spin_mutex output_mutex;

int recursive_processors = 2;

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
	{
		using namespace bsp;
		using namespace boost::program_options;
		options_description opts;
		opts.add_options()
			("help", "produce a help message")
			("recursive_procs,r", value<int>()->default_value(2),
			"How many processors to recursively create.")
			;
		variables_map vm;

		bsp_command_line(argc, argv, opts, vm);

		recursive_processors = vm["recursive_procs"].as<int>();
	}

	bsp_begin(MyContext, recursive_processors);

	// Things from here on are node-level SPMD. 
	// You'll have as many processes as there are
	// available via MPI.

	cout << "Hi, I am processor " << bsp_pid()+1 << endl;
	
	cout << recursive_processors << endl;

	bsp_end();
}
