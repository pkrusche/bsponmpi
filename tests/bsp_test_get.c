/*
BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
Copyright (C) 2006  Wijnand J. Suijlen

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


#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "bsp.h"
#include "bsp_alloc.h"

void a_simple_summation() {
	int *local_sums, i, j, result = 0;
	int xs[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	int nelem = 10;
	const int P = bsp_nprocs ();
	for (j = 0; j < nelem; j++)
		result += xs[j];
	bsp_push_reg (&result, sizeof (int));
	bsp_sync ();
	local_sums = (int *) bsp_malloc (P, sizeof (int));

	for (i = 0; i < P; i++)
		bsp_get (i, &result, 0, &local_sums[i], sizeof (int));

	bsp_sync ();

	result = 0;
	for (i = 0; i < P; i++)
	{
		assert (55 == local_sums[i]);
		result += local_sums[i];
	}

	assert (55 * P == result);

	bsp_pop_reg (&result);

	bsp_sync();
	bsp_free (local_sums);

}

void an_all_to_all()
{
	int P = bsp_nprocs();
	char * array1 = (char*) bsp_malloc(23*P, sizeof(char));
	char * array2 = (char*) bsp_malloc(23*P, sizeof(char));
	char buffer[100] ; 
	const double pi = 3.14159265358979;
	int s = bsp_pid(), i, j;
	bsp_push_reg(array1, 23 * P);
	bsp_push_reg(array2, 23 * P); 
	bsp_sync(); 

	sprintf(array1 + 23*s, "Bla %8.3f en %06d", s / pi, s); 
	for (i = 0; i < P; i++)
	{
		bsp_get(i, array1, i*23, array1 + i*23, 23);
		for (j = 0; j < 23; j++)
			bsp_get(i, array1, i*23 + j, &array2[i*23+j], 1);
	}	

	bsp_sync();
	for (i = 0; i < P; i++)
	{
		sprintf(buffer, "Bla %8.3f en %06d", i / pi, i); 
		assert(strncmp(buffer, array1 + i*23, 23)==0);
		assert(strncmp(buffer, array2 + i*23, 23)==0);
	}  
	bsp_pop_reg(array2);
	bsp_pop_reg(array1);
	bsp_sync();
	bsp_free(array1);
	bsp_free(array2);
}

void bsp_test_get(void) {
	a_simple_summation();
	an_all_to_all();
}


int main (int argc, char *argv[]) {
	bsp_init (&argc, &argv);
	bsp_test_get ();
	bsp_end();
	return 0;
}
