/*
BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
Copyright (C) 2006  Wijnand J. Suijlen, 2012 Peter Krusche

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

See the AUTHORS file distributed with this library for author contact
information.
*/


/** @file bsp_test_ops.cpp

Test all BSP communication operations in a context.

@author Peter Krusche
*/


#include "bsp_cpp/bsp_cpp.h"

#include "ops/test_put.h"
#include "ops/test_hpput.h"
#include "ops/test_get.h"
#include "ops/test_hpget.h"


/**
 * Main function
 */
int main (int argc, char** argv) {
	using namespace std;
	bsp_init (&argc, &argv);

	// Things from here on are node-level SPMD. 
	// You'll have as many processes as there are
	// available via MPI.
	int procs = 1;
	int max_processors = 100;

	/** This is how we read and parse command line options */
	try {
		using namespace bsp;
		using namespace boost::program_options;
		options_description opts;
		opts.add_options()
			("help", "produce a help message")
			("pmin,l", value<int>()->default_value(1),
			"We test for all numbers of processors from pmin to pmax.")
			("pmax,r", value<int>()->default_value(100),
			"We test for all numbers of processors from pmin to pmax.")
			;
		variables_map vm;

		bsp_command_line(argc, argv, opts, vm);

		procs = vm["pmin"].as<int>();
		max_processors = vm["pmax"].as<int>();
	} catch (std::exception e) {
		string s = e.what();
		s+= "\n";
		bsp_abort(s.c_str());
	}
	
	procs = min(procs, max_processors);
	max_processors = max(procs, max_processors);

	try {

		while (procs <= max_processors) {
			cout << "Testing put p = " << procs << endl;
			TestPut::run( procs );
			bsp_sync();
			cout << "Testing hpput p = " << procs << endl;
			TestHpPut::run( procs );
			bsp_sync();
			cout << "Testing get p = " << procs << endl;
			TestGet::run( procs );
			bsp_sync();
			cout << "Testing hpget p = " << procs << endl;
			TestHpGet::run( procs );
			bsp_sync();
			++procs;
		}

	} catch (std::runtime_error e) {
		string s = string ("BSP Application runtime error at p=%i: ") + e.what() + "\n";
		bsp_abort(s.c_str(), procs);
	}
	bsp_end();
}
