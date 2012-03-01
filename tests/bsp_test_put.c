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

#include "bsp.h"
#include "bsp_alloc.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define BUFLEN 1024

void 
	a_simple_summation()
{
	int i, numbers[10], sum = 0, *results;

	/* register memory */
	results =
		(int *) bsp_malloc (bsp_nprocs (), sizeof (int));
	bsp_push_reg (results, sizeof (int) * bsp_nprocs ());

	/* initialize array */
	for (i = 0; i < 10; i++)
		numbers[i] = (i * (bsp_pid () + 1) + 1) * 10;

	bsp_sync ();			/* to ensure registration */
	for (i = 0; i < 10; i++)
		sum += numbers[i];

	assert (450 * bsp_pid () + 550 == sum);

	for (i = 0; i < bsp_nprocs (); i++)
		bsp_put (i, &sum, results, bsp_pid () * sizeof (int), sizeof (int));
	bsp_sync ();

	for (i = 0; i < bsp_nprocs (); i++)
		assert (450 * i + 550 == results[i]);	/* assert results */
	sum = 0;
	for (i = 0; i < bsp_nprocs (); i++)
		sum += results[i];
	/* assert summation */
	assert ((550 + 450 * (bsp_nprocs () - 1) / 2) * bsp_nprocs () == sum);

	bsp_pop_reg (results);
	bsp_sync ();

	bsp_free (results);
}

void an_all_to_all()
{
	const int P = bsp_nprocs();
	char * array1 = bsp_malloc(P*23, 1), * array2 = bsp_malloc(P*23, 1), buffer[100] ; 
	const double pi = 3.14159265358979;
	int s = bsp_pid(), i, j;
	bsp_push_reg(array1, 23 * P);
	bsp_push_reg(array2, 23 * P); 
	bsp_sync(); 

	sprintf(array1 + 23*s, "Bla %8.3f en %06d", s / pi, s); 
	for (i = 0; i < P; i++)
	{
		bsp_put(i, array1 + 23*s, array1, s * 23, 23);
		for (j = 0; j < 23; j++)
			bsp_put(i, array1 + 23*s + j, array2, s * 23 + j, 1);
	}	

	bsp_sync();
	for (i = 0; i < P; i++)
	{
		sprintf(buffer, "Bla %8.3f en %06d", i / pi, i); 
		assert(strncmp(buffer, array1 + 23*i, 23)==0);
		assert(strncmp(buffer, array2 + 23*i, 23)==0);
	}  
	bsp_pop_reg(array2);
	bsp_pop_reg(array1);
	bsp_sync();
	bsp_free(array1);
	bsp_free(array2);
}

void bsp_test_put(void)
{
	a_simple_summation();
	an_all_to_all(); 
}

int	main (int argc, char *argv[]) {
	bsp_init (&argc, &argv);
	bsp_test_put ();
	bsp_end();
	return 0;
}
