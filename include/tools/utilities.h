/***************************************************************************
 *   Copyright (C) 2007   Peter Krusche, The University of Warwick         *
 *   peter@dcs.warwick.ac.uk                                               *
 ***************************************************************************/

#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <iostream>
#include <algorithm>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string>
#include <sstream>

#define ICD(J,K) (((J)+(K)-1)/(K))
#define ICR(J,K) ((J)-(K)*ICD(J,K))

#define SHL_BD(x,s) ((s) < 0 ? (x) >> (-(s)) : (x) << (s))
#define SHR_BD(x,s) ((s) < 0 ? (x) << (-(s)) : (x) >> (s))

/**
 * 
 * @brief Return factorial (n)
 * 
 * @param n the number to compute n! of
 * 
 * @return n!
 */
template<typename _i>
_i factorial (_i n) {
	_i v = 1;
	for(_i j = 1; j <= n; ++j) {
		v*= j;
	}
	return v;
}

namespace utilities {

extern void warmup(double t = 2.0);
extern double time();

inline double randnum(double l) {
	return(l*rand()/(RAND_MAX+1.0));
}

inline bool fexists(const char * name) {
	FILE * fp= fopen(name, "r");
	if (fp) {
		fclose(fp);
		return true;
	} else
	return false;
}

inline size_t fsize(const char * name) {
	FILE * fp= fopen(name, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t s = ftell(fp);
		fclose(fp);
		return s;
	} else {
		return 0;
	}
}

template<class _value_t>
struct rand_range : public std::pair<_value_t, _value_t> {
	rand_range(_value_t _size = RAND_MAX) {
		this->first = (_value_t )randnum((double)_size);
		this->second = (_value_t )randnum((double)_size);
		std::swap(this->first, this->second);
	}
};
	
};

#endif

