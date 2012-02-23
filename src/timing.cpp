/***************************************************************************
*   Copyright (C) 2009   Peter Krusche, The University of Warwick         *
*   peter@dcs.warwick.ac.uk                                               *
***************************************************************************/

#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else 
#include <unistd.h>
#include <sys/time.h>
#endif

namespace utilities {

double g_t0(-1);

double time() {
	if(g_t0 < 0) {
		g_t0 = 0;
		g_t0 = time();
		return 0.0;
	}
#ifdef _USE_MPI_WTIME
	return MPI_Wtime() - g_t0;
#else
#ifdef _WIN32
	LARGE_INTEGER t;
	LARGE_INTEGER f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	return (((double)t.QuadPart) / ((double)f.QuadPart)) - g_t0;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 1e-6*(((double)tv.tv_usec) + 1000000.0*((double)tv.tv_sec)) - g_t0;
#endif
#endif
}

void warmup(double t) {
	double t0 = utilities::time();
	for(;;) {
		if(utilities::time() - t0 > t) {
			break;
		}
		rand();
	}
}

};

