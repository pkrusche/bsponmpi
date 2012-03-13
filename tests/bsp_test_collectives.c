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


/** @file bsp_test_collectives.c

@author Peter Krusche
*/

#include "bsp.h"
#include "bsp_level1.h"

#include <assert.h>

#define MIN(l,r) (((l) < (r)) ? (l) : (r))
#define MAX(l,r) (((l) < (r)) ? (r) : (l))

void min_op (void * res, void *l, void * r, int* nbytes) {
	assert (*nbytes == sizeof(int));
	*((int*)res) = MIN(*((int*)l), *((int*)r));
}

void max_op (void * res, void *l, void * r, int* nbytes) {
	assert (*nbytes == sizeof(int));
	*((int*)res) = MAX(*((int*)l), *((int*)r));
}

void sum_op (void * res, void *l, void * r, int* nbytes) {
	assert (*nbytes == sizeof(int));
	*((int*)res) = (*((int*)l) + *((int*)r));
}


int main(int argc, char **argv) {
	int min = -1, max = -1, sum = -1;
	int j, sj, l;
	int q = 25;

	bsp_init(&argc, &argv);

	j = bsp_pid();

	/** 1. Test broadcast from every processor */
	sj = 0;
	for (l = 0; l < bsp_nprocs(); ++l) {
		q = 0;
		sj+= j; // for later
		if (j == l) {
			q = -1;
		}

		bsp_broadcast(l, &q, sizeof(int));

		assert (q == -1);
	}

	/** 2. Test fold. */

	bsp_fold(min_op, &j, &min, sizeof(int));
	bsp_fold(max_op, &j, &max, sizeof(int));
	bsp_fold(sum_op, &j, &sum, sizeof(int));

	assert (j == bsp_pid());
	assert (min == 0);
	assert (max == bsp_nprocs() - 1);
	assert (sj == sum);


	bsp_end();
	return 0;
}
