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

#include "boost/shared_array.hpp"
#include "bsp_cpp/bsp_cpp.h"

#define TEST_LEN 101

#include <tbb/mutex.h>

/** mutex to make sure output doesn't get garbled */
tbb::mutex countMutex;

/** Test for sharing variables and arrays
 *
 **/
class TestArraySharing : public bsp::Context {
public:
	TestArraySharing() :
		var1(100), var2(TEST_LEN)
	{
		CONTEXT_SHARED_BOTH(bsp::ReduceSum, var1, int);
		CONTEXT_SHARED_OBJECT(var2, bsp::SharedArray<int>);
	}

	void setup () {
		var2.init(1);
	}

	void run() {
		BSP_SCOPE(TestArraySharing);
		BSP_BEGIN()

		assert(var1 == 100);

		var1 = 1+bsp_pid();

		{
			tbb::mutex::scoped_lock lock(countMutex);
			std::cerr << var1 << " (" << ::bsp_pid() << ")" <<  std::endl;
		}
		for (int i = 0; i < TEST_LEN; ++i) {
			assert(var2[i] == 1);
		}
		std::cerr << var1 << std::endl;

		BSP_END()
	}

	int get() {
		return var1;
	}
protected:
	int var1;
	bsp::SharedArray<int> var2;
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

	runner.setup();

	runner.run();

	std::cerr << bsp_pid() << " : " << runner.get() << std::endl;

	if (runner.get() != processors*(processors+1)/2) {
		bsp_abort("Error: incorrect result %i (expt: %i)", 
			runner.get(), processors*(processors+1)/2);
	}


	bsp_end();
	return 0;
}
