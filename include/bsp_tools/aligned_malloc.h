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

#ifndef aligned_malloc_h__
#define aligned_malloc_h__

#include <stdlib.h>

static inline void *aligned_malloc(size_t size, int alignment) {
	char *block = malloc(size + 2*alignment + 4);

	char** aligned = (char**)(((size_t)block + alignment + sizeof(char*)) & (~(alignment-1)));
	aligned[-1] = block;

	return aligned;
}


static inline void aligned_free(void *memory) {
	free(((char**)memory)[-1]);
}

static inline int is_aligned_memory(void * v, int alignment) {
	return (((size_t)v) & (alignment-1)) == 0;
}

#endif // aligned_malloc_h__
