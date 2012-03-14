/*
BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
Copyright (C) 2006  Wijnand J. Suijlen, 2012 Peter Krusche

This library is bsp_free software; you can redistribute it and/or
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


/** @file bench_f.cpp

Benchmarking functions for floating point performance. 
Loosely based on the Oxford BSP toolset's benchmarking code,
and on the daxpy flop measurements from BSPEdupack.

@author Peter Krusche
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <stdexcept>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/bind.hpp>

#ifdef _HAVE_CBLAS
extern "C" {
#include <cblas.h>	
};
#endif

#include "bsp_alloc.h"
#include "bsp_cpp/bsp_cpp.h"
#include "bench_r.h"

using namespace std;

#ifndef S_DOT_OVERSAMPLE
#define S_DOT_OVERSAMPLE 100
#endif

#ifndef S_MAT_OVERSAMPLE
#define S_MAT_OVERSAMPLE 5
#endif

REGISTER_BENCHMARK("dot", "Dot product via loop", Dot);
REGISTER_BENCHMARK("Udot", "Dot product via uBLAS", DotUBLAS);
REGISTER_BENCHMARK("daxpy", "Vector addition via loop", Daxpy);
REGISTER_BENCHMARK("Udaxpy", "Vector addition via uBLAS", DaxpyUBLAS);
REGISTER_BENCHMARK("MatMult", "Matrix multiplication via IJK loop", MatMult);

#ifdef _HAVE_CBLAS
REGISTER_BENCHMARK("Cdot", "Dot product via CBLAS", DotCBLAS);
REGISTER_BENCHMARK("Cdaxpy", "Vector addition via CBLAS", DaxpyCBLAS);
#endif

/** random number helper. */
template <class _t>
_t positive_random_number (int n) {
	double d = rand();
	d = d * n / RAND_MAX;
	return (_t)d;
}

/** measure the time for n axpy operations on type _t */
template <class _t>
double measure_dot_rate(int n) {
	int i,j, dummy=0;
	double time_clockA, time_clockB;
	_t dot_product1 = 0;
	_t dot_product2 = 0;
	_t *vecA;
	_t *vecB;

	vecA = (_t *)bsp_calloc( n, sizeof(_t) );
	vecB = (_t *)bsp_calloc( n, sizeof(_t) );

	if (vecA==NULL || vecB==NULL) {
		throw std::runtime_error("Failed to allocate vector for dot product used in S");
	}

	for (i=0; i < n; i++) {
		vecA[i] = positive_random_number <_t> ( n );
		vecB[i] = positive_random_number <_t> ( n );
	}

	time_clockA = bsp_time();
	for (i=0;i<S_DOT_OVERSAMPLE;i++) {
		for (j=0; j < n; j++) 
			dot_product1 += vecA[j] * vecB[j]; 
		for (j=n-1; j>=0;j--)
			dot_product2 += vecA[j] * vecB[j]; 
	}
	time_clockB = bsp_time();

	if (dot_product1 > 0 && dot_product2 > 0) {
		/* Need this conditional, otherwise the optimiser might remove the
		dot-product as it isn't used in the code from here on */
		time_clockB = time_clockB - time_clockA;
	} else {
		throw std::runtime_error("Dot product computation gave an incorrect result.");
	}

	bsp_free(vecA);
	bsp_free(vecB);

	/* 2 flops * over_sample * scale factor * dot_product_size */
	return (double) (2*S_DOT_OVERSAMPLE*n) / time_clockB / 1000000.0;
}

/** matrix element accessor */
template <class _t> 
inline _t & matrix_el (int n, _t* m, int i, int j) {
	return *(m + (n*i + j));
}

