/***************************************************************************
*   Copyright (C) 2007   Peter Krusche, The University of Warwick         *
*   peter@dcs.warwick.ac.uk                                               *
***************************************************************************/

#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <stdio.h>
#include <stdlib.h>

// Integer ceiling division 
#define ICD(J,K) (((J)+(K)-1)/(K))
// Integer ceiling remainder
#define ICR(J,K) ((J)-(K)*ICD(J,K))

// both directions shifting
#define SHL_BD(x,s) ((s) < 0 ? (x) >> (-(s)) : (x) << (s))
#define SHR_BD(x,s) ((s) < 0 ? (x) << (-(s)) : (x) >> (s))

#ifdef __cplusplus

namespace utilities {

	/** Don't use this if you want actually good random numbers.
	*/

	inline double randnum(double l) {
		return(l*rand()/(RAND_MAX+1.0));
	}

	/** Simple file existence helper
	*/

	inline bool fexists(const char * name) {
		FILE * fp= fopen(name, "r");
		if (fp) {
			fclose(fp);
			return true;
		} else
			return false;
	}

	/** Get file size.
	*/

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

};

#endif

#endif

