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


/** @file benchmark.h

Helper class to determine computation or communication rates and latencies 
from a set of samples.

@author Peter Krusche
*/

#include "bsp_config.h"
#include "benchmark.h"
#include "benchmarkfactory.h"

#include <iostream>

/** Declaration of the benchmark object */
benchmark::BenchmarkData benchmark::BenchmarkRunner::b;

/** Benchmark runner implementation 
 * 
 * @param _bm benchmark name
 * @param _nmin start problem size
 * @param _nmax end problem size
 * @param _step step size
 * 
 */
void benchmark::BenchmarkRunner::set_parameters(
		std::string const & _bm, int _nmin, int _nmax, int _step) {
	nmin = _nmin;
	nmax = _nmax;
	step = _step;
	bmname = _bm;
}


/** implementation of benchmark runner */
void benchmark::BenchmarkRunner::run() {
	BSP_SCOPE(BenchmarkRunner);	
	BSP_BEGIN();

	p_rates = new double [bsp_nprocs()];
	bsp_push_reg(p_rates, sizeof(double) * bsp_nprocs());
	BSP_END();

	for (n = nmin; n < nmax; n += step) {
		BSP_BROADCAST_LOCAL(n);
		BSP_BEGIN();

		AbstractBenchmark * bm = benchmark::BenchmarkFactory::get_instance().create(bmname.c_str());
		double f = bm->run (n);

		bsp_put(0, &f, p_rates, sizeof(double) * bsp_pid(), sizeof(double));

		BSP_SYNC();

		if (bsp_pid() == 0) {
			for (int p = 0; p< bsp_nprocs(); ++p) {
				((BenchmarkRunner*)get_parent_context())->b.add_sample(n, p_rates[p]);
			}
			std::cerr << ".";
		}
		BSP_END();
	}
	if (bsp_pid() == 0) {
		std::cerr << std::endl;
	}
}

/** Get the results */
benchmark::BenchmarkData & benchmark::BenchmarkRunner::get_result () {
	return b;
}

