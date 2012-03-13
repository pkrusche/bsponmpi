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


/** @file bsp_test_cpp_collectives.cpp

@author Peter Krusche
*/


#include "bsp.h"
#include "bsp_level1.h"

#include <stdio.h>
#include <assert.h>

#include <algorithm>

struct min_op {
	int operator() (int l, int r) {
		using namespace std;
		return min(l, r);
	}
};

struct max_op {
	int operator() (int l, int r) {
		using namespace std;
		return max(l, r);
	}
};

struct sum_op {
	int operator() (int l, int r) {
		using namespace std;
		return l + r;
	}
};


int main(int argc, char **argv) {
	using namespace bsp;
	int min = -1, max = -1, sum = -1;
	int j, sj, l;
	int q = 25;

	bsp_init(&argc, &argv);

	j = bsp_pid();

	/** 1. Test broadcast from every processor */
	sj = 0;
	for (l = 0; l < bsp_nprocs(); ++l) {
		std::string s = "ABC";
		q = 0;
		sj+= l; // for later
		if (j == l) {
			s = "dabs";
			q = -1;
		}

		bsp_broadcast (l, q);
		bsp_broadcast (l, s);

		printf ("%i %i %s\n", j, q, s.c_str());

		assert (q == -1);
		assert (s == "dabs");
	}	

	/** 2. Test fold. */

	bsp_fold<int, min_op> (j, min);
	bsp_fold<int, max_op> (j, max);
	bsp_fold<int, sum_op> (j, sum);

	printf("%i %i %i %i %i\n", j, sj, min, max, sum);
	fflush(stdout);

	assert (j == bsp_pid());
	assert (min == 0);
	assert (max == bsp_nprocs() - 1);
	assert (sj == sum);


	bsp_end();
	return 0;
}

