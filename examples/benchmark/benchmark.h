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

#include <boost/accumulators/statistics.hpp> 

namespace benchmark {

	class Benchmark {
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
		inline void void ratetable ( std::ostream & output ) {
			std::map <int, psize_info > sample_map;
			collect(sample_map, c);

			std::list<int> ns;
			for (std::map<int, psize_info>::iterator it = sample_map.begin(), it_end = sample_map.end();
				it != it_end; ++it) {
				ns.push_back(*it);
			}
			std::sort (ns.begin(), ns.end());
			output << "n\tmean\tmin\tmax\ttavg\tmedian\terr\tsamples" << std::endl;
			for (std::list<int> :: iterator it = ns.begin(), it_end = ns.end(); it != it_end; ++it) {
				psize_info & p = sample_map[*it];
				output	<< p.n << "\t" << p.tmin<< "\t" << p.tmax << << "\t" 
						<< p.tavg << "\t" << p.tmerr << "\t" << p.tmed << std::endl;
			}
		}

	private:

		struct psize_info {
			int n;
			double tmin;
			double tmax;
			double tavg;
			double tmed;
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

			using namespace boost::accumulators;

			for (std::map<int, psize_info>::iterator it = sample_map.begin(), it_end = sample_map.end();
				it != it_end; ++it) {
				accumulator_set<double, 
					stats<tag::min, tag::max, tag::mean, tag::median, tag::error_of<tag::mean> > > acc1;

				for (std::list<double>::iterator it2 = it->second._samples.begin(), it2_end = it->second._samples.end(); 
					 it2 != it2_end; ++it2) {
					acc1( *it2 );
				}

				psize_info.tmin = min (acc1);
				psize_info.tmax = max(acc1);
				psize_info.tmed = median(acc1);
				psize_info.tavg = mean(acc1);
				psize_info.samples = it2->second._samples.size();
				psize_info.err = error_of<tag::mean> (acc1);
			}
		}

		std::vector<Sample> samples;
	};

;}

#endif // __benchmark_H__
