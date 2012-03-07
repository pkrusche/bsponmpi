/* 
	bench_l.cpp

#####################################################################
#                                                                   #
#  This file is part of the BSPWrapper package.                     #
#                                                                   #
#  Parts of this code have been adapted from the bspprobe tool      #
#  from the Oxford BSP toolset                                      #
#  (http://www.bsp-worldwide.org/implmnts/oxtool), which is         #
#                                                                   #
#  Copyright (C) 1995,1996,1997 University of Oxford                #
#                                                                   #
# Permission to use, copy, modify, and distribute this software,    #
# and to incorporate it, in whole or in part, into other software,  #
# is hereby granted without fee, provided that                      #
#   (1) the above copyright notice and this permission notice       #
#       appear in all copies of the source code, and the above      #
#       copyright notice appear in clearly visible form on all      #
#       supporting documentation and distribution media;            #
#   (2) modified versions of this software be accompanied by a      #
#       complete change history describing author, date, and        #
#       modifications made; and                                     #
#   (3) any redistribution of the software, in original or modified #
#       form, be without fee and subject to these same conditions.  #
#                                                                   # 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,  #
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES  #
#  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND         #
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT      #
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,     #
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     #
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR    #
#  OTHER DEALINGS IN THE SOFTWARE.                                  #
#                                                                   #
#                                                                   #
#                                                                   #
#####################################################################
	
	Conversion of original bspprobe (Oxtool) functions
	for measuring the latency to BSPWrapper. Added alltoall
	measurement
		
	10/02/2005 Peter Krusche
*/

#include "BSPWrapper.h"
#include "BSPUtils.h"
#include "BSPBenchmark.h"

using namespace std;
using namespace BSPWrapper;
using namespace BSPUtils;

#define MIN_SAMPLE			10  /* 10 */
#define L_OVERSAMPLE        100 /* 100 */

namespace BSPBenchmark {

int measure_sync_oversample() {
	int oversample;
	static double oversample_scale;
	double tA, tB;
	
	if(oversample_scale < 0) {
		bsp_dtime();
		for(int i=0;i<500;i++) 
			Sync();
		
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
