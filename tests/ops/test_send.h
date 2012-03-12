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


/** @file test_send.h

@author Peter Krusche
*/
#ifndef __test_send_H__
#define __test_send_H__

#include <iostream>
#include <stdlib.h>

#include "bsp_cpp/bsp_cpp.h"
#include "../unittest.h"

class TestSend : public bsp::Context {
public:
	void init() {
		var1 = 0;
		var2 = 0;
		var3 = 0;
	}

	static void run( int processors ) {
		using namespace std;
		TestSend c;
		BSP_SCOPE(TestSend, c, processors);
		BSP_BEGIN();

		myval[0] = -1;
		myval[1] = -1;
		myval[2] = -1;
		s = bsp_pid() * 10;
		
		size_t s = sizeof(int);
		bsp_set_tagsize(&s);

		BSP_SYNC();

		// Nearest Neighbour.

		var1 = bsp_pid() + 1;
		var2 = bsp_nprocs() - bsp_pid();
		var3 = bsp_pid() + 1;

		bsp_send (bsp_nprocs() - 1 - bsp_pid(), &s, &var1, sizeof(int));
		++s;
		bsp_send (bsp_nprocs() - 1 - bsp_pid(), &s, &var2, sizeof(int));
		++s;
		bsp_send (bsp_nprocs() - 1 - bsp_pid(), &s, &var3, sizeof(int));

		BSP_SYNC();
		
		int messages = -1;
		size_t bytes = -1;
		
		bsp_qsize (&messages, &bytes);
		
		CHECK_EQUAL(10*bsp_pid() + 2, s);
		CHECK_EQUAL(3, messages);
		CHECK_EQUAL(3*sizeof(int), bytes);

		int status = -1;
		int tag = -1;
		int szint = sizeof(int);
		
		for (int k = 0; k < 3; ++k) {
			bsp_get_tag(&status, &tag);
			CHECK_EQUAL(szint, status);
			CHECK( (tag%10) >= 0 && (tag%10) < 3 );
			CHECK_EQUAL( bsp_nprocs() - 1 - bsp_pid(), tag/10 );

			bsp_move( myval + (tag % 10), status );
		}

		CHECK_EQUAL( bsp_nprocs() - bsp_pid(), myval [0] );
		CHECK_EQUAL( bsp_pid() + 1, myval [1] );
		CHECK_EQUAL( bsp_nprocs() - bsp_pid(), myval[2] );

#ifdef MACCAROON
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

		for (int j = 0; j < bsp_nprocs(); ++j) {
			for (int k = 0; k < 10; ++k) {
				bsp_get(j, a2a_out, (bsp_pid()*10 + k)*sizeof(int), a2a_in + (j*10+k), sizeof(int));
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
#endif
		BSP_END();
	}

protected:
	int var1;
	int var2;
	int var3;
	int myval[3];

	int s;
	int * a2a_in;
	int * a2a_out;
};


#endif // __test_get_H__