/** measure the flop rate of matrix multiplication for 2 nxn matrices */
template <class _t>
double measure_matmul_rate(int n) {
	int i,j,k,o;
	_t *matA, *matB, *matC, s,fool_optimiser = 0;
	double time_clockA, time_clockB;

	matA = (_t*)bsp_calloc(n*n, sizeof(_t) );
	matB = (_t*)bsp_calloc(n*n, sizeof(_t) );
	matC = (_t*)bsp_calloc(n*n, sizeof(_t) );

	if (matA==NULL || matB==NULL || matC==NULL)
		throw std::runtime_error("Failed to allocate matrix");

	for(i = 0; i < n; i++) {
		for(j = 0; j < n; j++) {
			matrix_el(n, matA, i, j) = positive_random_number <_t> (n) ;
			matrix_el(n, matB, i, j) = positive_random_number <_t> (n) ;
			matrix_el(n, matC, i, j) = 0 ;
		}
	}

	time_clockA = bsp_time();
	for ( o = 0; o < S_MAT_OVERSAMPLE; o++) {
		for(i = 0; i < n; i++) {
			for(j = 0; j < n; j++) {
				s = 0;
				for(k = 0; k < n ; k++) {
					s += matrix_el(n, matA, i, j) * matrix_el(n, matB, k, j);
				}
				matrix_el(n, matC, i, k) = s;
			}
		}
		fool_optimiser += matrix_el(n, matC, 0, 0);
	}

	time_clockB = bsp_time();

	if (fool_optimiser < 0) {
		throw std::runtime_error("Matrix product returned invalid value.");
	} else {
		time_clockB = time_clockB - time_clockA;
	}

	bsp_free(matA);
	bsp_free(matB);
	bsp_free(matC);

	/* 2 flops * over_sample * scale factor * n*n*n */
	return ((double) 
		(2*S_MAT_OVERSAMPLE*n*n*n) / time_clockB) * 1e-6;
}

double benchmark::Dot::run(int n) {
	return measure_dot_rate<double>(n);
}

/** measure dot product rate with ublas */
double benchmark::DotUBLAS::run(int n) {
	double time0, time1;
	boost::numeric::ublas::vector<double> v1 (n), v2(n);
	double dot_product1 = 0;
	
	for (int j = 0; j < n; ++j) {
		v1 ( j ) = positive_random_number <double> (n) ;
		v2 ( j ) = positive_random_number <double> (n) ;
	}
	
	time0 = bsp_time();
	for ( int o = 0; o < S_DOT_OVERSAMPLE; o++) {
		dot_product1 += boost::numeric::ublas::inner_prod (v1, v2);
	}	
	time1 = bsp_time();
	
	if (dot_product1 >= 0) {
		time1-= time0;
	} else {
		throw std::runtime_error("Dot product computation gave an incorrect result.");		
	}
	return ((double)S_DOT_OVERSAMPLE*n) / time1 * 1e-6;
}

#ifdef _HAVE_CBLAS
/** measure dot product rate with cblas */
double benchmark::DotCBLAS::run(int n) {
	int i,j, dummy=0;
	double time_clockA, time_clockB;
	double dot_product1 = 0;
	double *vecA;
	double *vecB;

	vecA = (double *)bsp_calloc( n, sizeof(double) );
	vecB = (double *)bsp_calloc( n, sizeof(double) );

	if (vecA==NULL || vecB==NULL) {
		throw std::runtime_error("Failed to allocate vector for dot product used in S");
	}

	for (i=0; i < n; i++) {
		vecA[i] = positive_random_number <double> ( n );
		vecB[i] = positive_random_number <double> ( n );
	}

	time_clockA = bsp_time();
	for (i=0;i<S_DOT_OVERSAMPLE;i++) {
		dot_product1+= cblas_ddot(n, vecA, 1,vecB, 1);
	}
	time_clockB = bsp_time();

	if ( dot_product1 > 0 ) {
		/* Need this conditional, otherwise the optimiser might remove the
		dot-product as it isn't used in the code from here on */
		time_clockB = time_clockB - time_clockA;
	} else {
		throw std::runtime_error("Dot product computation gave an incorrect result.");
	}

	bsp_free(vecA);
	bsp_free(vecB);

	/* 2 flops * over_sample * scale factor * dot_product_size */
	return (double) (S_DOT_OVERSAMPLE*n) / time_clockB * 1e-6;	
}
#endif

