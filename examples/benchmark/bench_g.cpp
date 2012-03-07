/* 
	bench_g.cpp

#####################################################################
#                                                                   #
#  This file is part of the BSPWrapper package.                     #
#                                                                   #
#  Parts of this code have been adapted from the bspprobe tool      #
#  BSP toolset (http://www.bsp-worldwide.org/implmnts/oxtool),      #
#  which is                                                         #
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
	
	Benchmarking routines concerned with measuring
	the inverse bandwidth (g).	
		
	10/02/2005 Peter Krusche
*/

#ifndef __BENCH_G_CPP__
#define __BENCH_G_CPP__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <math.h>
#include <algorithm>
#include <list>
#include <unistd.h>

using namespace std;

#include "BSPWrapper.h"
#include "BSPUtils.h"
#include "BSPBenchmark.h"

using namespace BSPWrapper;
using namespace BSPUtils;

namespace BSPBenchmark {
const char * fnames[]= {
	"Get",
	"Send",
	"Put",
	"HpPut",
	"HpGet"
};

/*
 * function to determine g from a set of samples.
 * 
 * basic idea: 
 * 	determine the minimum value of g
 *  s_half and h_half are the starting message sizes and
 *  counts where g is 2*g_min at maximum
 *  we will then average over this area. 
 * 
 *  this will only give sensible results when the
 *  communications library can keep the bandwidth
 *  approximately constant for large messages
 * 
 *  small values of s_half indicate a low latency
 *  connection network
 *  
 *  h_half -> s_half indicates that message combining
 *  is present and works well (or bad performance if
 *  both are large)
 *
 * 
 * */
double determine_g(double *vg, int * s, int * h, int len) {
	int s_halfmin= INT_MAX, s_halfmax= INT_MAX, s_max= 0, s_min= INT_MAX;
	int h_halfmin= INT_MAX, h_halfmax= INT_MAX, h_max= 0, h_min= INT_MAX;
	double g_min= 1e80, g_avg= 0;
	double g_stddev= 0;
	list<double> avg_list;
	
	for(int j= 0; j < len; ++j) {
		g_min= min(g_min, vg[j]);
		s_min= min(s_min, s[j]);
		h_min= min(h_min, h[j]);
		s_max= max(s_max, s[j]);
		h_max= max(h_max, h[j]);
	}
	
	s_halfmax= s_max;
	h_halfmax= h_max;
	s_halfmin= s_max;
	h_halfmin= h_max;
	g_min*= 2;
	for(int j= 0; j < len; ++j)
		if(vg[j] <= g_min) {
			if(h[j] == h_min)
				s_halfmin= min(s[j], s_halfmin);
			if(h[j] == h_max)
				s_halfmax= min(s[j], s_halfmax);

			if(s[j] == s_min)
				h_halfmin= min(h[j], h_halfmin);
			if(s[j] == s_max)
				h_halfmax= min(h[j], h_halfmax);

			avg_list.insert(avg_list.begin(), vg[j]);
		}
	
	for (list<double>::iterator it= avg_list.begin();
		it != avg_list.end(); ++it) 
		g_avg+= *it;
	
	g_avg/= avg_list.size();

	for (list<double>::iterator it= avg_list.begin();
		it != avg_list.end(); ++it) 
		g_stddev+= (*it - g_avg)*(*it - g_avg);

	g_stddev/= avg_list.size() - 1;
	
	g_stddev= sqrt(g_stddev);
	Print(0, "\n\tdetermine_g: \n\t\ts_half_min= %i s_half_max= %i\n\t\th_half_min= %i h_half_max= %i\n\t\tg_min= %4.3g, stddev: %4.3g\n\n", 
				s_halfmax, s_halfmin, h_halfmax, h_halfmin, g_min, g_stddev);
	
	return g_avg;
}


double   measure_g_alltoall(int _size, int _h, int whichfunction, double fac, int sub, bool diag) {
  NUMERIC *src, *dst;
  int i,j,oversample;
  int k;
  double time_clockA, time_clockB;
  double h2s= 2.0;
  
  int vgs= 0;
  
  for (int size= _size; size > 0; size= (int)(size*fac - sub)) 
	  for (int h= diag ? (int)(h2s * size) : _h; h >= (diag ? (int)std::max(h2s * size, 1.0) : 1 ); h = (int)(h*fac - sub)) 
	  	++vgs;

  double* vg= new double[vgs];
  int* sg= new int[vgs];
  int* hg= new int[vgs];
  memset(vg, 0, sizeof(double)*vgs);
  memset(sg, 0, sizeof(int)*vgs);
  memset(hg, 0, sizeof(int)*vgs);
  
  src = (NUMERIC*)calloc(_size*P,sizeof(NUMERIC));
  dst = (NUMERIC*)calloc(_size*_h*P,sizeof(NUMERIC));
  if (src==NULL || dst==NULL)
    ERROR("{measureHRelationTime} unable to allocate memory");

  if(whichfunction == HR_FN_PUT || whichfunction == HR_FN_HPPUT)
	  PushReg(dst,_size*_h*P*sizeof(NUMERIC));

  if(whichfunction == HR_FN_GET || whichfunction == HR_FN_HPGET)
	  PushReg(src,_size*P*sizeof(NUMERIC));

  Sync();

  int _vgo= 0;
  for (int size= _size; size > 0; size= (int)(size*fac - sub)) {
	  for (int h= diag ? (int)(h2s * size) : _h; h >= (diag ? (int)std::max(h2s * size, 1.0) : 1 ); h = (int)(h*fac - sub)) {
		  oversample = (int) (1 * _size / size * _h / h);
		  if (oversample > 10) 
		  	oversample= 10;
		  if (oversample < 1) 
		  	oversample= 1;

		  time_clockA = bsp_dtime();
		  for(i=0;i<oversample;i++) {
		      for(k= 0;k < h*size;k+=size) {
			    for(j=0;j<P;j++) {
				      		
		      	int tag= ((Pid*h*size)+k)*sizeof(NUMERIC);
		      	switch(whichfunction) {
		      		case HR_FN_PUT:
				        Put(j,
		                    &src[j*size],
			                dst,
			                tag,
			                size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_HPPUT:
				        HpPut(j,
				              &src[j*size],
				              dst,
				              tag,
				              size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_GET:
				        Get((int)j,
				            src,
				            j*size*sizeof(NUMERIC),
				            dst+tag/sizeof(NUMERIC),
				            size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_HPGET:
				        HpGet((int)j,
				                  src,
				                  j*size*sizeof(NUMERIC),
				                  dst+tag/sizeof(NUMERIC),
				                  size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_SEND:
				        Send((int)j,
				        		  &tag,
				                  &src[j*size],
				                  size*sizeof(NUMERIC));
				        break;
				    default:
				    	break;
		      	}
		      }
		    }
		    Sync();
		    if(whichfunction == HR_FN_SEND) {
		    	int status, tag;
		    	
		    	GetTag(status, &tag);
		    	while(status > 0) {
			    	Move(dst+tag/sizeof(NUMERIC), status);
		    		GetTag(status, &tag);
		    	}
		    }
		  }
		
		  time_clockA = bsp_dtime();
		  
		  Fold( &time_clockA,&time_clockB,sizeof(double), 
		  	BSP_OPFUN dbl_max);
		
		  double g_time= (time_clockB / oversample) / (h * size * max(1,P-1));

		  sg[_vgo]= size;	  
		  hg[_vgo]= h;
		  vg[_vgo++]= g_time;	  
	  }
  }
  
  if(whichfunction == HR_FN_PUT || whichfunction == HR_FN_HPPUT)
	  PopReg(dst);

  if(whichfunction == HR_FN_GET || whichfunction == HR_FN_HPGET)
	  PopReg(src);

  Sync();
  free(dst);free(src);

  
  if(Pid == 0) {
	FILE * fp;
  	char filename[512];
    sprintf(filename, "bspprobe_%s_p%i_%s_alltoall.dat", BSPWRAPPER_ARCH, P, fnames[whichfunction]);
  	fp= fopen(filename, "w+");

	int lsg= 0;
  	for(int vgo= 0; vgo < vgs; ++vgo) {
  		if(lsg != sg[vgo]) {
			fprintf(fp, "\n");
			lsg= sg[vgo];
  		}
		fprintf(fp, "%i\t%i\t%g\n", sg[vgo], hg[vgo], vg[vgo]);
  	}
  	fclose(fp);
  }
  
  double g= determine_g(vg, sg, hg, vgs);	
  delete [] vg;
  delete [] sg;
  delete [] hg;
  
  return g;
}

double   measure_g_random(int _size, int _h, int whichfunction, double fac, int sub, bool diag) {
  NUMERIC *src, *dst;
  int i,j,oversample;
  int k;
  double time_clockA, time_clockB;
  double h2s= 2.0;

  int pa[P];
  int vgs= 0;
  
  for (int size= _size; size > 0; size= (int)(size*fac - sub)) 
	  for (int h= diag ? (int)(h2s * size) : _h; h >= (diag ? (int)std::max(h2s * size, 1.0) : 1 ); h = (int)(h*fac - sub)) 
	  	++vgs;

  double* vg= new double[vgs];
  int* sg= new int[vgs];
  int* hg= new int[vgs];
  memset(vg, 0, sizeof(double)*vgs);
  memset(sg, 0, sizeof(int)*vgs);
  memset(hg, 0, sizeof(int)*vgs);

  src = (NUMERIC*)calloc(_size*_h,sizeof(NUMERIC));
  dst = (NUMERIC*)calloc(_size*_h,sizeof(NUMERIC));
  if (src==NULL || dst==NULL)
    ERROR("{measureHRelationTime} unable to allocate memory");

  if(whichfunction == HR_FN_PUT || whichfunction == HR_FN_HPPUT)
	  PushReg(dst,_size*_h*sizeof(NUMERIC));

  if(whichfunction == HR_FN_GET || whichfunction == HR_FN_HPGET)
	  PushReg(src,_size*_h*sizeof(NUMERIC));

  Sync();

  int _vgo= 0;
  for (int size= _size; size > 0; size=(int)(size*fac - sub))
	  for (int h= diag ? (int)(h2s * size) : _h; h >= (diag ? (int)std::max(h2s * size, 1.0) : 1 ); h = (int)(h*fac - sub)) {
		  oversample = (int) (1 * _size / size * _h / h);
		  if (oversample > 10) 
		  	oversample= 10;
		  if (oversample < 1) 
		  	oversample= 1;
		  time_clockA = bsp_dtime();
		  
		  for(i=0;i<oversample;i++) {
			  randompermutation(pa, P);
			  j= pa[Pid];
		      for(k=0;k < h*size;k+=size) {
		      	int tag= k*sizeof(NUMERIC);
		      	switch(whichfunction) {
		      		case HR_FN_PUT:
				        Put(j,
		                    src+tag/sizeof(NUMERIC),
			                dst,
			                tag,
			                size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_HPPUT:
				        HpPut(j,
				              src+tag/sizeof(NUMERIC),
				              dst,
				              tag,
				              size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_GET:
				        Get((int)j,
				            src,
				            tag,
				            dst+tag/sizeof(NUMERIC),
				            size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_HPGET:
				        HpGet((int)j,
				                  src,
				                  tag,
				                  dst+tag/sizeof(NUMERIC),
				                  size*sizeof(NUMERIC));
				        break;
		      		case HR_FN_SEND:
				        Send((int)j,
				        		  &tag,
				                  src+tag/sizeof(NUMERIC),
				                  size*sizeof(NUMERIC));
				        break;
				    default:
				    	break;
		      	}
		    }
		    Sync();
		    if(whichfunction == HR_FN_SEND) {
		    	int status, tag;
		    	
		    	GetTag(status, &tag);
		    	while(status > 0) {
			    	Move(dst+tag/sizeof(NUMERIC), status);
		    		GetTag(status, &tag);
		    	}
		    }
		  }
		
		  time_clockA = bsp_dtime();
		  
		  Fold( &time_clockA,&time_clockB,sizeof(double), 
		  	BSP_OPFUN dbl_max);
		
		  double g_time= (time_clockB / oversample) / (h * size);
		  sg[_vgo]= size;	  
		  hg[_vgo]= h;
		  vg[_vgo++]= g_time;	  
	  }

  if(whichfunction == HR_FN_PUT || whichfunction == HR_FN_HPPUT)
	  PopReg(dst);

  if(whichfunction == HR_FN_GET || whichfunction == HR_FN_HPGET)
	  PopReg(src);

  Sync();
  free(dst);free(src); 
  
  if(Pid == 0) {
	FILE * fp;
  	char filename[512];
    sprintf(filename, "bspprobe_%s_p%i_%s_random.dat", BSPWRAPPER_ARCH, P, fnames[whichfunction]);
  	fp= fopen(filename, "w+");

	int lsg= 0;
  	for(int vgo= 0; vgo < vgs; ++vgo) {
  		if(lsg != sg[vgo]) {
			fprintf(fp, "\n");
			lsg= sg[vgo];
  		}
		fprintf(fp, "%i\t%i\t%g\n", sg[vgo], hg[vgo], vg[vgo]);
  	}
  	fclose(fp);
  }
  
  double g= determine_g(vg, sg, hg, vgs);	
  delete [] vg;
  delete [] sg;
  delete [] hg;
  return g;
}


}


#endif
