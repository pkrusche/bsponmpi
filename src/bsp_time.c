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

/** @file bsp_time.c
	Implements the function bsp_time independently of platform and
	availability of MPI.
    @author Peter Krusche
*/

#include "bsp_config.h"

#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else 
#include <unistd.h>
#include <sys/time.h>
#endif

double bsp_begintime = 0;

/**
 * @brief return the time elapsed since computation has started in seconds.
 */
double BSP_CALLING bsp_time() {
#ifdef _USE_MPI_WTIME
	return MPI_Wtime() - bsp.begintime;
#else
#ifdef _WIN32
	LARGE_INTEGER t;
	LARGE_INTEGER f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	return (((double)t.QuadPart) / ((double)f.QuadPart)) - bsp_begintime;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 1e-6*(((double)tv.tv_usec) + 1000000.0*((double)tv.tv_sec)) - bsp_begintime;
#endif
#endif
}

/**
 * @brief Waste some time. Useful for benchmarking on systems that clock
 *        down when idle.
 */

void BSP_CALLING bsp_warmup(double t) {
	double t0 = bsp_time();
	srand(0);
	while (bsp_time() - t0 < t) {
		rand();
	}
	srand(0);
}
