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


/** @file benchmarkfactory.h

Maintain a list of benchmarks and their description.

@author Peter Krusche
*/

#ifndef __Benchmarkfactory_h__
#define __Benchmarkfactory_h__ 

#include <boost/bind.hpp>

namespace benchmark {
	/** Abstract benchmark base class */
	class AbstractBenchmark {
	public:
		/** Run on problem size n, return a rate of operations (complexity(n) / time(n)) */
		virtual double run (int n) = 0;		
	};
	
	/** Benchmark factory */
	class BenchmarkFactory {
	public:
		typedef boost::function0< AbstractBenchmark* > b_factory;
		struct description {
			std::string desc;
			b_factory factory;
		};

		/** Get factory instance */
	    static BenchmarkFactory & get_instance() {
			static BenchmarkFactory f;
			return f;
		}
			
		/** Register a new benchmark type */
		inline void reg (const char * name, const char * desc, b_factory f) {
			description mbd;
			mbd.desc = std::string (desc);
			mbd.factory = f;
			map[index(name)] = mbd;
		}

		/** Remove a benchmark type */
		inline void remove (const char * name) {
			map.erase (map.find(index(name)));
		}
	
		/** Create a benchmark */
		inline AbstractBenchmark* create (const char * name) {
			if (map.find(index(name)) == map.end()) {
				throw std::runtime_error (std::string ( "Unknown benchmark type: '") + index(name) + "'");
			}
			return map[index(name)].factory();
		}

		/** Get the description for a benchmark */
		inline std::string describe (const char * name) {
			if (map.find(index(name)) == map.end()) {
				throw std::runtime_error (std::string ( "Unknown benchmark type: '") + index(name) + "'");
			}
			return map[index(name)].desc;
		}
	
		/** List all benchmark types to output stream */
		inline void list(std::ostream & out) {
			for (std::map<std::string, description>::iterator it = map.begin(), it_end = map.end();
					it != it_end; ++it) {
				out << "\t" << it->first << "\t" << it->second.desc << std::endl;
			}
		}
	
	private:

		inline std::string index (const char * name ) {
			std::string xname(name);
			std::transform(xname.begin(), xname.end(), xname.begin(), ::tolower);
			return xname;
		}

	    BenchmarkFactory() {}

	    std::map<std::string, description> map;
	};	

/** Static registration helper */
template <class B>
struct BenchmarkReg {
	static benchmark::AbstractBenchmark * create () {
		static B b;
		return &b;
	}
	
	BenchmarkReg(const char * name, const char * desc) {
		my_name = name;
		benchmark::BenchmarkFactory::get_instance().reg(name, desc, 
			boost::bind(&BenchmarkReg<B>::create));
	}
	
	~BenchmarkReg() {
		benchmark::BenchmarkFactory::get_instance().remove(my_name.c_str());
	}
	
	std::string my_name;
};

#define REGISTER_BENCHMARK(name, desc, benchmarkclass) \
	static benchmark::BenchmarkReg< benchmark::benchmarkclass > _br##benchmarkclass (name, desc); 

};

#endif
