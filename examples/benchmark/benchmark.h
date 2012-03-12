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

#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

#include "bsp_cpp/bsp_cpp.h"

#include <boost/shared_ptr.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp> 

namespace benchmark {
	
	/** Abstract benchmark base class */
	class AbstractBenchmark {
	public:
		/** Run on problem size n, return a rate of operations (complexity(n) / time(n)) */
		virtual double run (int n) = 0;		
	};
	
#define REGISTER_BENCHMARK(name, desc, benchmarkclass) namespace { static RegisterBenchmark g_benchreg##name<benchmarkclass> (name, desc); };
	
	/** A benchmark factory to list all possible benchmarks */
	class BenchmarkFactory {
	public:		
		/** Create specific benchmark */
		static Benchmark * create(std::string const & );
		
		/** register a new benchmark */
		static void register (std::string const &, std::string const &, boost::shared_ptr<Benchmark>);		
	private:
		struct BenchmarkDesc {
			boost::shared_ptr<Benchmark> benchmark;
			std::string description;
		};
		
		static std::map<std::string, BenchmarkDesc > 
			factory_map;
	};
	
	/** Benchmark registration helper */
	template <class _b>
	class RegisterBenchmark {
	public:
		RegisterBenchmark(std::string const & name, std::string const & desc) {
			boost::shared_ptr<Benchmark> bmp (new _b);
			BenchmarkFactory::register (name, desc, bmp);
		}	
	};
	
	/** This class handles benchmark data statistics */
	class BenchmarkData {
	public:
		struct Sample {
			int n;
			double rate;
		};
		
		/** 
		 * Each sample has: a problem size (n), and a rate. 
		 */

		inline void add_sample (int n, double r) {
			Sample s;
			s.n = n;
			s.rate = r;
			samples.push_back(s);
		}

		/** output a table of rates and errors. */
		inline void ratetable ( std::ostream & output ) {
			std::map <int, psize_info > sample_map;
			collect( sample_map );

			std::vector<int> ns;
			for (std::map<int, psize_info>::iterator it = sample_map.begin(), it_end = sample_map.end();
				it != it_end; ++it) {
				ns.push_back( it->first );
			}
			std::sort (ns.begin(), ns.end());
			output << "n\tmean\tmin\tmax\terr" << std::endl;
			for (std::vector<int> :: iterator it = ns.begin(), it_end = ns.end(); it != it_end; ++it) {
				psize_info & p = sample_map[*it];
				output
					<< p.n << "\t" 
					<< p.tavg << "\t" 
					<< p.tmin << "\t" 
					<< p.tmax << "\t" 
					<< p.err << std::endl;
			}
		}

	private:

		struct psize_info {
			int n;
			double tmin;
			double tmax;
			double tavg;
			int samples;
			double err;
			std::list<double> _samples;
		};
		
		/**
		 * Collect samples and statistical information into a map.
		 */

		void collect ( std::map <int, psize_info > & sample_map ) {
			for (std::vector<Sample>::iterator it = samples.begin(), it_end = samples.end();
				it != it_end; ++it) {
				std::map<int, psize_info>::iterator it2 = sample_map.find (it->n);
				if (it2 == sample_map.end()) {
					psize_info p;
					sample_map[it->n] = p;
				}
				psize_info & pr = sample_map[it->n];
				pr.n = it->n;
				pr._samples.push_back(it->rate);
			}

			using namespace boost;
			using namespace boost::accumulators;

			for (std::map<int, psize_info>::iterator it = sample_map.begin(), it_end = sample_map.end();
				it != it_end; ++it) {
				accumulator_set<double, stats<	tag::min, tag::max, tag::mean, tag::error_of<tag::mean> > > acc1;

				for (std::list<double>::iterator it2 = it->second._samples.begin(), it2_end = it->second._samples.end(); 
					 it2 != it2_end; ++it2) {
					acc1( *it2 );
				}

				it->second.tmin = min(acc1);
				it->second.tmax = max(acc1);
				it->second.tavg = mean(acc1);
				it->second.err = error_of<tag::mean> (acc1); 
				it->second.samples = it->second._samples.size();
			}
		}

		std::vector<Sample> samples;
	};

	/** class to run a benchmark for various values of n in a BSP context */
	class BenchmarkRunner : public bsp::Context {
	public:
		public:
			void init ();
			
			/** run a benchmark, add to benchmark data */
			Benchmark & run (std::string const & , int p, int nmin, int nmax, int step);
		private:
			static BenchmarkData b;
			double * p_rates;

			// n counter
			static int n;
		
	};
	
;}

#endif // __benchmark_H__
