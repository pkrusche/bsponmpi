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


/** @file bench.cpp

BSP Benchmarking tool.

Measures computation rate r, communication rate g, and latency l.

@author Peter Krusche
*/

#include "bsp_cpp/bsp_cpp.h"

#include "bench_r.h"

int main(int argc, char **argv) {
	bsp_init(&argc, &argv);
	using namespace std;
	using namespace bsp;

	// Things from here on are node-level SPMD. 
	// You'll have as many processes as there are
	// available via MPI.
	int processors = 2;
	int nmin = 1;
	int nmax = 512;
	double warmuptime = 2.0;

	/** This is how we read and parse command line options */
	try {
		using namespace bsp;
		using namespace boost::program_options;
		options_description opts;
		opts.add_options()
			("help", "produce a help message")
			("procs,p", value<int>()->default_value(2), 
			"How many processors to recursively create.")
			("nmin,l", value<int>()->default_value(1), 
			"Minimum value of n.")
			("nmax,r", value<int>()->default_value(512), 
			"Minimum value of n.")
			("warmup,w", value<double>()->default_value(2.0),
			"How much time to warm up. (default: 2s)"
			)
			;
		variables_map vm;

		bsp_command_line(argc, argv, opts, vm);

		processors = vm["procs"].as<int>();
		nmin = vm["nmin"].as<int>();
		nmax = vm["nmax"].as<int>();
		warmuptime = vm["warmup"].as<double>();
	} catch (std::runtime_error e) {
		string s = e.what();
		s+= "\n";
		bsp_abort(s.c_str());
	}

	bsp_warmup ( warmuptime );
	benchmark::CompilerDAXPYs p;
	benchmark::Benchmark b = p.run(processors, nmin, nmax);

	if (bsp_pid() == 0) {
		b.ratetable(std::cout);
	}

	bsp_end();
} /* end main */
