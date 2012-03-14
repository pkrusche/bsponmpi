/*
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
    Copyright (C) 2006  Wijnand J. Suijlen, 2012, Peter Krusche
                                                                                
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

#include "bsp_config.h"

#include <iostream>
#include <climits>

#include "bsp_cpp/bsp_cpp.h"
#include "bsp_cpp/Shared/SharedVariable.h"

#define K 10

int main (int argc, char ** argv) {
	using namespace bsp;
	using namespace std;

	bsp_init (&argc, &argv);
	
	SharedVariableSet s_top, s_locals[K];

	int i_in = bsp_pid();
	int j_in = bsp_pid() + 1;
	std::string k_in = "Initial value.";

	int i[K], j[K];
	std::string k [K];

	memset (i, -1, sizeof(int) * K);
	memset (j, -1, sizeof(int) * K);

	/** 1. create variable maps */

	SHARE_VARIABLE_IR (s_top, bsp::ReduceSum,       0, i_in, int);
	SHARE_VARIABLE_R (s_top,  bsp::ReduceMax, INT_MIN, j_in, int);
	SHARE_VARIABLE_I (s_top, k_in, std::string);

	for (int p = 0; p < K; ++p) {
		// trickery to map array elements to the same slot as i_in/j_in
		int & i_in (i[p]);
		int & j_in (j[p]);
		std::string & k_in(k[p]);

		SHARE_VARIABLE_IR (s_locals[p], bsp::ReduceSum, 0, i_in, int);
		SHARE_VARIABLE_R (s_locals[p],  bsp::ReduceMax, INT_MIN, j_in,  int);
		SHARE_VARIABLE_I (s_locals[p], k_in, std::string);
	}

	/** 2. connect variable maps */

	for (int p = 0; p < K; ++p) {
		s_top.add_as_children(s_locals[p]);
	}

	s_top.initialize_all( 0 );

	for (int p = 0; p < K; ++p) {
		assert (i[p] == 0);
		assert (j[p] == -1);
		assert (k[p] == "Initial value.");
	}
	
	for (int p = 0; p < K; ++p) {
		i[p] = 10 * bsp_pid() + p;
		j[p] = rand() % 100;
	}

	/** validation... */
	int isum = 0;
	int jmax = 0;
	for (int p = 0; p < bsp_nprocs(); ++p) {
		for (int k = 0; k < K; ++k) {
			isum += 10 * p + k;
			jmax = max(jmax, j[k]);
		}
	}
	
	std::cout << bsp_pid() << ": " << i_in << " " << j_in << " " << isum << " " << jmax << std::endl;
	
	s_top.reduce_all();

	std::cout << bsp_pid() << ": "  << i_in << " " << j_in << " " << isum << " " << jmax << std::endl;

	std::cout.flush();
	
	bsp_sync();

	assert (isum == i_in);
	assert (jmax == j_in);

	bsp_end();

	return 0;
}
