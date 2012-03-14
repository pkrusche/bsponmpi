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

#ifndef __benchmark_H__
#define __benchmark_H__

#include "bsp_cpp/bsp_cpp.h"
#include "benchmarkdata.h"

namespace benchmark {
	
	/** Class to run a benchmark for various values of n in a BSP context */
	class BenchmarkRunner : public bsp::Context {
	public:
		public:

			/** Set up input parameter sharing */
			BenchmarkRunner() {
				CONTEXT_SHARED_INIT(n, int);
				CONTEXT_SHARED_INIT(nmin, int);
				CONTEXT_SHARED_INIT(nmax, int);
				CONTEXT_SHARED_INIT(bmname, std::string);
				CONTEXT_SHARED_INIT(step, int);
			}

			/** Set up parameters */
			void set_parameters (std::string const & , int , int , int );

			/** Get the results */
			BenchmarkData & get_result ();
		
		protected:
		
			/** Run the benchmark (called by Runner) */ 
			void run();

	
		private:
			static BenchmarkData b;
			double * p_rates;
			
			// n counter
			int n, nmin, nmax, step;
			std::string bmname;
		};
	
;}

#endif // __benchmark_H__
