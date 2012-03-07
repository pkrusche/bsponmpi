/* 
	bench_f.cpp

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

	
	Conversion of bspprobe (Oxtool) benchmarking
	functions, extended for different types and
	use of BLAS for double values
		
	10/02/2005 Peter Krusche
*/

#ifndef __BENCH_F_CPP__
#define __BENCH_F_CPP__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>

#ifdef _USE_BLAS
extern "C" {
#include "cblas.h"
}
#endif

using namespace std;

#include "BSPWrapper.h"
#include "BSPUtils.h"

using namespace BSPWrapper;
using namespace BSPUtils;

#define MIN_SAMPLE          10     /*    10   */
#define S_DOT_OVERSAMPLE    20     /*    20   */
#define S_MAT_OVERSAMPLE    10     /*    10   */
#define DOT_PRODUCT_SIZE 1048570   /* 1048570 */
#define MATRIX_SIZE         100    /* 100, s.t n^2 not power of 2 */

namespace BSPBenchmark {

double measure_f_dot_int() 
{
  int i,j,up=1,dummy=0;
  double time_clockA, time_clockB;
  int dot_product = 0;
  int *vecA;
  int *vecB;

  vecA = (int *)calloc(DOT_PRODUCT_SIZE,sizeof(int));
  vecB = (int *)calloc(DOT_PRODUCT_SIZE,sizeof(int));

  if (vecA==NULL || vecB==NULL)
    ERROR("Failed to allocate vector for dot product used in S");

  for (i=0; i < DOT_PRODUCT_SIZE; i++) {
     vecA[i] = 1;
     vecB[i] = 2;
  }
  Sync();
  time_clockA = Time();
  for (i=0;i<S_DOT_OVERSAMPLE;i++) {
    if (up) {
      for (j=0; j < DOT_PRODUCT_SIZE; j++) 
        dot_product += vecA[j] * vecB[j]; 
      up = 0;
    } else {
      for (j=DOT_PRODUCT_SIZE-1; j>=0;j--)
        dot_product += vecA[j] * vecB[j]; 
      up = 1;
    }
  }
  if (dot_product>0) {
     /* Need this conditional, otherwise the optimiser removes the
        dot-product as it isnt used in the code from here on */
     time_clockA = bsp_dtime();
  }
  Fold( &time_clockA,&time_clockB,sizeof(double), 
  	BSP_OPFUN dbl_max);
  
  free(vecA); free(vecB);

  /* 2 flops * over_sample * scale factor of 5 * dot_product_size */
  return (double) time_clockB/(2*S_DOT_OVERSAMPLE*DOT_PRODUCT_SIZE);
}


double measure_f_dot_double() 
{
  int i,j,up=1,dummy=0;
  double time_clockA, time_clockB;

  double dot_product = 0.0;
  double *vecA;
  double *vecB;

  vecA = (double *)calloc(DOT_PRODUCT_SIZE,sizeof(double));
  vecB = (double *)calloc(DOT_PRODUCT_SIZE,sizeof(double));

  if (vecA==NULL || vecB==NULL)
    ERROR("Failed to allocate vector for dot product used in S");

  for (i=0; i < DOT_PRODUCT_SIZE; i++) {
     vecA[i] = 0.01;
     vecB[i] = 0.02;
  }
  Sync();
  time_clockA = Time();
  for (i=0;i<S_DOT_OVERSAMPLE;i++) {
 #ifndef _USE_BLAS
    if (up) {
      for (j=0; j < DOT_PRODUCT_SIZE; j++) 
        dot_product += vecA[j] * vecB[j]; 
      up = 0;
    } else {
      for (j=DOT_PRODUCT_SIZE-1; j>=0;j--)
        dot_product += vecA[j] * vecB[j]; 
      up = 1;
    }
#else
	dot_product+= cblas_ddot(DOT_PRODUCT_SIZE, vecA, 1, vecB, 1);
#endif
  }
  if (dot_product>0) {
     /* Need this conditional, otherwise the optimiser removes the
        dot-product as it isnt used in the code from here on */
     time_clockA = bsp_dtime();
  }
  Fold( &time_clockA,&time_clockB,sizeof(double), 
  	BSP_OPFUN dbl_max);
  
  free(vecA); free(vecB);

  /* 2 flops * over_sample * scale factor of 5 * dot_product_size */
  return (double) time_clockB/(2*S_DOT_OVERSAMPLE*DOT_PRODUCT_SIZE);
}

double measure_f_matmul_int() 
{
  int i,j,k,o;
  int **matA, **matB, **matC, **tempmat, *vec, s,fool_optimiser;
  double time_clockA, time_clockB;

  matA = (int**)calloc(MATRIX_SIZE,sizeof(int*));
  matB = (int**)calloc(MATRIX_SIZE,sizeof(int*));
  matC = (int**)calloc(MATRIX_SIZE,sizeof(int*));
  if (matA==NULL || matB==NULL || matC==NULL)
    ERROR("Failed to allocate matrix");

  matA[0] = (int*)calloc(MATRIX_SIZE*MATRIX_SIZE,sizeof(int));
  matB[0] = (int*)calloc(MATRIX_SIZE*MATRIX_SIZE,sizeof(int));
  matC[0] = (int*)calloc(MATRIX_SIZE*MATRIX_SIZE,sizeof(int));
  if (matA[0]==NULL || matB[0]==NULL || matC[0]==NULL)
    ERROR("Failed to allocate matrix vector");
  for(i=1;i<MATRIX_SIZE;i++) {
    matA[i]=matA[0]+(MATRIX_SIZE*i);
    matB[i]=matB[0]+(MATRIX_SIZE*i);
    matC[i]=matC[0]+(MATRIX_SIZE*i);
  }
  for(i=0;i<MATRIX_SIZE;i++) {
    for(j=0;j<MATRIX_SIZE;j++) {
      matA[i][j] = (int) (j+1);
      matB[i][j] = (int) (i+1);
      matC[i][j] = 0;
    }
  }
  Sync();
  fool_optimiser=0;
  bsp_dtime();
  for (o=0;o<S_MAT_OVERSAMPLE;o++) {
    for(j=0;j<MATRIX_SIZE;j++) {
      for(i=0;i<MATRIX_SIZE;i++) {
        s=0;
        for(k=0;k<MATRIX_SIZE;k++) {
          s += matA[i][k] * matB[j][k];
        }
        matC[j][i] = s;
      }
    }
    fool_optimiser+=matC[0][0];
  }
  if (fool_optimiser<=0) ERROR("Optimiser being too clever");
  time_clockA = bsp_dtime();
  Fold( &time_clockA,&time_clockB,sizeof(double), 
  	BSP_OPFUN dbl_max);

  
  free(matA[0]); free(matB[0]);free(matC[0]);
  free(matA);    free(matB);   free(matC);

  /* 2 flops * over_sample * scale factor of 5 * dot_product_size */
  return 1.0/(((double) 
                 (2*S_MAT_OVERSAMPLE*MATRIX_SIZE) / time_clockB)*
                 (MATRIX_SIZE*MATRIX_SIZE));
 }

double measure_f_matmul_double() 
{
  int i,j,k,o;
  double **matA, **matB, **matC, **tempmat, *vec, s,fool_optimiser;
  double time_clockA, time_clockB;

  matA = (double**)calloc(MATRIX_SIZE,sizeof(double*));
  matB = (double**)calloc(MATRIX_SIZE,sizeof(double*));
  matC = (double**)calloc(MATRIX_SIZE,sizeof(double*));
  if (matA==NULL || matB==NULL || matC==NULL)
    ERROR("Failed to allocate matrix");

  matA[0] = (double*)calloc(MATRIX_SIZE*MATRIX_SIZE,sizeof(double));
  matB[0] = (double*)calloc(MATRIX_SIZE*MATRIX_SIZE,sizeof(double));
  matC[0] = (double*)calloc(MATRIX_SIZE*MATRIX_SIZE,sizeof(double));
  if (matA[0]==NULL || matB[0]==NULL || matC[0]==NULL)
    ERROR("Failed to allocate matrix vector");
  for(i=1;i<MATRIX_SIZE;i++) {
    matA[i]=matA[0]+(MATRIX_SIZE*i);
    matB[i]=matB[0]+(MATRIX_SIZE*i);
    matC[i]=matC[0]+(MATRIX_SIZE*i);
  }
  for(i=0;i<MATRIX_SIZE;i++) {
    for(j=0;j<MATRIX_SIZE;j++) {
      matA[i][j] = (double) (j+1);
      matB[i][j] = (double) (i+1);
      matC[i][j] = 0.0;
    }
  }
  Sync();
  fool_optimiser=0.0;
  bsp_dtime();
  for (o=0;o<S_MAT_OVERSAMPLE;o++) {
#ifndef _USE_BLAS
    for(j=0;j<MATRIX_SIZE;j++) {
      for(i=0;i<MATRIX_SIZE;i++) {
        s=0.0;
        for(k=0;k<MATRIX_SIZE;k++) {
          s += matA[i][k] * matB[j][k];
        }
        matC[j][i] = s;
      }
    }
#else
	cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,MATRIX_SIZE,MATRIX_SIZE,MATRIX_SIZE,1.0,
				matA[0],MATRIX_SIZE,matB[0],MATRIX_SIZE,
				0.0,matC[0],MATRIX_SIZE);
#endif
    fool_optimiser+=matC[0][0];
  }
  if (fool_optimiser<=0.0) ERROR("Optimiser being too clever");
  time_clockA = bsp_dtime();
  Fold( &time_clockA,&time_clockB,sizeof(double), 
  	BSP_OPFUN dbl_max);

  
  free(matA[0]); free(matB[0]);free(matC[0]);
  free(matA);    free(matB);   free(matC);

  /* 2 flops * over_sample * scale factor of 5 * dot_product_size */
  return 1.0/(((double) 
                 (2*S_MAT_OVERSAMPLE*MATRIX_SIZE) / time_clockB)*
                 (MATRIX_SIZE*MATRIX_SIZE));
 }


}
#endif
