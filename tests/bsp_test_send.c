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
#include <stdio.h>
#include <assert.h>
#include <string.h>

void 
	a_simple_summation()
{
	int sum = 0, i, status, a;
	size_t b;
	const int *p;
	const int P = bsp_nprocs();
	int nmesg = P, accumsz = P * sizeof(int);
	const int s = bsp_pid();
	void *voidptr;

	for (i = 0; i < P; i++)
		bsp_send(i, NULL, &s, sizeof(int));

	bsp_sync();
	while (bsp_get_tag(&status, NULL), status != -1)
	{
		bsp_qsize(&a, &b);
		assert(a == nmesg);
		assert(b == accumsz);
		bsp_hpmove(&voidptr, (void **) &p);
		sum += *p;
		nmesg--;
		accumsz -= sizeof(int);
	}  
	bsp_sync();  
	assert(sum == (P-1)*P / 2);
}


void just_messages () {
	char buf1[100], buf2[100];
	int status, count = 0, tag;
	size_t tag_size = sizeof(int);
	const int P = bsp_nprocs(), s = bsp_pid();

	bsp_set_tagsize(&tag_size);
	bsp_sync();

	sprintf (buf1, "Hallo! Hier processor %d", bsp_pid ());
	bsp_send (0, &s, buf1, strlen (buf1) + 1);

	bsp_sync ();

	if (bsp_pid () == 0)
	{
		while (bsp_get_tag (&status, &tag), status != -1)
		{
			bsp_move (buf1, 100);
			sprintf(buf2, "Hallo! Hier processor %d", count);
			assert (tag == count);
			assert (strncmp(buf1, buf2, 100) == 0);
			count++;
		}
		assert (P == count);
	}
}

void bsp_test_send(void)
{
	a_simple_summation();
	just_messages();
}

int	main (int argc, char *argv[])
{
	bsp_init (&argc, &argv);
	bsp_test_send ();
	bsp_end();
	return 0;
}
