/*
    BSPonMPI. This is an implementation of the BSPlib standard on top of MPI
    Copyright (C) 2006  Wijnand J. Suijlen
                                                                                
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

#ifndef __BSP_BROADCAST_H__
#define __BSP_BROADCAST_H__

#include "bsp_config.h"

#ifdef _HAVE_MPI
#include "mpi.h"

static inline void bsp_broadcast(int source, void* source_data, size_t len) {
	MPI_Bcast(source_data, (int)len, MPI_BYTE, source, MPI_COMM_WORLD);
}
#else
static inline void bsp_broadcast(int source, void* source_data, size_t len) {
}
#endif // _HAVE_MPI

#ifdef __cplusplus

namespace bsp {

	/************************************************************************/
	/* Various generic broadcast functions                                  */
	/************************************************************************/

	template <typename _t>
	inline void bsp_broadcast(int source, _t & data) {
		::bsp_broadcast(source, &data, sizeof(_t));
	}

	template <>
	inline void bsp_broadcast<std::string>(int source, std::string & data) {
		size_t len;

		len = data.size();
		bsp_broadcast(source, len);

		char * data_copy = new char[len];

		if(bsp_pid() == source) {
			data.resize(len);
			memcpy(data_copy, data.c_str(), len);
		}

		::bsp_broadcast(source, data_copy, len);
		data = std::string(data_copy,len);
		delete [] data_copy;
	}
};

#endif

#endif

