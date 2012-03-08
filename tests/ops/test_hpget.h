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


/** @file test_get.h

@author Peter Krusche
*/
#ifndef __test_hpget_H__
#define __test_hpget_H__

#include <iostream>
#include <stdlib.h>

#include "bsp_cpp/bsp_cpp.h"
#include "../unittest.h"

class TestHpGet : public bsp::Context {
public:
	void init() {
		var1 = 0;
		memset(var2, 0, sizeof(int)*5);
		var3 = 0;
	}

	static void run( int processors ) {
		using namespace std;
		TestHpGet c;
		BSP_SCOPE(TestHpGet, c, processors);
		BSP_BEGIN();

		bsp_push_reg(&var1, sizeof (int));
		bsp_push_reg(var2, sizeof (int));
		bsp_push_reg(&var3, sizeof (int));

		myval1 = -1;
		myval2 = -1;
		myval3 = -1;

		BSP_SYNC();

		// Nearest Neighbour.

		var1 = bsp_pid() + 1;
		var2[2] = bsp_nprocs() - bsp_pid();
		var3 = bsp_pid() + 1;

		BSP_SYNC();

		bsp_hpget (bsp_nprocs() - 1 - bsp_pid(), &var1, 0, &myval1, sizeof(int));
		bsp_hpget (bsp_nprocs() - 1 - bsp_pid(), var2, 2*sizeof(int), &myval2, sizeof(int));
		bsp_hpget (bsp_nprocs() - 1 - bsp_pid(), &var3, 0, &myval3,  sizeof(int));

		BSP_SYNC();

		CHECK_EQUAL( bsp_nprocs() - bsp_pid(), myval1 );
		CHECK_EQUAL( bsp_pid() + 1, myval2 );
		CHECK_EQUAL( bsp_nprocs() - bsp_pid(), myval3 );
		CHECK_EQUAL( 0 , var2[0] );
		CHECK_EQUAL( 0 , var2[1] );
		CHECK_EQUAL( 0 , var2[3] );
		CHECK_EQUAL( 0 , var2[4] );
		CHECK_EQUAL( bsp_pid() + 1, var1 );
		CHECK_EQUAL( bsp_nprocs() - bsp_pid(), var2[2] );
		CHECK_EQUAL( bsp_pid() + 1, var3 );

		bsp_pop_reg(&var1);
		bsp_pop_reg(var2);
		bsp_pop_reg(&var3);

		a2a_in = new int [bsp_nprocs() * 100];
		a2a_out = new int [bsp_nprocs() * 100];

		memset (a2a_in, 0, bsp_nprocs() * 100 * sizeof(int));
		memset (a2a_out, -1, bsp_nprocs() * 100 * sizeof(int));

		bsp_push_reg(a2a_out, bsp_nprocs() * 100 * sizeof(int));

		BSP_SYNC();

		for (int j = 0; j < bsp_nprocs(); ++j) {
			for (int k = 0; k < 10; ++k) {
				CHECK_EQUAL(0, a2a_in[j*10+k]);
				CHECK_EQUAL(-1, a2a_out[j*10+k]);
				a2a_in [j*10+k] = -1;
				a2a_out[j*10+k] = ( j * bsp_nprocs() + bsp_pid() ) * k;
			}
		}

		BSP_SYNC();

		for (int j = 0; j < bsp_nprocs(); ++j) {
			for (int k = 0; k < 10; ++k) {
				bsp_hpget(j, a2a_out, (bsp_pid()*10 + k)*sizeof(int), a2a_in + (j*10+k), sizeof(int));
			}
		}

		BSP_SYNC();

		for (int j = 0; j < bsp_nprocs(); ++j) {
			for (int k = 0; k < 10; ++k) {
				using namespace std;
				CHECK_EQUAL(
					(bsp_pid() * bsp_nprocs() + j)*k, 
					a2a_in[j*10 + k]
				);
				CHECK_EQUAL(( j * bsp_nprocs() + bsp_pid() ) * k, a2a_out[j*10+k]);
			}
		}

		bsp_pop_reg(a2a_out);

		delete [] a2a_in;
		delete [] a2a_out;

		BSP_END();
	}

protected:
	int var1;
	int var2[5];
	int var3;
	int myval1;
	int myval2;
	int myval3;
	int * a2a_in;
	int * a2a_out;
};


#endif // __test_get_H__
