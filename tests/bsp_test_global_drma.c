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

void test_drma() {
    int val = 10*bsp_pid();
    int target = ( bsp_pid() + 1 ) % bsp_nprocs();
    int sum1 = 0;
    int sum2 = 0;
    int * p_arr = bsp_malloc ( bsp_nprocs() *2, sizeof ( int ) );

    bsp_global_handle_t h = bsp_global_alloc ( sizeof ( int ) * bsp_nprocs() * 2 );
    bsp_sync();

    bsp_sync();

    bsp_global_put ( &val, h, 2*sizeof ( int ) *target, sizeof ( int ) );
    bsp_global_put ( &val, h, 2*sizeof ( int ) *target + sizeof ( int ), sizeof ( int ) );

    bsp_sync();

    bsp_global_get ( h, 0, p_arr, 2*sizeof ( int ) * bsp_nprocs() );

    bsp_sync();

    for ( target = 0; target < bsp_nprocs(); ++target ) {
        sum1+= p_arr[2*target] + p_arr[2*target + 1];
        sum2+= 20*target;
    }

    if ( sum1 != sum2 ) {
        printf ( "Global DRMA Failed: %i != %i\n", sum1, sum2 );
    }

    bsp_sync();

    bsp_free ( p_arr );
    bsp_global_free ( h );
}

void bsp_test_get ( void ) {
    test_drma();
}

int main ( int argc, char *argv[] ) {
    bsp_init ( &argc, &argv );
    bsp_test_get ();
	bsp_end();
    return 0;
}
