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


/** @file bench_l.cpp

Conversion of original bspprobe (Oxtool) functions
for measuring the latency to BSPonMPI. Added alltoall-type
measurement.

@author Peter Krusche
*/

#include "bsp_cpp/bsp_cpp.h"

using namespace std;

#define MIN_SAMPLE			10  
#define L_OVERSAMPLE        100

namespace bsp_benchmark {

int measure_sync_oversample() {
	int oversample;
	static double oversample_scale;
	double tA, tB;
	
	if(oversample_scale < 0) {
		bsp_dtime();
		for(int i=0;i<500;i++) 
			
			;
		
		tA = bsp_dtime();
		Fold(&tA,&tB,sizeof(double), BSP_OPFUN dbl_max);
		oversample_scale = (1.0/tB)/(L_OVERSAMPLE/500.0);
	}
	
	oversample= max((int)(L_OVERSAMPLE*oversample_scale), MIN_SAMPLE);
	return oversample;
}

double measure_l_nocomm() {
	int oversample= measure_sync_oversample();
	
	for (int i=0; i<10;i++) 
		Sync(); /* Warm things up ;-) */
	  	
	double time_clockA = bsp_dtime();
	for (int i=0; i < oversample; i++) 
		 Sync();  
	
	time_clockA = bsp_dtime();
	double time_clockB;
	Fold( &time_clockA,&time_clockB,sizeof(double), 
		BSP_OPFUN dbl_min);
	return (time_clockB / oversample);
}

double measure_l_localshift() {
	int right, 
		oversample= measure_sync_oversample();
	int junk;

	PushReg(&junk,sizeof(int));
	Sync();
	for (int i=0; i<10;i++) 
		Sync(); /* Warm things up ;-) */
  
 	right = (Pid+1)%(P);
	double time_clockA = bsp_dtime(), time_clockB;
	for (int i=0; i < oversample; i++) {
		HpPut(right,&junk,&junk,0,sizeof(int));
		Sync();  
	}
	time_clockA = bsp_dtime();
	Fold( &time_clockA,&time_clockB,sizeof(double), 
  		  BSP_OPFUN dbl_min);
 
	PopReg(&junk);
	return (time_clockB / oversample);
}

double measure_l_alltoall() 
{
	int oversample= measure_sync_oversample();
	int junk[P];

	PushReg(junk,sizeof(int)*P);
	Sync();
	for (int i=0; i<10;i++) 
		Sync(); /* Warm things up ;-) */
  
	double time_clockA = bsp_dtime(), time_clockB;
	for (int i=0; i < oversample; i++) {
		for(int j= 0; j < P; ++j)
			HpPut(j,&junk[j],&junk, j*sizeof(int), sizeof(int));
		Sync(); 
	}
	time_clockA = bsp_dtime();
	Fold( &time_clockA,&time_clockB,sizeof(double), 
  		  BSP_OPFUN dbl_min);
 
	PopReg(junk);
	return (time_clockB / oversample);
}

}
