/*
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
    Copyright (C) 2006  Wijnand J. Suijlen, 2012, Peter Krusche
                                                                                
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
#ifndef __BSP_Fold_h__
#define __BSP_Fold_h__ 

#include "bsp_config.h"

#ifdef _HAVE_MPI

#ifdef __cplusplus
extern "C" {
#endif
extern 	MPI_Comm bsp_communicator;
#ifdef __cplusplus
} /* extern "C" */
#endif

/** Implementation of bsp_fold using MPI_Allgather. 

see http://www.bsp-worldwide.org/implmnts/oxtool/man/bsp_fold.3.html
*/
inline void bsp_fold( void (*op)(void*,void*,void*,int*),
    void *src, void *dst, int nbytes) {	
	int procs = bsp_nprocs();
	
	if (procs > 1) {
		char * alldata = (char*)bsp_malloc(nbytes*procs);
		MPI_Allgather(src, nbytes, alldata, nbytes, bsp_communicator);

		for (int j = 0; j < procs - 1; ++j) {
			op (dst, alldata + nbytes*j, alldata + nbytes*(j+1), nbytes);
		}

		bsp_free(alldata);		
	} else {
		memcpy(dst, src, nbytes);		
	}
}

#else

/** nothing to do but copy on a single processor */
static inline void bsp_fold( void (*op)(void*,void*,void*,int*),
     void *src, void *dst, int nbytes) {
	memcpy(dst, src, nbytes);
}

#endif // _HAVE_MPI

#ifdef __cplusplus

#include <tbb/blocked_range.h>

/** in C++ we also accept TBB style reducers. */
template<typename Range, typename Body>
void bsp_fold( const Range& range, Body& body ) {
	
}

#endif

#endif