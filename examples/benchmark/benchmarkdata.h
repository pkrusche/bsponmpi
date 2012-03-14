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


/** @file benchmarkdata.h

Helper class to determine computation or communication rates and latencies 
from a set of samples.

@author Peter Krusche
*/

#ifndef __benchmarkdata_H__
#define __benchmarkdata_H__

#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp> 

#include "bsp_cpp/Shared/SharedVariable.h"

namespace benchmark {
	
	/** This class handles benchmark data statistics */
	class BenchmarkData {
	public:

		BenchmarkData() {}

		BenchmarkData(BenchmarkData const & rhs) {
			add_samples(rhs);
		}

		/** operator = implementation */
		BenchmarkData & operator=(BenchmarkData const & rhs) {
			if (&rhs == this) {
				return *this;
			}
			samples.clear();
			add_samples(rhs);
			return *this;
		}

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

		/** 
		 * Each sample has: a problem size (n), and a rate. 
		 */
		inline void add_samples (BenchmarkData const & d) {
			for (size_t j = 0; j < d.samples.size(); ++j ) {
				samples.push_back(d.samples[j]);
			}
		}

		/**
		 * Reset the list of samples
		 */
		inline void reset_samples() {
			samples.clear();
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

		/** Get the list of samples */
		std::vector<Sample> & get_samples() { return samples; }

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
				it->second.samples = (int)it->second._samples.size();
			}
		}

		std::vector<Sample> samples;
	};

	/** Combine two sets of results */
	template <class _c>
	struct BenchmarkCombine {
		void operator () (BenchmarkData & b, BenchmarkData const & b_add) {
			b.add_samples(b_add);
		}
	};

};

/************************************************************************/
/* Make benchmark data serializable                                     */
/************************************************************************/

template <> 
inline size_t bsp::SharedSerializable<benchmark::BenchmarkData>::serialized_size() {
	return sizeof(size_t) + sizeof(benchmark::BenchmarkData::Sample) * valadr->get_samples().size();
}

template <> 
inline void bsp::SharedSerializable<benchmark::BenchmarkData>::serialize (void * target, size_t nbytes) {
	ASSERT (nbytes >= serialized_size());
	*((size_t *) target) = valadr->get_samples().size();
	
	benchmark::BenchmarkData::Sample * sp = (benchmark::BenchmarkData::Sample *)(((size_t *) target) + 1);
	for (size_t k = 0; k < valadr->get_samples().size(); ++k) {
		memcpy ( sp, &(valadr->get_samples()[k]), sizeof (benchmark::BenchmarkData::Sample) );
		++sp;
	}	
}

template <> 
inline void bsp::SharedSerializable<benchmark::BenchmarkData>::deserialize(void * source, size_t nbytes) {
	ASSERT (nbytes >= sizeof(size_t) );
	size_t len = *((size_t*)source);
	ASSERT (nbytes >= sizeof(size_t) + len*sizeof(benchmark::BenchmarkData::Sample) );
	
	valadr->get_samples().resize(len);

	benchmark::BenchmarkData::Sample * sp = (benchmark::BenchmarkData::Sample *)(((size_t *) source) + 1);
	for (size_t k = 0; k < len; ++k) {
		memcpy ( &(valadr->get_samples()[k]), sp, sizeof (benchmark::BenchmarkData::Sample) );
		++sp;
	}
}



#endif