double benchmark::Daxpy::run(int n) {
	int i, dummy=0;
	double time_clockA, time_clockB;
	double dot_product1 = 1.0;
	double *vecA;
	double *vecB;

	vecA = (double *)bsp_calloc( n, sizeof(double) );
	vecB = (double *)bsp_calloc( n, sizeof(double) );

	if (vecA==NULL || vecB==NULL) {
		throw std::runtime_error("Failed to allocate vector for dot product used in S");
	}

	for (i=0; i < n; i++) {
		vecA[i] = positive_random_number <double> ( n );
		vecB[i] = positive_random_number <double> ( n );
	}

	time_clockA = bsp_time();
	for (i=0;i<S_DOT_OVERSAMPLE;i++) {
		dot_product1+= vecA[0];
		for (int j = 0; j < n; ++j) {
			vecA[j]+= dot_product1*vecB[j];
		}
	}
	time_clockB = bsp_time();

	if ( dot_product1 > 0 ) {
		/* Need this conditional, otherwise the optimiser might remove the
		dot-product as it isn't used in the code from here on */
		time_clockB = time_clockB - time_clockA;
	} else {
		throw std::runtime_error("Daxpy product computation gave an incorrect result.");
	}

	bsp_free(vecA);
	bsp_free(vecB);

	/* 2 flops * over_sample * scale factor * dot_product_size */
	return (double) (S_DOT_OVERSAMPLE*n) / time_clockB * 1e-6;	
}

double benchmark::DaxpyUBLAS::run(int n) {
	double time0, time1;
	boost::numeric::ublas::vector<double> v1 (n), v2 (n);
	double dot_product1 = 1;
	
	for (int j = 0; j < n; ++j) {
		v1 ( j ) = positive_random_number <double> (n) ;
		v2 ( j ) = positive_random_number <double> (n) ;
	}
	
	time0 = bsp_time();
	for ( int o = 0; o < S_DOT_OVERSAMPLE; o++) {
		dot_product1 = v1(0);
		v1+= dot_product1*v2;
	}	
	time1 = bsp_time();
	
	if (dot_product1 >= 0) {
		time1-= time0;
	} else {
		throw std::runtime_error("Dot product computation gave an incorrect result.");		
	}
	return ((double)S_DOT_OVERSAMPLE*n) / time1 * 1e-6;
}

#ifdef _HAVE_CBLAS
double benchmark::DaxpyCBLAS::run(int n) {
	int i,j, dummy=0;
	double time_clockA, time_clockB;
	double dot_product1 = 1.0;
	double *vecA;
	double *vecB;

	vecA = (double *)bsp_calloc( n, sizeof(double) );
	vecB = (double *)bsp_calloc( n, sizeof(double) );

	if (vecA==NULL || vecB==NULL) {
		throw std::runtime_error("Failed to allocate vector for dot product used in S");
	}

	for (i=0; i < n; i++) {
		vecA[i] = positive_random_number <double> ( n );
		vecB[i] = positive_random_number <double> ( n );
	}

	time_clockA = bsp_time();
	for (i=0;i<S_DOT_OVERSAMPLE;i++) {
		dot_product1+= vecA[0];
		cblas_daxpy(n, dot_product1, vecA, 1, vecB, 1);
	}
	time_clockB = bsp_time();

	if ( dot_product1 > 0 ) {
		/* Need this conditional, otherwise the optimiser might remove the
		dot-product as it isn't used in the code from here on */
		time_clockB = time_clockB - time_clockA;
	} else {
		throw std::runtime_error("Daxpy product computation gave an incorrect result.");
	}

	bsp_free(vecA);
	bsp_free(vecB);

	/* 2 flops * over_sample * scale factor * dot_product_size */
	return (double) (S_DOT_OVERSAMPLE*n) / time_clockB * 1e-6;	
}
#endif

double benchmark::MatMult::run(int n) {
	return measure_matmul_rate<double>(n);
}
