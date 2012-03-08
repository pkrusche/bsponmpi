/***************************************************************************
 *   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#include "bsp_cpp/bsp_cpp.h"

#include <iostream>

#include <tbb/spin_mutex.h>

/** this is a helper to make sure output doesn't get garbled */
tbb::spin_mutex output_mutex;

class MyContext : public bsp::Context {
public:
	void init() {
		var1 = ((MyContext*)get_parent_context())->var1;
		memset(var2, 0, sizeof(int)*5);
		var3 = ((MyContext*)get_parent_context())->var3;
	}

	static void run( int processors ) {
		using namespace std;
		MyContext c;
		BSP_SCOPE(MyContext, c, processors);
		BSP_BEGIN();

		bsp_push_reg(&var1, sizeof (int));
		bsp_push_reg(var2, sizeof (int));
		bsp_push_reg(&var3, sizeof (int));

		BSP_SYNC();

		myval1 = bsp_pid() + 1;
		myval2 = bsp_nprocs() - bsp_pid();
		myval3 = bsp_pid() + 1;
		
		{
			tbb::spin_mutex::scoped_lock l (output_mutex);
			cout << "Process " << bsp_pid() << " sends values [" 
				 << myval1 << "," << myval2 << "," << myval3 << "]"
				 << " to process " << bsp_nprocs() - 1 - bsp_pid() << endl;
		}

		bsp_hpput (bsp_nprocs() - 1 - bsp_pid(), &myval1, &var1, 0, sizeof(int));
		bsp_hpput (bsp_nprocs() - 1 - bsp_pid(), &myval2, var2, 2*sizeof(int), sizeof(int));
		bsp_hpput (bsp_nprocs() - 1 - bsp_pid(), &myval3, &var3, 0, sizeof(int));

		BSP_SYNC();

		{
			tbb::spin_mutex::scoped_lock l (output_mutex);
			cout << "Process " << bsp_pid() << " received values [" 
				<< var1 << "," << var2[2] << "," << var3 << "]"
				<< endl;
		}


		bsp_pop_reg(&var1);
		bsp_pop_reg(var2);
		bsp_pop_reg(&var3);

		BSP_END();
	}


protected:
	int var1;
	int var2[5];
	int var3;
	int myval1;
	int myval2;
	int myval3;
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


	try {
		MyContext::run( recursive_processors );

		bsp_end();


	} catch (std::exception e) {
		string s = string ("BSP Application runtime error: ") + e.what();
		s+= "\n";
		bsp_abort(s.c_str());
	}
}
