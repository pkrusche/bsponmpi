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


/** @file bsp_test_shared_array.cpp

@author Peter Krusche

Test to verify shared array variables work

*/

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <vector> 
#include <cmath>

#include "boost/shared_array.hpp"
#include "bsp_cpp/bsp_cpp.h"

#define TEST_LEN 101

// #include <tbb/mutex.h>

/** mutex to make sure output doesn't get garbled */
// tbb::mutex countMutex;

/** Test class which stores a double value, and combines
 *  the two values by computing their geometric mean */
class GeoMean : public bsp::Reduceable {
public:
	GeoMean() : value(0) {}
	GeoMean(double _value) : value(_value) {}

	GeoMean const & operator=(double rhs) {
		value = rhs;
		return *this;
	} 

	GeoMean const & operator=(GeoMean const & rhs) {
		value = rhs.value;
		return *this;
	} 

	operator double () {
		return value;
	}

	void serialize(void * target, size_t nbytes) {
		ASSERT(nbytes >= sizeof(double));
		*((double*)target) = value;
	}

	void deserialize(void * source, size_t nbytes) {
		ASSERT(nbytes >= sizeof(double));
		value = *((double*)source);
	}

	size_t serialized_size() {
		return sizeof(double);
	}

	void make_neutral() {
		value = 0;
	}

	void reduce_with(Reduceable const * _other) {
		GeoMean const * other = static_cast<GeoMean const *>(_other);
		value = sqrt(value*value + other->value*other->value);
	}

private:
	double value;
};

/** Test for sharing variables and arrays
 *
 **/
class TestArraySharing : public bsp::Context {
public:
	TestArraySharing() :
		var1(100), var2(TEST_LEN), var3(TEST_LEN)
	{
		var2.init(1);
		var3.init(GeoMean(5.0));

		CONTEXT_SHARED_BOTH(bsp::ReduceSum, var1, int);
		CONTEXT_SHARED_OBJECT(var2, bsp::SharedArray<int, bsp::ReduceSum > );
		CONTEXT_SHARED_OBJECT(var3, bsp::SharedArray<GeoMean, bsp::Reduce > );
	}

	void run() {
		BSP_SCOPE(TestArraySharing);
		BSP_BEGIN()

		if(var1 != 100) {
			bsp_abort("Initialisiation of var1 to 100 failed, value is %i.", var1);
		}


		for (int i = 0; i < TEST_LEN; ++i) {
			double d = (double)var3[i];
			d = fabs(d-5);
			if(d > 0.0001) {
				bsp_abort("var3 Initialisiation failed: expt 5, value is %f", (double)var3[i]);
			}
			var3[i] = (1+bsp_pid()) * i;
		}
		var1 = 1+bsp_pid();

		// {
			// tbb::mutex::scoped_lock lock(countMutex);
			// std::cerr << var1 << " (" << ::bsp_pid() << ")" <<  std::endl;
		// }
		
		for (int i = 0; i < TEST_LEN; ++i) {
			if(var2[i] != 1) {
				bsp_abort("Initialisiation of var2 to 1 failed, value is %i.", var2[i]);
			}
			var2[i] = var1;
		}

		// std::cerr << var1 << std::endl;

		BSP_END()
	}

	void test_array_result( int processors ) {
		test_var1_result(processors);
		for (int i = 0; i < TEST_LEN; ++i) {
			if (var1 != var2[i]) {
				bsp_abort("Error: incorrect result at %i %i (expt: %i)", 
					i, var2[i], var1);
			}
			double gmean = 0;
			for (int p = 1; p <= processors; ++p) {
				double pp = p*i;
				gmean = sqrt(gmean*gmean + pp*pp);
			}
			double test = fabs(gmean - (double)var3[i]);
			if(test > 0.00001) {
				bsp_abort("Error: incorrect result at %i %f (expt: %f)", 
					i, (double)var3[i], gmean);	
			// } else {
			// 	std::cerr << i << "\t" << (double)var3[i] << "\t" <<  gmean << std::endl;
			}
		}
	}

protected:
	void test_var1_result(int processors) {
		if (var1 != processors*(processors+1)/2) {
			bsp_abort("Error: incorrect result %i (expt: %i)", 
				var1, processors*(processors+1)/2);
		}
	}

protected:
	int var1;
	bsp::SharedArray<int, bsp::ReduceSum > var2;
	bsp::SharedArray<GeoMean, bsp::Reduce> var3;
};


int main(int argc, char * argv[]) {
	bsp_init(&argc, &argv);

	int processors = 5;
	if (argc > 1) {
		processors = atoi(argv[1]);
		if (processors < 1) {
			processors = 1;
		}
	}

	bsp::Runner<TestArraySharing> runner(processors);

	runner.run();
	runner.test_array_result(processors);

	bsp_end();
	return 0;
}
