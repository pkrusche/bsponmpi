/*
  ###########################################################################
  ##      BSPedupack Version 1.0                                           ##
  ##      Copyright (C) 2004 Rob H. Bisseling                              ##
  ##                                                                       ##
  ##      BSPedupack is released under the GNU GENERAL PUBLIC LICENSE      ##
  ##      Version 2, June 1991 (given in the file LICENSE)                 ##
  ##                                                                       ##
  ###########################################################################
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bsp.h"

#define SZDBL (sizeof(double))
#define SZINT (sizeof(int))
#define TRUE (1)
#define FALSE (0)
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

double *vecallocd(int n);
int *vecalloci(int n);
double **matallocd(int m, int n);
void vecfreed(double *pd);
void vecfreei(int *pi);
void matfreed(double **ppd);
